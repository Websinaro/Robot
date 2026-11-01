#pragma once
// Energy-based utterance endpointing. Thresholds are STARTING VALUES and MUST be
// tuned on the real INMP441 (room noise, gain) - see docs/HARDWARE_VERIFICATION.md.
#include <stddef.h>
#include <stdint.h>

namespace weby {

struct VadConfig {
  uint32_t speechRmsThreshold;   // RMS (int16 units) that counts as speech
  uint16_t startFrames;          // consecutive loud frames to declare speech start
  uint16_t endSilenceMs;         // trailing silence that ends the utterance
  uint16_t minSpeechMs;          // shorter voiced audio is rejected as noise
  uint32_t noSpeechTimeoutMs;    // give up if nobody speaks
  uint32_t maxUtteranceMs;       // hard cap on one utterance
  VadConfig() : speechRmsThreshold(600), startFrames(3), endSilenceMs(800),
                minSpeechMs(200), noSpeechTimeoutMs(5000), maxUtteranceMs(15000) {}
};

enum class VadEvent : uint8_t {
  NONE, SPEECH_STARTED, UTTERANCE_END, NO_SPEECH_TIMEOUT, MAX_DURATION, TOO_SHORT
};

class VoiceActivity {
 public:
  explicit VoiceActivity(const VadConfig& cfg = VadConfig());
  void reset();
  // Feed one frame; frameMs is its duration. After a terminal event, returns NONE until reset().
  VadEvent processFrame(const int16_t* samples, size_t n, uint32_t frameMs);
  bool inSpeech() const { return state_ == SPEECH; }
  bool finished() const { return state_ == DONE; }
  static uint32_t rms(const int16_t* samples, size_t n);

 private:
  enum State { WAITING, SPEECH, DONE };
  VadConfig cfg_;
  State state_;
  uint32_t waitMs_, loudRun_, utterMs_, silenceMs_;
};

}  // namespace weby
