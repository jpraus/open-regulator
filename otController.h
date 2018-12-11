#ifndef OT_CONTROLLER_H
#define OT_CONTROLLER_H

#include "regulator.h"
#include "opentherm.h"
#include "state.h"
#include "accumulatorCtrl.h"

class OT_CONTROLLER {
  public:
    OT_CONTROLLER(STATE *state, ACCUMULATOR_CTRL *accumulatorCtrl, byte pinMasterIn, byte pinMasterOut, byte pinSlaveIn, byte pinSlaveOut);
    void setup();
    void update(int deltaMs);

  private:
    STATE &_state;
    ACCUMULATOR_CTRL &_accumulatorCtrl;
  
    OpenthermData _request;
    OpenthermData _response;
    bool _responseOverridden = false; // user provided response to master instead of received from slave
    
    byte _pinMasterIn;
    byte _pinMasterOut;
    byte _pinSlaveIn;
    byte _pinSlaveOut;

    byte _mode;
    byte _phase;

    void _interceptRequest();
    void _interceptResponse();
    void _onRequestTimeout();

    void _logRequestToSerial();
    void _logResponseToSerial();
};

#endif
