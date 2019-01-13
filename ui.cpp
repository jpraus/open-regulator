#include "regulator.h"
#include "log.h"
#include "ui.h"
#include <U8g2lib.h>

#define ALIGN_LEFT 0
#define ALIGN_CENTER 2
#define ALIGN_RIGHT 5

UI::UI(U8G2_ST7920_128X64_1_SW_SPI *lcd, STATE *state, byte backlightPin) 
    : _lcd(*lcd), _state(*state), _backlightPin(backlightPin) {
}

void UI::setup() {
  pinMode(_backlightPin, OUTPUT);
  digitalWrite(_backlightPin, OUTPUT);
  _lcd.begin();
  _lcd.setColorIndex(1);
}

void UI::update() {
  _frame = (_frame >= 1) ? 0 : _frame + 1;
  _lcd.firstPage();

  do {
    drawLogScreen();
    //drawSchemaScreen();
  } while (_lcd.nextPage());
}

void UI::drawLogScreen() {
  _lcd.setFont(u8g2_font_5x7_mf);
  _lcd.setColorIndex(1);

  for (int i = 0; i < LOG_LINES; i ++) {
    _lcd.setCursor(0, i * 7 + 7); 
    _lcd.print(LOG::logMessages[i]);
  }
}

void UI::drawSchemaScreen() {
  _lcd.setFont(u8g2_font_5x8_mf);
  _lcd.setColorIndex(1);

  drawBoiler(0, 0);
  drawAccumulator(96, 0);

  float chTemp = _state.accumulator.chOn ? _state.accumulator.chTemp : (_state.boiler.chOn ? _state.boiler.feedTemp : 0);
  drawHotWater(57, 4, _state.thermostat.dhwTemp, _state.thermostat.dhwSetpoint, _state.accumulator.dhwOn || _state.boiler.dhwOn);
  drawCentralHeating(57, 36, chTemp, _state.thermostat.chSetpoint, _state.thermostat.chOn);

  if (_state.accumulator.dhwOn) {
    drawHLineWithArrow(77, 17, 18, true); // accumulator -> hot water
  }
  if (_state.accumulator.chOn) { 
    drawPercents(75, 44, _state.accumulator.valveAngle / 0.9, ALIGN_RIGHT); // convert 0-90 angle to percents
    _lcd.drawVLine(85, 47, 5); // valve symbol
    _lcd.drawVLine(86, 48, 3);
    _lcd.drawVLine(88, 48, 3);
    _lcd.drawVLine(89, 47, 5);
    drawHLineWithArrow(77, 49, 18, true); // accumulator -> central heating
  }

  if (_state.boiler.dhwOn) {
    drawHLineWithArrow(33, 17, 18, false); // boiler -> hot water
  }
  if (_state.boiler.chOn) {
    drawHLineWithArrow(33, 49, 18, false); // boiler -> central heating
  }

  if (_state.thermostat.online > 0) {
    _lcd.drawRBox(32, 1, 12, 8, 2); // OT+ indicator
    _lcd.setColorIndex(0);
    _lcd.drawStr(33, 8, "0T");
    _lcd.setColorIndex(1);
  }
  if (_state.accumulator.mode >= 10 && _frame == 0) { // AntiStop mode
    _lcd.drawRBox(45, 1, 13, 8, 2); // Anti stop protection is on
    _lcd.setColorIndex(0);
    _lcd.drawStr(47, 8, "AS");
    _lcd.setColorIndex(1);
  }
//  if (true) {
//    _lcd.drawRBox(84, 1, 13, 8, 2); // ETH indicator
//    _lcd.setColorIndex(0);
//    _lcd.drawStr(86, 8, "UP");
//    _lcd.setColorIndex(1);
//  }

  drawTime(73, 10, _state.accumulator.dhwTotalMs + _state.boiler.dhwTotalMs);
  drawTime(73, 42, _state.accumulator.chTotalMs + _state.boiler.chTotalMs);
}

void UI::drawBoiler(u8g2_uint_t x, u8g2_uint_t y) {
  _lcd.drawStr(x + 4, y + 8, "KOTEL");
  _lcd.drawRFrame(x, y + 10, 32, 54, 3);
  
  if (_state.boiler.flameOn) {
    drawTemperature(x + 9, y + 20, _state.boiler.feedTemp);
    drawFlame(x + 12 ,y + 30);
    drawPercents(x + 7, y + 49, _state.boiler.modulationLvl, ALIGN_CENTER);
    drawTemperature(x + 9, y + 60, _state.boiler.returnTemp);
  }
  else {
    _lcd.drawStr(x + 9, y + 20, "OFF");
  }
}

