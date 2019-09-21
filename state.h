#ifndef STATE_H
#define STATE_H

struct BoilerState {
  bool fault = false;
  bool chOn = false;
  bool dhwOn = false;
  bool flameOn = false;
  float modulationLvl = 0;
  float feedTemp = 0;
  float returnTemp = 0;
  unsigned long dhwTotalMs = 0; // how long DWH pump was running in total
  unsigned long chTotalMs = 0;  // how long CH pump was running in total
};

struct ThermostatState {
  bool chOn = false;
  bool dhwOn = false;
  float outsideTemp = 0;
  float roomTemp = 0;
  float dhwTemp = 0;
  float chSetpoint = 0;
  float dhwSetpoint = 0;
  byte online = 0; // OT online counter (0 = offline)
};

struct Accumulator {
  bool chOn = false;
  bool dhwOn = false;
  float topTemp = 0;
  float bottomTemp = 0;
  float chTemp = 0;
  float returnTemp = 0;
  float valveAngle = 0;
  float pressure = 0;
  byte mode = 0;
  unsigned long dhwTotalMs = 0; // how long DWH pump was running in total
  unsigned long chTotalMs = 0;  // how long CH pump was running in total
  unsigned long dhwNotRunMs = 0; // how long DHW pump was not running - used for anti-stop alogrithm
  unsigned long chNotRunMs = 0;  // how long CH pump was not running - used for anti-stop alogrithm
};

struct Config {
  byte dhwFeedingTempExcess; // minimalni nadbytek napajeci teploty pro zapnuti TUV cerpadla z AKU nadrze (v AKU musi byt vyssi teplota pro natopeni TUV)
  byte hysteresis; // o kolik vic musi mit zdroj tepla aby se zapnulo natapeni TUV/CH
  byte dhwHysteresis; // kdy prestaneme natapet nadrz na teplou vodu
  long antiStopMs; // max time pumps are not run
};

class STATE {
  public:
    STATE() {
      config.dhwFeedingTempExcess = 7; // at leat 7°C above DHW setpoint to run feeding from AKU
      config.hysteresis = 5; // at least 5°C above CH/DHW setpoint to start feeding from AKU (in case of DHW + dhwFeedingTempExcess)
      config.dhwHysteresis = 2; // stop when 2°C above DHW setpoint
      config.antiStopMs = 432000000; // run each pump at least every 5days
    }

    void store();
    bool load();

    BoilerState boiler; // read-only
    ThermostatState thermostat; // read-only
    Accumulator accumulator; // read-write
    Config config; // consts
};

#endif
