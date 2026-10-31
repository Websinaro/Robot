#pragma once
// Emotion definitions. Pure data, no hardware dependencies.
#include <stdint.h>

namespace weby {

enum class Emotion : uint8_t {
  HAPPY, EXCITED, CURIOUS, CONFUSED, SAD, SLEEPY, ANGRY, SURPRISED,
  THINKING, LISTENING, SPEAKING, SEARCHING, ERROR, IDLE,
  COUNT
};

enum class MouthShape : uint8_t {
  SMILE, BIG_SMILE, FLAT, FROWN, OPEN_O, TALK, WAVY, SMALL, COUNT
};

struct EmotionProfile {
  const char* name;
  uint16_t color;        // RGB565 eye/mouth colour
  uint8_t eyeOpenPct;    // 10..100
  int8_t lidTilt;        // >0 angry (inner lid lowered), <0 sad (outer lid lowered), 0 none
  int8_t pupilBiasX;     // resting highlight offset, pixels
  int8_t pupilBiasY;
  MouthShape mouth;
  bool canBlink;
  bool wander;           // pupils drift when idle
};

const EmotionProfile& emotionProfile(Emotion e);
const char* emotionName(Emotion e);
// Case-insensitive lookup, used for server-provided emotion identifiers.
// Returns false (and leaves `out` untouched) for unknown names.
bool emotionFromName(const char* name, Emotion& out);

}  // namespace weby
