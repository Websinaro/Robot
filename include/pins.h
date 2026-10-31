#pragma once
// SINGLE source of truth for GPIO assignments (ESP32-S3-DevKitC-1 style board).
// Avoided on purpose: 0/3/45/46 (strapping), 19/20 (native USB), 26-32 (flash),
// 33-37 (octal flash/PSRAM on R8 modules), 43/44 (UART0).
// Every GPIO below is exposed on the 44-pin dual-USB-C ESP32-S3 devkit header (checked
// against the photo). NOT yet verified electrically.
#include <stdint.h>

namespace weby {
namespace pins {

// ST7789V TFT, SPI (FSPI bus) - kept from the original project
constexpr int8_t TFT_SCLK = 4;
constexpr int8_t TFT_MOSI = 5;
constexpr int8_t TFT_CS   = 6;
constexpr int8_t TFT_DC   = 7;
constexpr int8_t TFT_RST  = 15;
constexpr int8_t TFT_BL   = 1;    // -1 if the backlight is wired straight to 3V3

// INMP441 microphone (I2S RX) - Phase 2
constexpr int8_t MIC_SCK = 17;
constexpr int8_t MIC_WS  = 18;
constexpr int8_t MIC_SD  = 8;

// MAX98357A amplifier (I2S TX) - Phase 2
constexpr int8_t AMP_BCLK = 9;
constexpr int8_t AMP_LRC  = 10;
constexpr int8_t AMP_DIN  = 11;
constexpr int8_t AMP_SD   = 2;    // amp SD (shutdown/mute) pin; driven by firmware to avoid pops

// External SPI microSD module (separate bus from the display) - Phase 2
constexpr int8_t SD_SCK  = 12;
constexpr int8_t SD_MISO = 13;
constexpr int8_t SD_MOSI = 14;
constexpr int8_t SD_CS   = 21;

}  // namespace pins
}  // namespace weby