void UI::drawAccumulator(u8g2_uint_t x, u8g2_uint_t y) {
  _lcd.drawStr(x + 9, y + 8, "AKU");
  _lcd.drawRFrame(x, y + 10, 32, 54, 3);
  
  drawTemperature(x + 9, y + 20, _state.accumulator.topTemp);
  drawTemperature(x + 9, y + 60, _state.accumulator.bottomTemp);

  if (_frame == 0 || _state.accumulator.pressure <= 2.5) {
    _lcd.drawBox(x + 2, y + 23, 28, 10);
    _lcd.setColorIndex(0);
  }
  drawPressure(x + 6, y + 31, _state.accumulator.pressure);
  _lcd.setColorIndex(1);

  float storedAvgTemp = (_state.accumulator.topTemp + _state.accumulator.bottomTemp) / 2.0; // prumer dolni a horni teploty v zasobniku
  float kJ = 2090.0 * (storedAvgTemp - _state.thermostat.roomTemp); // kJ tepla ulozeneho v zasobniku [c * m * (t2 - t1)] - c * m = 4.18 * 500 = 2090;
  if (kJ >= 0) {
    _lcd.setCursor(x + 4, y + 41);
    _lcd.print(round(kJ * 0.00028));
    _lcd.print("kWh");
    _lcd.setCursor(x + 8, y + 49);
    _lcd.print("~");
    _lcd.print(round(kJ * 0.000023)); // 12kW / h
    _lcd.print("h");
  }
}

void UI::drawTemperature(u8g2_uint_t x, u8g2_uint_t y, int temp) {
  _lcd.setCursor(x, y);
  if (temp < 10) {
     _lcd.print("--");
  }
  else if (temp >= 100) {
     _lcd.print("99");
  }
  else {
    _lcd.print(temp);
  }
  _lcd.print("C");
}

void UI::drawPressure(u8g2_uint_t x, u8g2_uint_t y, float pressure) {
  char buffer[4];
  _lcd.setCursor(x, y);
  if (pressure <= 0) {
     _lcd.print("---");
  }
  else {
    dtostrf(pressure, 1, 1, buffer);
    _lcd.print(buffer);
  }
  _lcd.print("b");
}

void UI::drawPercents(u8g2_uint_t x, u8g2_uint_t y, byte percents, byte alignOffset) {
  if (percents < 100) {
    x += alignOffset;
  }
  if (percents < 10) {
    x += alignOffset;
  }
  _lcd.setCursor(x, y);
  if (percents >= 0) {
    _lcd.print(percents);
    _lcd.print("%");
  }
  else {
    _lcd.print("--");
  }
}

void UI::drawTime(u8g2_uint_t x, u8g2_uint_t y, unsigned long ms) {
  _lcd.setCursor(x, y);
  
  long sec = round(ms / 1000.0);
  if (sec > 86400) { // more then 1 day
    _lcd.print((int) floor(sec / 86400.0));
    _lcd.print("d");
  }
  else if (sec > 3600) { // more then 1 hour
    _lcd.print((int) floor(sec / 3600.0));
    _lcd.print("h");
  }
  else if (sec > 60) { // more then 1 minute
    _lcd.print((int) floor(sec / 60.0));
    _lcd.print("m");
  }
  else {
    _lcd.print(sec);
    _lcd.print("s");
  }
}

void UI::drawCentralHeating(u8g2_uint_t x, u8g2_uint_t y, int temp, int setpoint, bool on) {
  if (!on || _frame == 1) {
    _lcd.drawHLine(x + 2, y + 1, 10);
    _lcd.drawHLine(x + 2, y + 5, 10);
    _lcd.drawVLine(x + 3, y, 7);
    _lcd.drawVLine(x + 4, y, 7);
    _lcd.drawVLine(x + 6, y, 7);
    _lcd.drawVLine(x + 7, y, 7);
    _lcd.drawVLine(x + 9, y, 7);
    _lcd.drawVLine(x + 10, y, 7);
  }

  drawTemperature(x, y + 16, temp);
  drawTemperature(x, y + 24, setpoint);
  _lcd.drawStr(x - 5, y + 24, "(");
  _lcd.drawStr(x + 15, y + 24, ")");
}

