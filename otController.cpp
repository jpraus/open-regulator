#include "regulator.h"
#include "otController.h"

#define PHASE_LISTEN_MASTER   1
#define PHASE_REQUEST_SLAVE   2
#define PHASE_RESPOND_MASTER  3

#define MODE_GATEWAY  0
#define MODE_MASTER   1

OT_CONTROLLER::OT_CONTROLLER(STATE *state, ACCUMULATOR_CTRL *accumulatorCtrl, byte pinMasterIn, byte pinMasterOut, byte pinSlaveIn, byte pinSlaveOut)
    : _state(*state), _accumulatorCtrl(*accumulatorCtrl) {
  _pinMasterIn = pinMasterIn;
  _pinMasterOut = pinMasterOut;
  _pinSlaveIn = pinSlaveIn;
  _pinSlaveOut = pinSlaveOut;
}

void OT_CONTROLLER::setup() {
  pinMode(_pinMasterIn, INPUT);
  digitalWrite(_pinMasterOut, HIGH);
  pinMode(_pinMasterOut, OUTPUT); // low output = high current, high output = low current
  pinMode(_pinSlaveIn, INPUT);
  digitalWrite(_pinSlaveOut, HIGH);
  pinMode(_pinSlaveOut, OUTPUT); // low output = high voltage, high output = low voltage

  _mode = MODE_GATEWAY;
  _phase = PHASE_LISTEN_MASTER;
}

void OT_CONTROLLER::_interceptRequest() {
  if (_request.id == OT_MSGID_STATUS) {
    _state.thermostat.chOn = bitRead(_request.valueHB, 0);
    _state.thermostat.dhwOn = bitRead(_request.valueHB, 1);
    
    byte override = _accumulatorCtrl.getOverrideOTFlags();
    if (override > 0) {
      _request.valueHB = 0; // override request - act as master dont want to do anything
      _response.type = OT_MSGTYPE_READ_ACK;  // override response - ask as slave is doing what required
      _response.id = OT_MSGID_STATUS;
      _response.valueHB = _request.valueHB;
      _response.valueLB = override;
      _responseOverridden = true;
    }
  }
  else if (_request.id == OT_MSGID_CH_SETPOINT) {
    _state.thermostat.chSetpoint = _request.f88();
  }
  else if (_request.id == OT_MSGID_DHW_SETPOINT) {
    _state.thermostat.dhwSetpoint = _request.f88();
  }
}

void OT_CONTROLLER::_interceptResponse() {
  if (_response.id == OT_MSGID_STATUS) {
    _state.boiler.fault = bitRead(_response.valueLB, 0);
    _state.boiler.chOn = bitRead(_response.valueLB, 1);
    _state.boiler.dhwOn = bitRead(_response.valueLB, 2);
    _state.boiler.flameOn = bitRead(_response.valueLB, 3);
  }
  else if (_response.id == OT_MSGID_MODULATION_LEVEL) {
    _state.boiler.modulationLvl = _response.f88();
  }
  else if (_response.id == OT_MSGID_ROOM_TEMP) {
    _state.thermostat.roomTemp = _response.f88();
  }
  else if (_response.id == OT_MSGID_FEED_TEMP) {
    _state.boiler.feedTemp = _response.f88();
    // override with accumulator feed temp if running from accumulator
    if (_state.accumulator.chOn) { 
      _response.f88(_state.accumulator.chTemp);
      _responseOverridden = true;
    }
    else if (_state.accumulator.dhwOn) {
      _response.f88(_state.accumulator.topTemp);
      _responseOverridden = true;
    }
  }
  else if (_response.id == OT_MSGID_DHW_TEMP) {
    _state.thermostat.dhwTemp = _response.f88();
  }
  else if (_response.id == OT_MSGID_OUTSIDE_TEMP) {
    _state.thermostat.outsideTemp = _response.f88();
  }
  else if (_response.id == OT_MSGID_RETURN_WATER_TEMP) {
    _state.boiler.returnTemp = _response.f88();
    // override with accumulator return temp if running from accumulator
    if (_state.accumulator.chOn || _state.accumulator.dhwOn) {
      _response.f88(_state.accumulator.returnTemp);
      _responseOverridden = true;
    }
  }
  _state.thermostat.online = 10;
}

