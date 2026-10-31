#include "personality/emotions.h"

namespace weby {
namespace {

// Order MUST match enum class Emotion.
const EmotionProfile kProfiles[] = {
  //  name        colour  open tilt bx by  mouth                 blink wander
  {"HAPPY",     0x07FF,  90,  0,   0,  0, MouthShape::SMILE,     true,  true },
  {"EXCITED",   0xFFE0, 100,  0,   0, -2, MouthShape::BIG_SMILE, true,  true },
  {"CURIOUS",   0x07E0, 100,  0,   4, -2, MouthShape::SMALL,     true,  true },
  {"CONFUSED",  0xFD20,  85, -1,   3,  0, MouthShape::WAVY,      true,  true },
  {"SAD",       0x2D7F,  70, -2,   0,  4, MouthShape::FROWN,     true,  false},
  {"SLEEPY",    0x5ACB,  35,  0,   0,  3, MouthShape::FLAT,      true,  false},
  {"ANGRY",     0xF800,  80,  2,   0,  0, MouthShape::FROWN,     false, false},
  {"SURPRISED", 0xFFFF, 100,  0,   0,  0, MouthShape::OPEN_O,    false, false},
  {"THINKING",  0xA81F,  80,  0,   5, -4, MouthShape::SMALL,     true,  false},
  {"LISTENING", 0x07FF, 100,  0,   0,  0, MouthShape::SMALL,     true,  false},
  {"SPEAKING",  0x07FF,  95,  0,   0,  0, MouthShape::TALK,      true,  true },
  {"SEARCHING", 0xFD20,  90,  0,   0, -3, MouthShape::FLAT,      true,  true },
  {"ERROR",     0xF800,  80, -1,   0,  0, MouthShape::WAVY,      false, false},
  {"IDLE",      0x07FF,  90,  0,   0,  0, MouthShape::SMILE,     true,  true },
};

const unsigned kCount = sizeof(kProfiles) / sizeof(kProfiles[0]);

bool equalsIgnoreCase(const char* a, const char* b) {
  while (*a && *b) {
    char ca = *a, cb = *b;
    if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca + 32);
    if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb + 32);
    if (ca != cb) return false;
    ++a; ++b;
  }
  return *a == *b;
}

}  // namespace

const EmotionProfile& emotionProfile(Emotion e) {
  unsigned i = static_cast<unsigned>(e);
  return kProfiles[i < kCount ? i : static_cast<unsigned>(Emotion::IDLE)];
}

const char* emotionName(Emotion e) { return emotionProfile(e).name; }

bool emotionFromName(const char* name, Emotion& out) {
  if (!name) return false;
  for (unsigned i = 0; i < kCount; ++i) {
    if (equalsIgnoreCase(name, kProfiles[i].name)) {
      out = static_cast<Emotion>(i);
      return true;
    }
  }
  return false;
}

}  // namespace weby
