#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

class RobotDisplay {
  public:
    bool begin();

    void showStartupFace();
    void showHappyFace();
    void showSleepyFace();
    void clear();

  private:
    Adafruit_ST7789 tft;
    void drawFace(uint16_t background, uint16_t eyeColor, bool sleepy);
};
