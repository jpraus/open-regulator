#ifndef VALVE_H
#define VALVE_H

#include "regulator.h"
#include "state.h"
#include "ioDriver.h"

class VALVE {
  public:
    VALVE(IO_DRIVER *io, STATE *state) : _io(*io), _state(*state) {}
    void setup();
    void fullClose();
    long move(int8_t deltaAngle);
    void update(int deltaMs);
    void stop();
    bool isIdle();

  private:
    IO_DRIVER &_io;
    STATE &_state;
  
    // state variables
    float _angle; // estimated angle value

    // movement variables
    int8_t _direction;
    long _time = 0;
};

#endif
