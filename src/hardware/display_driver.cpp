#include "hardware/display_driver.h"
#include "pins.h"
#include "config.h"
#include "system/log.h"

namespace weby {

DisplayDriver::DisplayDriver()
    : spi_(FSPI),
      tft_(&spi_, pins::TFT_CS, pins::TFT_DC, pins::TFT_RST),
      ready_(false) {}

void DisplayDriver::backlight(bool on) {
  if (pins::TFT_BL < 0) return;
  digitalWrite(pins::TFT_BL, on ? HIGH : LOW);
}

ErrorCode DisplayDriver::begin() {
  if (pins::TFT_BL >= 0) {
    pinMode(pins::TFT_BL, OUTPUT);
    digitalWrite(pins::TFT_BL, LOW);   // keep dark until the first frame is drawn
  }

  // SCLK, MISO (unused), MOSI, SS. CS is also driven by the Adafruit driver.
  spi_.begin(pins::TFT_SCLK, -1, pins::TFT_MOSI, pins::TFT_CS);

  // Native panel size is portrait 240x320; rotation 1 gives 320x240 landscape.
  tft_.init(config::DISPLAY_NATIVE_W, config::DISPLAY_NATIVE_H);
  tft_.setSPISpeed(config::DISPLAY_SPI_HZ);
  tft_.setRotation(config::DISPLAY_ROTATION);
  tft_.setTextWrap(false);
  tft_.fillScreen(0x0000);

  if (tft_.width() != config::DISPLAY_W || tft_.height() != config::DISPLAY_H) {
    LOGE("disp", "unexpected size %dx%d (wanted %dx%d)", tft_.width(), tft_.height(),
         config::DISPLAY_W, config::DISPLAY_H);
    return ErrorCode::DISPLAY_INIT_FAILED;
  }

  backlight(true);
  ready_ = true;
  LOGI("disp", "ST7789 ready %dx%d @ %lu Hz", tft_.width(), tft_.height(),
       (unsigned long)config::DISPLAY_SPI_HZ);
  return ErrorCode::OK;
}

}  // namespace weby
