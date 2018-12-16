#include "regulator.h"
#include "ioDriver.h"

#define TERM_R25 2000 // referencni odpor termistoru pri 25°C
#define TERM_R 2192 // hodnota odporu v delici s termistorem

#define TEMPS_COUNT 6
#define RELAYS_COUNT 5

byte tempPins[] = {PIN_T1, PIN_T2, PIN_T3, PIN_T4, PIN_T5, PIN_T6};
byte relayPins[] = {PIN_RELAY1, PIN_RELAY2, PIN_RELAY3, PIN_RELAY4, PIN_RELAY5};

void IO_DRIVER::setup() {
  byte i;

  // temperature & pressure
  pinMode(PIN_PRESSURE, INPUT);
  for (i = 0; i < TEMPS_COUNT; i++) {
    //pinMode(tempPins[i], INPUT); // TODO: setup only connected ones?
  }

  // relays
  for (i = 0; i < RELAYS_COUNT; i++) {
    //digitalWrite(relayPins[i], LOW);
    //pinMode(relayPins[i], OUTPUT);
  }
  //pinMode(PIN_RELAY3, OUTPUT);

  // rotary encoder
  pinMode(PIN_ROTARY_ENC_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_ENC_BUTTON), IO_DRIVER::_rotaryEncoderButtonISR, RISING); // 0 = pressed
}

void IO_DRIVER::_rotaryEncoderButtonISR() {
  logMessage("Click");
}

void IO_DRIVER::valve(int8_t direction) {
  //bitWrite(_value, SHIFT_BIT_VALVE_RIGHT, (direction < 0 ? 1 : 0));
  //bitWrite(_value, SHIFT_BIT_VALVE_LEFT, (direction > 0 ? 1 : 0));
}

void IO_DRIVER::chPump(bool on) {
  //bitWrite(_value, SHIFT_BIT_CH_PUMP, (on ? 1 : 0));
}

void IO_DRIVER::dhwPump(bool on) {
  //bitWrite(_value, SHIFT_BIT_DHW_PUMP, (on ? 1 : 0));
}

float IO_DRIVER::readPressure() {
  // TODO: read pressure
  return 1.0;
}

float IO_DRIVER::readTemp(const byte which) {
  byte pinNumber = tempPins[which - 1];
  analogRead(pinNumber); // read and throwaway first reading, due to internal implementation of circuits its inaccurate in lot of cases (after reading another pin, or writing to another pin)

  float value = analogRead(pinNumber);
  if (value >= 1000.0) {
    return -1; // KTY probably not connected
  }
  return _analogKtyToCelsius(value);
}

float IO_DRIVER::_analogKtyToCelsius(float analogValue) {
  float ukty = VCC * analogValue / 1024.0; // vypocet napeti na senzoru
  float a = 0.007874 * TERM_R25; // alfa x R25
  float b = 0.00001874 * TERM_R25; // beta x R25
  float c = TERM_R25 - TERM_R * ukty / (VCC - ukty); // koeficient c 
  float delta = a * a - 4 * b * c;
  float t = (-a + sqrt(delta)) / (2 * b);
  return t + 23;
}
