#include "display/face_renderer.h"
#include "config.h"
#include <math.h>
#include <Arduino.h>   // random()

namespace weby {
namespace {

// Layout for the 320x240 landscape canvas
const int16_t kEyeLX = 95, kEyeRX = 225, kEyeCY = 100;
const int16_t kEyeW = 68, kEyeMaxH = 96;
const int16_t kEyeBoxW = 84, kEyeBoxH = 116;
const int16_t kMouthCX = 160;
const int16_t kMouthBoxX = 100, kMouthBoxY = 158, kMouthBoxW = 120, kMouthBoxH = 76;
const uint16_t kBg = 0x0000, kWhite = 0xFFFF;
const int8_t kPupilRangeX = 6, kPupilRangeY = 5;

inline bool due(uint32_t now, uint32_t t) { return (int32_t)(now - t) >= 0; }

// Thick arc made of filled circles (Adafruit_GFX has no arc primitive).
// Angles in degrees, screen coordinates (y grows downward, so 0..180 is the lower half).
void arcStroke(Adafruit_GFX& g, int16_t cx, int16_t cy, int16_t r, int a0, int a1,
               int16_t thickness, uint16_t color) {
  for (int a = a0; a <= a1; a += 3) {
    float rad = (float)a * 0.0174533f;
    int16_t x = cx + (int16_t)lroundf((float)r * cosf(rad));
    int16_t y = cy + (int16_t)lroundf((float)r * sinf(rad));
    g.fillCircle(x, y, thickness / 2, color);
  }
}

// Filled lower half-disc built from horizontal lines.
void lowerHalfDisc(Adafruit_GFX& g, int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  for (int16_t dy = 0; dy <= r; ++dy) {
    int16_t half = (int16_t)lroundf(sqrtf((float)(r * r - dy * dy)));
    g.drawFastHLine(cx - half, cy + dy, 2 * half + 1, color);
  }
}

}  // namespace

FaceRenderer::FaceRenderer(Adafruit_GFX& gfx)
    : gfx_(gfx), emotion_(Emotion::IDLE), speaking_(false), blinkClosed_(false),
      needFull_(true), pupilX_(0), pupilY_(0), talkLevel_(0),
      nextBlinkMs_(0), blinkEndMs_(0), nextPupilMs_(0), nextTalkMs_(0) {}

void FaceRenderer::begin(uint32_t nowMs) {
  const EmotionProfile& p = emotionProfile(emotion_);
  pupilX_ = p.pupilBiasX;
  pupilY_ = p.pupilBiasY;
  nextBlinkMs_ = nowMs + config::BLINK_MIN_GAP_MS;
  nextPupilMs_ = nowMs + config::PUPIL_MIN_GAP_MS;
  nextTalkMs_ = nowMs;
  needFull_ = true;
  update(nowMs);
}

void FaceRenderer::setEmotion(Emotion e) {
  if (e == emotion_) return;
  emotion_ = e;
  const EmotionProfile& p = emotionProfile(e);
  pupilX_ = p.pupilBiasX;
  pupilY_ = p.pupilBiasY;
  blinkClosed_ = false;
  needFull_ = true;
}

void FaceRenderer::setSpeaking(bool speaking) {
  if (speaking == speaking_) return;
  speaking_ = speaking;
  talkLevel_ = 0;
  drawMouth();
}

int16_t FaceRenderer::eyeHeight() const {
  if (blinkClosed_) return 6;
  int16_t h = (int16_t)((int32_t)kEyeMaxH * emotionProfile(emotion_).eyeOpenPct / 100);
  return h < 8 ? 8 : h;
}

void FaceRenderer::drawEye(int16_t cx, bool isLeft) {
  const EmotionProfile& p = emotionProfile(emotion_);
  const int16_t h = eyeHeight();
  const int16_t top = kEyeCY - h / 2;

  gfx_.fillRect(cx - kEyeBoxW / 2, kEyeCY - kEyeBoxH / 2, kEyeBoxW, kEyeBoxH, kBg);

  int16_t radius = h / 2 < 24 ? h / 2 : 24;
  gfx_.fillRoundRect(cx - kEyeW / 2, top, kEyeW, h, radius, p.color);

  if (h >= 40) {
    // Highlight, drifts with the "gaze".
    gfx_.fillCircle(cx - 10 + pupilX_, kEyeCY - h / 6 + pupilY_, 9, kWhite);
    gfx_.fillCircle(cx + 12 + pupilX_ / 2, kEyeCY + h / 8 + pupilY_ / 2, 4, kWhite);

    // Eyelid for angry/sad/confused: background-coloured triangle over the top corner.
    if (p.lidTilt != 0) {
      int16_t depth = (int16_t)((p.lidTilt > 0 ? p.lidTilt : -p.lidTilt) * 10);
      int16_t outerX = isLeft ? cx - kEyeW / 2 - 1 : cx + kEyeW / 2 + 1;
      int16_t innerX = isLeft ? cx + kEyeW / 2 + 1 : cx - kEyeW / 2 - 1;
      if (p.lidTilt > 0) {   // angry: inner corner lowered
        gfx_.fillTriangle(outerX, top - 1, innerX, top - 1, innerX, top + depth, kBg);
      } else {               // sad / confused: outer corner lowered
        gfx_.fillTriangle(outerX, top + depth, outerX, top - 1, innerX, top - 1, kBg);
      }
    }
  }
}

void FaceRenderer::drawEyes() {
  drawEye(kEyeLX, true);
  drawEye(kEyeRX, false);
}

void FaceRenderer::drawMouth() {
  const EmotionProfile& p = emotionProfile(emotion_);
  const uint16_t c = p.color;
  gfx_.fillRect(kMouthBoxX, kMouthBoxY, kMouthBoxW, kMouthBoxH, kBg);

  MouthShape shape = speaking_ ? MouthShape::TALK : p.mouth;
  switch (shape) {
    case MouthShape::SMILE:
      arcStroke(gfx_, kMouthCX, 165, 38, 25, 155, 6, c);
      break;
    case MouthShape::BIG_SMILE:
      lowerHalfDisc(gfx_, kMouthCX, 172, 34, c);
      break;
    case MouthShape::FLAT:
      gfx_.fillRoundRect(kMouthCX - 22, 190, 44, 6, 3, c);
      break;
    case MouthShape::FROWN:
      arcStroke(gfx_, kMouthCX, 235, 38, 205, 335, 6, c);
      break;
    case MouthShape::OPEN_O:
      gfx_.fillCircle(kMouthCX, 192, 18, c);
      gfx_.fillCircle(kMouthCX, 192, 10, kBg);
      break;
    case MouthShape::TALK: {
      static const int16_t heights[4] = {6, 14, 24, 34};
      int16_t h = heights[talkLevel_ & 3];
      gfx_.fillRoundRect(kMouthCX - 26, 172, 52, h, h / 2 < 10 ? h / 2 : 10, c);
      break;
    }
    case MouthShape::WAVY: {
      int16_t x = kMouthCX - 36;
      int16_t y = 192;
      for (int i = 0; i < 6; ++i) {
        int16_t nx = x + 12;
        int16_t ny = (i & 1) ? 192 : 184;
        gfx_.drawLine(x, y, nx, ny, c);
        gfx_.drawLine(x, y + 1, nx, ny + 1, c);
        gfx_.drawLine(x, y + 2, nx, ny + 2, c);
        x = nx;
        y = ny;
      }
      break;
    }
    case MouthShape::SMALL:
      gfx_.fillRoundRect(kMouthCX - 10, 190, 20, 6, 3, c);
      break;
    default:
      break;
  }
}

void FaceRenderer::redrawAll() {
  gfx_.fillScreen(kBg);
  drawEyes();
  drawMouth();
  needFull_ = false;
}

void FaceRenderer::update(uint32_t nowMs) {
  if (needFull_) {
    redrawAll();
    // Re-arm timers relative to now so a long previous expression doesn't cause a burst.
    nextBlinkMs_ = nowMs + config::BLINK_MIN_GAP_MS + (uint32_t)random(config::BLINK_GAP_JITTER_MS);
    nextPupilMs_ = nowMs + config::PUPIL_MIN_GAP_MS;
    return;
  }

  const EmotionProfile& p = emotionProfile(emotion_);

  // Blink
  if (p.canBlink) {
    if (!blinkClosed_ && due(nowMs, nextBlinkMs_)) {
      blinkClosed_ = true;
      blinkEndMs_ = nowMs + config::BLINK_DURATION_MS;
      drawEyes();
    } else if (blinkClosed_ && due(nowMs, blinkEndMs_)) {
      blinkClosed_ = false;
      nextBlinkMs_ = nowMs + config::BLINK_MIN_GAP_MS + (uint32_t)random(config::BLINK_GAP_JITTER_MS);
      drawEyes();
    }
  }

  // Gaze drift
  if (p.wander && !blinkClosed_ && due(nowMs, nextPupilMs_)) {
    pupilX_ = (int8_t)(p.pupilBiasX + (int8_t)random(-kPupilRangeX, kPupilRangeX + 1));
    pupilY_ = (int8_t)(p.pupilBiasY + (int8_t)random(-kPupilRangeY, kPupilRangeY + 1));
    nextPupilMs_ = nowMs + config::PUPIL_MIN_GAP_MS + (uint32_t)random(config::PUPIL_GAP_JITTER_MS);
    drawEyes();
  }

  // Talking mouth
  if (speaking_ && due(nowMs, nextTalkMs_)) {
    talkLevel_ = (uint8_t)random(4);
    nextTalkMs_ = nowMs + config::TALK_FRAME_MS;
    drawMouth();
  }
}

}  // namespace weby
