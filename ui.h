#ifndef UI_H
#define UI_H

#include <U8g2lib.h>
#include "state.h"

class UI {
  public:
    UI(U8G2_ST7920_128X64_1_SW_SPI *lcd, STATE *state, byte backlightPin);
    void update();
    void setup();
    void draw();

  private:
    U8G2_ST7920_128X64_1_SW_SPI &_lcd;
    STATE &_state;
    byte _backlightPin;
    byte _frame = 0;

    void drawLogScreen();
    void drawSchemaScreen();

    void drawAccumulator(u8g2_uint_t x, u8g2_uint_t y);
    void drawBoiler(u8g2_uint_t x, u8g2_uint_t y);
    void drawTemperature(u8g2_uint_t x, u8g2_uint_t y, int temp);
    void drawPressure(u8g2_uint_t x, u8g2_uint_t y, float pressure);
    void drawPercents(u8g2_uint_t x, u8g2_uint_t y, byte percents, byte alignOffset);
    void drawTime(u8g2_uint_t x, u8g2_uint_t y, unsigned long ms);
    void drawCentralHeating(u8g2_uint_t x, u8g2_uint_t y, int temp, int setpoint, bool on);
    void drawHotWater(u8g2_uint_t x, u8g2_uint_t y, int temp, int setpoint, bool on);
    void drawHLineWithArrow(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, bool pointingLeft);
    void drawPump(u8g2_uint_t x, u8g2_uint_t y, boolean on);
    void drawFlame(u8g2_uint_t x, u8g2_uint_t y);
    void drawFlame0(u8g2_uint_t x, u8g2_uint_t y);
    void drawFlame1(u8g2_uint_t x, u8g2_uint_t y);
};

#endif