void OT_CONTROLLER::update(int deltaMs) {
  if (_phase == PHASE_LISTEN_MASTER) {
    if (OPENTHERM::isIdle() || OPENTHERM::isError() || OPENTHERM::isSent()) { // start listening when not listening
      OPENTHERM::listen(_pinMasterIn);
    }
    else if (OPENTHERM::getMessage(_request)) { // request received
      OPENTHERM::stop();
      _phase = PHASE_REQUEST_SLAVE;
      _responseOverridden = false;
      _logRequestToSerial();
      _interceptRequest();
    }
  }
  else if (_phase == PHASE_REQUEST_SLAVE) {
    if (OPENTHERM::isIdle()) { // send request to slave
      OPENTHERM::send(_pinSlaveOut, _request);
    }
    else if (OPENTHERM::isSent()) { // await response from slave with timeout of 800ms
      OPENTHERM::listen(_pinSlaveIn, 800);
    }
    else if (OPENTHERM::hasMessage()) { // response received
      if (!_responseOverridden) {
        OPENTHERM::getMessage(_response);
        _interceptResponse();
      }
      OPENTHERM::stop();
      _phase = (_mode == MODE_GATEWAY) ? PHASE_RESPOND_MASTER : PHASE_LISTEN_MASTER;
      _logResponseToSerial();
    }
    else if (OPENTHERM::isError()) { // no response from slave or error in it, lets skip it and listen to next request from master
      OPENTHERM::stop();
      _phase = PHASE_LISTEN_MASTER;
      _onRequestTimeout();
    }
  }
  else if (_phase == PHASE_RESPOND_MASTER) {
    if (OPENTHERM::isIdle()) { // send response to master
      OPENTHERM::send(_pinMasterOut, _response);
    }
    else if (OPENTHERM::isSent()) { // await response from slave with timeout of 800ms
      OPENTHERM::stop();
      _phase = PHASE_LISTEN_MASTER;
    }
  }

  // update stats
  if (_state.boiler.chOn && _state.boiler.flameOn) {
    _state.boiler.chTotalMs += deltaMs;
  }
  if (_state.boiler.dhwOn && _state.boiler.flameOn) {
    _state.boiler.dhwTotalMs += deltaMs;
  }
}

void OT_CONTROLLER::_onRequestTimeout() {
  Serial.print("[OT] ");
  OPENTHERM::printToSerial(_request);
  Serial.println(" -> Timeout");
}

void OT_CONTROLLER::_logRequestToSerial() {
  #ifdef DEBUG
    if (_request.id == OT_MSGID_STATUS) {
      Serial.print("[OT] Master status: ");
      Serial.println(_request.valueHB, BIN);
    }
    else if (_request.id == OT_MSGID_CH_SETPOINT) {
      Serial.print("[OT] CH setpoint: ");
      Serial.print(_request.f88());
      Serial.println(" C");
    }
    else if (_request.id == OT_MSGID_DHW_SETPOINT) {
      Serial.print("[OT] DHW setpoint: ");
      Serial.print(_request.f88());
      Serial.println(" C");
    }
    else if (_request.id == OT_MSGID_ROOM_SETPOINT) {
      Serial.print("[OT] Room setpoint: ");
      Serial.print(_request.f88());
      Serial.println(" C");
    }
  #endif
}

void OT_CONTROLLER::_logResponseToSerial() {
  #ifdef DEBUG
    if (_response.id == OT_MSGID_STATUS) {
      Serial.print("[OT] Slave status: ");
      Serial.print(_response.valueLB, BIN);
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_MODULATION_LEVEL) {
      Serial.print("[OT] Modulation lvl: ");
      Serial.print(_response.f88());
      Serial.print(" %");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_ROOM_TEMP) {
      Serial.print("[OT] Room temp: ");
      Serial.print(_response.f88());
      Serial.print(" C");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_FEED_TEMP) {
      Serial.print("[OT] Feed temp: ");
      Serial.print(_response.f88());
      Serial.print(" C");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_DHW_TEMP) {
      Serial.print("[OT] DHW temp: ");
      Serial.print(_response.f88());
      Serial.print(" C");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_OUTSIDE_TEMP) {
      Serial.print("[OT] Outside temp: ");
      Serial.print(_response.f88());
      Serial.print(" C");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_RETURN_WATER_TEMP) {
      Serial.print("[OT] Return temp: ");
      Serial.print(_response.f88());
      Serial.print(" C");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_EXHAUST_TEMP) {
      Serial.print("[OT] Exhaust temp: ");
      Serial.print(_response.s16());
      Serial.print(" C");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_DHW_FLOW_RATE) {
      Serial.print("[OT] Flow rate: ");
      Serial.print(_response.f88());
      Serial.print(" l/m");
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_FAULT_FLAGS) {
      Serial.print("[OT] Faults: ");
      Serial.print(_response.valueHB, BIN);
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
    else if (_response.id == OT_MSGID_CH_SETPOINT) { 
      // already logged in request
    }
    else if (_response.id == OT_MSGID_DHW_SETPOINT) { 
      // already logged in request
    }
    else if (_request.id == OT_MSGID_ROOM_SETPOINT) {
      // already logged in request
    }
    else {
      Serial.print("[OT] ");
      OPENTHERM::printToSerial(_request);
      Serial.print(" -> ");
      OPENTHERM::printToSerial(_response);
      Serial.println(_responseOverridden ? " (OV)" : "");
    }
  #endif
}
