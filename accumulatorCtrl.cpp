#include "regulator.h"
#include "accumulatorCtrl.h"

#define MODE_NONE           0
#define MODE_CH_RESET       1
#define MODE_CH_ON          2
#define MODE_DHW_ON         3
#define MODE_CH_ANTISTOP_1  10 // valve full open
#define MODE_CH_ANTISTOP_2  11 // valve full close
#define MODE_CH_ANTISTOP_3  12 // pump running
#define MODE_DHW_ANTISTOP   13

void ACCUMULATOR_CTRL::setup() {
  _valve.setup();
  _changeMode(MODE_NONE);
}

void ACCUMULATOR_CTRL::update(int deltaMs) {
  if (_timer > 0) {
    _timer -= deltaMs;
  }
  _valve.update(deltaMs); // need to be called first to compute estimated angle correctly (smallest time deviation)
  _updateState();
  _updateDhw(deltaMs);
  _updateCh(deltaMs);
  _updateAntiStop(deltaMs);
}

byte ACCUMULATOR_CTRL::getOverrideOTFlags() {
  _updateState();

  if (_state.accumulator.dhwOn) {
    return B100; // DHW on
  }
  if (_state.accumulator.chOn) {
    return B10; // CH on
  }

  return 0; // no override
}

void ACCUMULATOR_CTRL::_updateState() {
  // DHW - termostat jenom urcuje teplotu DHW a jestli ma byt zrovna k dispozici, ale nezastavi kdyz uz je nahrata
  if (_state.thermostat.dhwOn) {
    if (_state.thermostat.dhwSetpoint < _state.thermostat.dhwTemp - _state.config.dhwHysteresis) {
      _state.accumulator.dhwOn = false; // stop/nothing - DHW is hot
    }
    else if (_state.accumulator.dhwOn == false && _state.boiler.dhwOn == false // do not start DHW if boiler is already on fire, let him finish the job
        && _state.thermostat.dhwTemp <= _state.accumulator.topTemp - _state.config.dhwFeedingTempExcess - _state.config.hysteresis) {
      _state.accumulator.dhwOn = true; // start
    }
    else if (_state.accumulator.dhwOn && _state.thermostat.dhwTemp > _state.accumulator.topTemp - _state.config.dhwFeedingTempExcess) {
      _state.accumulator.dhwOn = false; // stop - not enought energy
    }
    // else no change
  }
  else {
     _state.accumulator.dhwOn = false; // stop - DHW is off
  }

  // CH - termostat rozhoduje o tom zda se bude topit CH nebo nebude, my jenom kouknem zda na to mame energii
  if (_state.thermostat.chOn) {
    if (_state.accumulator.chOn == false && _state.thermostat.chSetpoint <= _state.accumulator.topTemp - _state.config.hysteresis) {
      _state.accumulator.chOn = true; // start
    }
    else if (_state.accumulator.chOn == true && _state.accumulator.valveAngle >= 90.0 && _state.accumulator.chTemp < _state.thermostat.chSetpoint && _state.thermostat.chSetpoint > _state.accumulator.topTemp - _state.config.hysteresis) { // topime z nadrze ale uz nemame dost energie, prejdeme na kotel
      _state.accumulator.chOn = false; // stop - not enought energy
    }
    // else no change
  }
  else {
    _state.accumulator.chOn = false; // stop - CH is off
  }

  // DHW has priority
  if (_state.accumulator.dhwOn || _state.boiler.dhwOn) {
    _state.accumulator.chOn = false;
  }
}

void ACCUMULATOR_CTRL::_updateAntiStop(int deltaMs) {
  _state.accumulator.dhwNotRunMs += deltaMs;
  _state.accumulator.chNotRunMs += deltaMs;

  if (_mode == MODE_CH_ANTISTOP_1 && _valve.isIdle()) { // valve full open, full close it now
    _valve.fullClose(); // full close valve
    _changeMode(MODE_CH_ANTISTOP_2);

    Serial.println("[CH] AS valve close");
  }
  else if (_mode == MODE_CH_ANTISTOP_2 && _valve.isIdle()) { // valve full close, run pump
    _io.chPump(true);
    _timer = 60000; // run CH pump for 1 minute
    _changeMode(MODE_CH_ANTISTOP_3);

    Serial.println("[CH] AS pump");
  }
  else if (_mode == MODE_CH_ANTISTOP_3 && _timer <= 0) { // CH pump run, stop AS
    _io.chPump(false);
    _changeMode(MODE_NONE);

    Serial.println("[CH] AS off");
  }
  else if (_mode == MODE_DHW_ANTISTOP && _timer <= 0) { // DHW pump run, stop AS
    _io.dhwPump(false);
    _changeMode(MODE_NONE);

    Serial.println("[DHW] AS off");
  }
  else if (_mode == MODE_NONE && _state.boiler.chOn == false && _state.boiler.dhwOn == false) { // start AS unless boiler is currently running
    if (_state.accumulator.chNotRunMs > _state.config.antiStopMs) { //enagage anti-stop feature - run CH pump for 1 minute, full open/close valve
      _changeMode(MODE_CH_ANTISTOP_1);
      _state.accumulator.chNotRunMs = 0;
      _valve.move(90); // full open valve

      Serial.println("[CH] AS valve open");
    }
    else if (_state.accumulator.dhwNotRunMs > _state.config.antiStopMs) { //enagage anti-stop feature - run DHW pump for 1 minute
      _changeMode(MODE_DHW_ANTISTOP);
      _timer = 60000; // run DHW pump for 1 minute
      _state.accumulator.dhwNotRunMs = 0;
      _io.dhwPump(true);

      Serial.println("[DWH] AS pump");
    }
  }
}

