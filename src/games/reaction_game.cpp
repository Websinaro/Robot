#include "games/reaction_game.h"

namespace weby {
namespace { inline bool reached(uint32_t now, uint32_t t) { return (int32_t)(now - t) >= 0; } }

void ReactionGame::start(uint32_t now, uint32_t delayMs, uint32_t reactTimeoutMs) {
  goAt_ = now + delayMs;
  timeoutAt_ = goAt_ + reactTimeoutMs;
  reactionMs_ = 0;
  phase_ = ReactionPhase::WAITING;
}

void ReactionGame::update(uint32_t now) {
  if (phase_ == ReactionPhase::WAITING && reached(now, goAt_)) phase_ = ReactionPhase::GO;
  if (phase_ == ReactionPhase::GO && reached(now, timeoutAt_)) phase_ = ReactionPhase::DONE_TIMEOUT;
}

void ReactionGame::press(uint32_t now) {
  update(now);
  if (phase_ == ReactionPhase::WAITING) {
    phase_ = ReactionPhase::DONE_EARLY;            // false start
  } else if (phase_ == ReactionPhase::GO) {
    reactionMs_ = now - goAt_;
    phase_ = ReactionPhase::DONE_OK;
    if (bestMs_ == 0 || reactionMs_ < bestMs_) bestMs_ = reactionMs_;
  }
}

}  // namespace weby
