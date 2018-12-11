#ifndef STATE_H
#define STATE_H

#define LOG_LINES 8

struct BoilerState {
  bool fault;
  bool chOn;
  bool dhwOn;
  bool flameOn;
  float modulationLvl;
  float feedTemp;
  float returnTemp;
  unsigned long dhwTotalMs = 0; // how long DWH pump was running in total
  unsigned long chTotalMs = 0;  // how long CH pump was running in total
};

struct ThermostatState {
  bool chOn;
  bool dhwOn;
  float outsideTemp;
  float roomTemp;
  float dhwTemp;
  float chSetpoint;
  float dhwSetpoint;
  byte online; // OT online counter (0 = offline)
};

struct Accumulator {
  bool chOn;
  bool dhwOn;
  float topTemp;
  float bottomTemp;
  float chTemp;
  float returnTemp;
  float valveAngle;
  float pressure;
  byte mode;
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
    void logMessage(String message);

    BoilerState boiler; // read-only
    ThermostatState thermostat; // read-only
    Accumulator accumulator; // read-write
    Config config; // consts

    String logMessages[LOG_LINES];
};

#endif
