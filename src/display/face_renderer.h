#pragma once
// Draws Weby's face on any Adafruit_GFX target. update() is non-blocking and
// only redraws the regions that changed (eyes / mouth) to avoid full-screen flicker.
// Phase 1 scope: static expressions, blinking, pupil drift, talking mouth.
// Smooth emotion-to-emotion tweening belongs to Phase 6 (animation engine).
#include <Adafruit_GFX.h>
#include "personality/emotions.h"

namespace weby {

class FaceRenderer {
 public:
  explicit FaceRenderer(Adafruit_GFX& gfx);

  void begin(uint32_t nowMs);
  void setEmotion(Emotion e);
  Emotion emotion() const { return emotion_; }
  void setSpeaking(bool speaking);
  void update(uint32_t nowMs);

 private:
  void redrawAll();
  void drawEye(int16_t cx, bool isLeft);
  void drawEyes();
  void drawMouth();
  int16_t eyeHeight() const;

  Adafruit_GFX& gfx_;
  Emotion emotion_;
  bool speaking_;
  bool blinkClosed_;
  bool needFull_;
  int8_t pupilX_, pupilY_;
  uint8_t talkLevel_;
  uint32_t nextBlinkMs_, blinkEndMs_, nextPupilMs_, nextTalkMs_;
};

}  // namespace weby
