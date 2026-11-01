#pragma once
#include <stdint.h>

// microSD is OPTIONAL: set -DWEBY_HAS_SD=1 only after the card module is wired and verified.
#ifndef WEBY_HAS_SD
#define WEBY_HAS_SD 0
#endif

namespace weby {
namespace config {

// Display: ST7789V native 240x320, used in landscape (rotation 1) = 320x240
constexpr int16_t  DISPLAY_NATIVE_W = 240;
constexpr int16_t  DISPLAY_NATIVE_H = 320;
constexpr int16_t  DISPLAY_W        = 320;
constexpr int16_t  DISPLAY_H        = 240;
constexpr uint8_t  DISPLAY_ROTATION = 1;
constexpr uint32_t DISPLAY_SPI_HZ   = 40000000UL;

// Face timing (ms)
constexpr uint32_t BLINK_DURATION_MS    = 110;
constexpr uint32_t BLINK_MIN_GAP_MS     = 2500;
constexpr uint32_t BLINK_GAP_JITTER_MS  = 3500;
constexpr uint32_t PUPIL_MIN_GAP_MS     = 1200;
constexpr uint32_t PUPIL_GAP_JITTER_MS  = 2800;
constexpr uint32_t TALK_FRAME_MS        = 130;

// Bring-up: how long each emotion is shown in the hardware test cycle
constexpr uint32_t BRINGUP_EMOTION_MS = 3000;

}  // namespace config
}  // namespace weby
