#ifndef IO_DRIVERS_H
#define IO_DRIVERS_H

#include "regulator.h"
//#include <Encoder.h>

class IO_DRIVER {
  public:
    void setup();
    void valve(int8_t direction);
    void chPump(bool on);
    void dhwPump(bool on);

    float readPressure();
    float readTemp(const byte which); // 1-6

  private:
    static float _analogKtyToCelsius(float analogValue);
    static void _rotaryEncoderButtonISR();
    static void _rotaryEncoderAISR();
    static void _rotaryEncoderBISR();
};

#endif
