#ifndef REGULATOR_H
#define REGULATOR_H

#include "Arduino.h"
#include "log.h"

// constants
#define VCC 6.3 // napajeci napeti delice
#define DEBUG 1
#define SIGNUM(x) (x >= 0 ? 1 : -1)

// definitions of I/O pins
#define ACTY_LED 53

// display 128x64 (radic ST7920)
#define DSPY_BCKLIGHT 28
#define DSPY_EN 29
#define DSPY_RW 30
#define DSPY_RS 31

// OpenTherm Boiler/Thermostat protocol
#define THERMOSTAT_IN 20
#define THERMOSTAT_OUT 22
#define BOILER_IN 21
#define BOILER_OUT 23

// analog pressure sensor
#define PIN_PRESSURE A0

// NTC temperature sensors
#define PIN_T1 A1 // built-in temp
#define PIN_T2 A2
#define PIN_T3 A3
#define PIN_T4 A4
#define PIN_T5 A5
#define PIN_T6 A6

// relays
#define PIN_RELAY1 36
#define PIN_RELAY2 35
#define PIN_RELAY3 34
#define PIN_RELAY4 33 // valve triac 1
#define PIN_RELAY5 32 // valve triac 2

// rotary encoder
#define PIN_ROTARY_ENC_1 7
#define PIN_ROTARY_ENC_2 6
#define PIN_ROTARY_ENC_BUTTON 5

#endif