void UI::drawHotWater(u8g2_uint_t x, u8g2_uint_t y, int temp, int setpoint, bool on) {
  if (!on || _frame == 1) {
    _lcd.drawHLine(x + 6, y, 5);
    _lcd.drawPixel(x + 8, y + 1);
    _lcd.drawVLine(x + 3, y + 1, 5);
    _lcd.drawHLine(x + 4, y + 2, 6);
    _lcd.drawHLine(x + 4, y + 3,7);
    _lcd.drawHLine(x + 4, y + 4, 7);
    _lcd.drawHLine(x + 8, y + 5, 3);
    _lcd.drawHLine(x + 8, y + 6, 3);
  }
   
  drawTemperature(x, y + 16, temp);
  drawTemperature(x, y + 24, setpoint);
  _lcd.drawStr(x - 5, y + 24, "(");
  _lcd.drawStr(x + 15, y + 24, ")");
}

void UI::drawPump(u8g2_uint_t x, u8g2_uint_t y, boolean on) {
  _lcd.drawCircle(x + 4, y + 4, 4);
  if (on && _frame == 1) {
    _lcd.drawVLine(x + 3, y + 2, 5);
    _lcd.drawVLine(x + 4, y + 2, 5);
    _lcd.drawVLine(x + 5, y + 3, 3);
    _lcd.drawPixel(x + 6, y + 4);
  }
}

void UI::drawHLineWithArrow(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, bool pointingLeft) {
  _lcd.drawHLine(x, y, len);
  if (pointingLeft) {
    _lcd.drawVLine(x + 1, y - 1, 3);
    _lcd.drawVLine(x + 2, y - 2, 5);
  }
  else {
    _lcd.drawVLine(x + len - 2, y - 1, 3);
    _lcd.drawVLine(x + len - 3, y - 2, 5);
  }
}

void UI::drawFlame(u8g2_uint_t x, u8g2_uint_t y) {
  if (_frame == 0) {
    drawFlame0(x, y);
  } else if (_frame == 1) {
    drawFlame1(x, y);
  }
}

void UI::drawFlame0(u8g2_uint_t x, u8g2_uint_t y) {
  _lcd.drawPixel(x + 4, y);
  _lcd.drawHLine(x + 3, y + 1, 2);
  _lcd.drawHLine(x + 3, y + 2, 3);
  _lcd.drawHLine(x + 4, y + 3, 3);
  _lcd.drawHLine(x + 4, y + 4, 4);
  _lcd.drawHLine(x + 3, y + 5, 5);
  _lcd.drawHLine(x + 2, y + 6, 2); _lcd.drawHLine(x + 5, y + 6, 2);
  _lcd.drawHLine(x + 1, y + 7, 2); _lcd.drawHLine(x + 4, y + 7, 2);
  _lcd.drawHLine(x, y + 8, 3); _lcd.drawHLine(x + 4, y + 8, 3);
  _lcd.drawHLine(x, y + 9, 3); _lcd.drawHLine(x + 5, y + 9, 2);
  _lcd.drawHLine(x + 1, y + 10, 2); _lcd.drawHLine(x + 5, y + 10, 2);
}

void UI::drawFlame1(u8g2_uint_t x, u8g2_uint_t y) {
  _lcd.drawPixel(x + 3, y);
  _lcd.drawHLine(x + 3, y + 1, 2);
  _lcd.drawHLine(x + 2, y + 2, 3);
  _lcd.drawHLine(x + 1, y + 3, 3);
  _lcd.drawHLine(x, y + 4, 4);
  _lcd.drawHLine(x, y + 5, 5);
  _lcd.drawHLine(x + 1, y + 6, 2); _lcd.drawHLine(x + 4, y + 6, 2);
  _lcd.drawHLine(x + 2, y + 7, 2); _lcd.drawHLine(x + 5, y + 7, 2);
  _lcd.drawHLine(x + 1, y + 8, 3); _lcd.drawHLine(x + 5, y + 8, 3);
  _lcd.drawHLine(x + 1, y + 9, 2); _lcd.drawHLine(x + 5, y + 9, 3);
  _lcd.drawHLine(x + 1, y + 10, 2); _lcd.drawHLine(x + 5, y + 10, 2);
}
