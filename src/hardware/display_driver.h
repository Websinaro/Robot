#pragma once
// ST7789V 240x320 TFT on its own SPI bus, used in landscape (320x240).
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "error_codes.h"

namespace weby {

class DisplayDriver {
 public:
  DisplayDriver();
  ErrorCode begin();
  bool ready() const { return ready_; }
  Adafruit_GFX& gfx() { return tft_; }
  void backlight(bool on);

 private:
  SPIClass spi_;          // must be declared before tft_ (tft_ keeps a pointer to it)
  Adafruit_ST7789 tft_;
  bool ready_;
};

}  // namespace weby
