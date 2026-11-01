#pragma once
// Non-blocking reaction game. Time is injected (millis()) so it is testable and never blocks.
#include <stdint.h>

namespace weby {

enum class ReactionPhase : uint8_t { IDLE, WAITING, GO, DONE_OK, DONE_EARLY, DONE_TIMEOUT };

class ReactionGame {
 public:
  ReactionGame() : phase_(ReactionPhase::IDLE), goAt_(0), timeoutAt_(0), reactionMs_(0), bestMs_(0) {}
  // delayMs: random wait before GO (caller supplies it from the hardware RNG).
  void start(uint32_t nowMs, uint32_t delayMs, uint32_t reactTimeoutMs = 2000);
  void update(uint32_t nowMs);   // advances WAITING->GO and GO->DONE_TIMEOUT
  void press(uint32_t nowMs);    // user input (button / voice "now")
  ReactionPhase phase() const { return phase_; }
  bool finished() const { return phase_ >= ReactionPhase::DONE_OK; }
  uint32_t reactionMs() const { return reactionMs_; }
  uint32_t bestMs() const { return bestMs_; }   // 0 = no successful round yet
  void restoreBest(uint32_t ms) { bestMs_ = ms; }

 private:
  ReactionPhase phase_;
  uint32_t goAt_, timeoutAt_, reactionMs_, bestMs_;
};

}  // namespace weby
