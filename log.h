#ifndef LOGGER_H
#define LOGGER_H

#include "Arduino.h"

#define LOG_LINES 8

#define logMessage(x) LOG::__logMessage(x)

class LOG {
  public:
    static void __logMessage(String message);
    static String logMessages[LOG_LINES];
};

#endif
