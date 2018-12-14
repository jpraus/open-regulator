#include "log.h"

String LOG::logMessages[LOG_LINES];

void LOG::__logMessage(String message) {
  for (int i = LOG_LINES - 1; i > 0 - 1; i--) {
    logMessages[i] = logMessages[i - 1];
  }
  logMessages[0] = message;
}
