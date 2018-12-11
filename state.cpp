#include "regulator.h"
#include "state.h"
#include <EEPROM.h>

#define VERSION 1 // this must be written on first byte of memory to verify that memory can be read from

struct Data {
  unsigned long dhwNotRunMs;
  unsigned long chNotRunMs;
  unsigned long accumulatorDhwTotalMs;
  unsigned long accumulatorChTotalMs;
  unsigned long boilerDhwTotalMs;
  unsigned long boilerChTotalMs;
};

void STATE::store() {
  byte versionCode = VERSION;
  EEPROM.write(0, versionCode);

  Data data = {
    accumulator.dhwNotRunMs,
    accumulator.chNotRunMs,
    accumulator.dhwTotalMs,
    accumulator.chTotalMs,
    boiler.dhwTotalMs,
    boiler.chTotalMs
  };
  EEPROM.put(1, data);
}

bool STATE::load() {
  Data data;
  byte versionCode; 

  versionCode = EEPROM.read(0); // version
  if (versionCode == VERSION) {
    EEPROM.get(1, data);

    accumulator.dhwNotRunMs = data.dhwNotRunMs;
    accumulator.chNotRunMs = data.chNotRunMs;
    accumulator.dhwTotalMs = data.accumulatorDhwTotalMs;
    accumulator.chTotalMs = data.accumulatorChTotalMs;

    boiler.dhwTotalMs = data.boilerDhwTotalMs;
    boiler.chTotalMs = data.boilerChTotalMs;

    return true;
  }
  return false;
}

void STATE::logMessage(String message) {
  for (int i = LOG_LINES - 1; i > 0 - 1; i--) {
    logMessages[i] = logMessages[i - 1];
  }
  logMessages[0] = message;
}
