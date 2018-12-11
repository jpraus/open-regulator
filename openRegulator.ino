

/**
 * TODO: 
 * - pri prekroceni urciteho tlaku (2.6?) spustit topeni naplno aby se co nejvic vychladila voda a snizil se tlak
 * - ovladaci tlacitko pro zobrazeni podrobnosti?
 */

#include "regulator.h"
#include "ui.h"
#include "state.h"
#include "otController.h"
#include "ioDriver.h"
#include "accumulatorCtrl.h"
//#include "uplink.h"

#define ACTY_LED 13

// display 128x64 (radic ST7920)
#define DSPY_BCKLIGHT 28
#define DSPY_EN 29
#define DSPY_RW 30
#define DSPY_RS 31

// OpenTherm Boiler/Thermostat protocol
#define THERMOSTAT_IN 2
#define THERMOSTAT_OUT 4
#define BOILER_IN 3
#define BOILER_OUT 5

// Shift register for digital OUT (74HC595)
#define LATCH_PIN 8
#define CLOCK_PIN 9
#define DATA_PIN 6
#define OE_PIN 7

// Analog multiplexor (4051N)
#define IN_PIN A6
#define A_PIN A3
#define B_PIN A4
#define C_PIN A5

// Pressure sensor
#define PRESSURE_SENSOR_PIN A7

int everySecondTimer = 0;
unsigned long deltaMsRef = 0;

IO_DRIVER io;
STATE state;
ACCUMULATOR_CTRL accumulatorCtrl(&io, &state);
OT_CONTROLLER openthermCtrl(&state, &accumulatorCtrl, THERMOSTAT_IN, THERMOSTAT_OUT, BOILER_IN, BOILER_OUT);
U8G2_ST7920_128X64_1_SW_SPI lcd(U8G2_R0, /* clock=*/ DSPY_EN, /* data=*/ DSPY_RW, /* CS=*/ DSPY_RS, /* reset=*/ U8X8_PIN_NONE);
UI ui(&lcd, &state, DSPY_BCKLIGHT);
//UPLINK uplink(&state);

void setup() {
  Serial.begin(115200);
  pinMode(PRESSURE_SENSOR_PIN, INPUT);
  pinMode(ACTY_LED, OUTPUT);

  state.load();
  io.setup();
  accumulatorCtrl.setup();
  openthermCtrl.setup();
  ui.setup();
  //uplink.setup();

  deltaMsRef = millis();
  everySecondTimer = 1000;
}

void loop() {
  int deltaMs = getDeltaMs();
  openthermCtrl.update(deltaMs);
  accumulatorCtrl.update(deltaMs);

  everySecondTimer -= deltaMs;
  if (everySecondTimer <= 0) {
    everySecondTimer += 1000;
    digitalWrite(ACTY_LED, HIGH);
    doEverySecond();
    digitalWrite(ACTY_LED, LOW);
  }

  //uplink.update(deltaMs);
  ui.draw();
}

void doEverySecond() {
  // every second ticks
  ui.update();

  // read temps
  state.accumulator.chTemp = io.readTemp(2);
  state.accumulator.returnTemp = io.readTemp(3);
  state.accumulator.topTemp = io.readTemp(4);
  state.accumulator.bottomTemp = io.readTemp(1);

  // read pressure sensor and map it to bars
  //state.accumulator.pressure = mapfloat(analogRead(PRESSURE_SENSOR_PIN), 118, 921, 0, 12);

  // reset OT online status
  if (state.thermostat.online > 0) {
    state.thermostat.online --;
  }

  // persist configuration data to EEPROM
  //state.store();
}

int getDeltaMs() {
  unsigned long time = millis();
  if (time < deltaMsRef) {
    deltaMsRef = time; // overflow of millis happen, we lost some millis but that does not matter for us, keep moving on
    return time;
  }
  int deltaMs = time - deltaMsRef;
  deltaMsRef += deltaMs;
  return deltaMs;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
