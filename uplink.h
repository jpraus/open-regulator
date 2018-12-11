#ifndef UPLINK_H
#define UPLINK_H

#include "regulator.h"
#include "state.h"
#include <Ethernet.h>

class UPLINK {
  public:
    UPLINK(STATE *state) : _state(*state) {}
    void setup();
    void update(int deltaMs);

  private:
    EthernetClient _client;
    STATE &_state;
    long _timer;
    boolean _awaitingResponse;

    bool _send();
    String _createStateData();
    void _printJsonFloat(String &buffer, float value);
    void _printJsonBool(String &buffer, bool value);
};

#endif