void ACCUMULATOR_CTRL::_updateDhw(int deltaMs) {
  if (_mode == MODE_DHW_ON) {
    _state.accumulator.dhwTotalMs += deltaMs;
    _state.accumulator.dhwNotRunMs = 0;
  }
  if (_state.accumulator.dhwOn) {
    if (_mode == MODE_NONE) {
      _io.dhwPump(true);
      _changeMode(MODE_DHW_ON);

      Serial.println("[DHW] on");
    }
  }
  else if (_mode == MODE_DHW_ON) {
    _io.dhwPump(false);
    _changeMode(MODE_NONE);

    Serial.println("[DHW] off");
  }
}

void ACCUMULATOR_CTRL::_updateCh(int deltaMs) {
  if (_mode == MODE_CH_ON) {
    _state.accumulator.chTotalMs += deltaMs;
    _state.accumulator.chNotRunMs = 0;
  }  
  if (_state.accumulator.chOn) {
    if (_mode == MODE_NONE && _valve.isIdle()) { // CH was off, start with valve reset to full close if not yet done
      _valve.fullClose();
      _changeMode(MODE_CH_RESET);

      Serial.println("[CH] start");
    }
    else if (_mode == MODE_CH_RESET && _valve.isIdle()) { // CH valve full close, start pump and open valve to estimated angle
      _io.chPump(true);
      _valve.move(50); // valve is currently full close so let it open to 50 degree to start mixing water
      _timer = 120000; // 60s should be enough for water to start flowing and sensors to get correct readings -> after this timeout start regulation of valve
      _changeMode(MODE_CH_ON);

      Serial.println("[CH] pump start, valve warm up");
    }
    else if (_mode == MODE_CH_ON) { // ch is on, regulate valve to achive CH feed temp equals CH setpoint
      _regulateValve(deltaMs);
    }
  }
  else if (_mode == MODE_CH_RESET || _mode == MODE_CH_ON) {
    _io.chPump(false);
    _valve.fullClose();
    _changeMode(MODE_NONE);

    Serial.println("[CH] off");
  }
}

/**
 * Regulation algorithm:
 * - read difference of setpoint and output
 * - if difference is worse then 0.5 C then move valve in appropriate direction
 * - calculate time coefficient based on difference of input temp and return temp (higher difference then slower we need to go)
 * - wait for that regulation time for feedback
 */
void ACCUMULATOR_CTRL::_regulateValve(int deltaMs) {
  if (_timer > 0) {
    return; // time to next step not passed yet
  }

  // TODO: utilize full close/open when return/top temp equals to setpoint temp  
  float difference = _state.thermostat.chSetpoint - _state.accumulator.chTemp;
  float error = abs(difference);

  if (error < 0.5) {
    _valve.stop();
    return; // good enought error
  }

  // try to move valve to fix error
  int direction = SIGNUM(difference);
  _valve.move(direction); // move for angle of 1

  // coefficient = hot/cold water ration of inputs of valve to calc how big change is needed to control output temp
  float coef = (_state.accumulator.topTemp - _state.accumulator.returnTemp) / 10.0; // higher divider = slower change
  coef = coef / (error / 4.0); // speed up regulation when error is significant - higher divider = slower change
  _timer = coef * 1000; // ms
  if (_timer < 250) {
    _timer = 250; // smallest step is 250 ms
  }

  Serial.print("[CH] valve regulate (");
  Serial.print(coef);
  Serial.print(") ");
  Serial.println(_timer);
}

void ACCUMULATOR_CTRL::_changeMode(byte mode) {
  _mode = mode;
  _state.accumulator.mode = mode;
}
