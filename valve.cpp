#include "regulator.h"
#include "valve.h"

#define DEGREE_PER_MS 0.000704
#define MS_PER_DEGREE 1420

long VALVE::move(int8_t deltaAngle) {
  _direction = SIGNUM(deltaAngle);
  _time = MS_PER_DEGREE;
  _time *= abs(deltaAngle);
  _io.valve(_direction);
  return _time;
}

void VALVE::setup() {
  move(-90); // reset to initial full-closed position
}

void VALVE::fullClose() {
  float deltaAngle = _angle + 14; // move to initial full-closed position and fix error up to 15%
  if (deltaAngle > 90) {
    deltaAngle = 90;
  }
  move(deltaAngle * -1); // move to initial full-closed position and fix error up to 15%
}

void VALVE::update(int deltaMs) {
  if (_time > 0) {
    _time -= deltaMs;
    if (_time <= 0) { // stop condition
      _io.valve(0);
    }

    _angle += _direction * deltaMs * DEGREE_PER_MS; // rouding error will adds up here rapidly
    if (_angle < 0) {
      _angle = 0;
    }
    else if (_angle > 90) {
      _angle = 90;
    }
    _state.accumulator.valveAngle = _angle;
  }
}

void VALVE::stop() {
  _time = 0;
}

bool VALVE::isIdle() {
  return _time <= 0;
}
