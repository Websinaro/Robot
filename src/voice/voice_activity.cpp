#include "voice/voice_activity.h"
#include <math.h>

namespace weby {

VoiceActivity::VoiceActivity(const VadConfig& cfg) : cfg_(cfg) { reset(); }

void VoiceActivity::reset() {
  state_ = WAITING;
  waitMs_ = loudRun_ = utterMs_ = silenceMs_ = 0;
}

uint32_t VoiceActivity::rms(const int16_t* s, size_t n) {
  if (!s || n == 0) return 0;
  double acc = 0;
  for (size_t i = 0; i < n; ++i) acc += (double)s[i] * (double)s[i];
  return (uint32_t)(sqrt(acc / (double)n) + 0.5);
}

VadEvent VoiceActivity::processFrame(const int16_t* samples, size_t n, uint32_t frameMs) {
  if (state_ == DONE) return VadEvent::NONE;
  const bool loud = rms(samples, n) >= cfg_.speechRmsThreshold;

  if (state_ == WAITING) {
    waitMs_ += frameMs;
    loudRun_ = loud ? loudRun_ + 1 : 0;
    if (loudRun_ >= cfg_.startFrames) {
      state_ = SPEECH;
      utterMs_ = loudRun_ * frameMs;
      silenceMs_ = 0;
      return VadEvent::SPEECH_STARTED;
    }
    if (waitMs_ >= cfg_.noSpeechTimeoutMs) {
      state_ = DONE;
      return VadEvent::NO_SPEECH_TIMEOUT;
    }
    return VadEvent::NONE;
  }

  // SPEECH
  utterMs_ += frameMs;
  silenceMs_ = loud ? 0 : silenceMs_ + frameMs;
  if (utterMs_ >= cfg_.maxUtteranceMs) {
    state_ = DONE;
    return VadEvent::MAX_DURATION;
  }
  if (silenceMs_ >= cfg_.endSilenceMs) {
    state_ = DONE;
    uint32_t voiced = utterMs_ > silenceMs_ ? utterMs_ - silenceMs_ : 0;
    return voiced < cfg_.minSpeechMs ? VadEvent::TOO_SHORT : VadEvent::UTTERANCE_END;
  }
  return VadEvent::NONE;
}

}  // namespace weby
