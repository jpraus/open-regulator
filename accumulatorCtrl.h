#ifndef ACCUMULATOR_CTRL_H
#define ACCUMULATOR_CTRL_H

#include "regulator.h"
#include "state.h"
#include "ioDriver.h"
#include "valve.h"

class ACCUMULATOR_CTRL {
  public:
    ACCUMULATOR_CTRL(IO_DRIVER *io, STATE *state) : 
      _io(*io), 
      _state(*state), 
      _valve(io, state) {}
    void setup();
    void update(int deltaMs);

    byte getOverrideOTFlags();

  private:
    IO_DRIVER &_io;
    STATE &_state;
    VALVE _valve;
    byte _mode;
    long _timer; // multi-purpose time count-down

    void _updateState();
    void _updateDhw(int deltaMs);
    void _updateCh(int deltaMs);
    void _updateAntiStop(int deltaMs);
    void _changeMode(byte mode);

    // valve regulation
    void _regulateValve(int deltaMs);
};

#endif
