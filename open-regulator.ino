/**
 * TODO: 
 * - pri prekroceni urciteho tlaku (2.6?) spustit topeni naplno aby se co nejvic vychladila voda a snizil se tlak
 * - ovladaci tlacitko pro zobrazeni podrobnosti?
 */

#include "regulator.h"
#include "ui.h"
#include "state.h"
//#include "otController.h"
#include "ioDriver.h"
//#include "accumulatorCtrl.h"
//#include "uplink.h"

int everySecondTimer = 0;
unsigned long deltaMsRef = 0;

byte actyLed = HIGH;

IO_DRIVER io;
STATE state;
//ACCUMULATOR_CTRL accumulatorCtrl(&io, &state);
//OT_CONTROLLER openthermCtrl(&state, &accumulatorCtrl, THERMOSTAT_IN, THERMOSTAT_OUT, BOILER_IN, BOILER_OUT);
U8G2_ST7920_128X64_1_SW_SPI lcd(U8G2_R0, /* clock=*/ DSPY_EN, /* data=*/ DSPY_RW, /* CS=*/ DSPY_RS, /* reset=*/ U8X8_PIN_NONE);
UI ui(&lcd, &state, DSPY_BCKLIGHT);
//UPLINK uplink(&state);

void setup() {
  Serial.begin(115200);
  Serial.println("OpenRegulator 0.1");

  pinMode(ACTY_LED, OUTPUT);
  digitalWrite(ACTY_LED, HIGH);

  state.load(); // <- freezing the device
  io.setup();
  //accumulatorCtrl.setup();
  //openthermCtrl.setup();
  ui.setup();
  //uplink.setup();

  deltaMsRef = millis();
  everySecondTimer = 1000;
}

void loop() {
  int deltaMs = getDeltaMs();
  //openthermCtrl.update(deltaMs);
  //accumulatorCtrl.update(deltaMs);

  everySecondTimer -= deltaMs;
  if (everySecondTimer <= 0) {
    everySecondTimer += 1000;
    doEverySecond();
    actyLed = (actyLed == HIGH ? LOW : HIGH);
    digitalWrite(ACTY_LED, actyLed);
  }

  //uplink.update(deltaMs);
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
