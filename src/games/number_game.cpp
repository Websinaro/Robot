#include "games/number_game.h"

namespace weby {

void NumberGame::start(int lo, int hi, int secret, uint8_t maxTries) {
  if (hi < lo) { int t = lo; lo = hi; hi = t; }
  min_ = lo; max_ = hi;
  secret_ = secret < lo ? lo : (secret > hi ? hi : secret);
  maxTries_ = maxTries ? maxTries : 1;
  used_ = 0;
  phase_ = GamePhase::WAITING_FOR_INPUT;
}

GuessResult NumberGame::guess(int v) {
  if (phase_ == GamePhase::IDLE) return GuessResult::NOT_STARTED;
  if (phase_ != GamePhase::WAITING_FOR_INPUT) return GuessResult::GAME_OVER;
  if (v < min_ || v > max_) return GuessResult::OUT_OF_RANGE;
  ++used_;
  if (v == secret_) { phase_ = GamePhase::FINISHED_WIN; return GuessResult::CORRECT; }
  if (used_ >= maxTries_) phase_ = GamePhase::FINISHED_LOSE;
  return v < secret_ ? GuessResult::TOO_LOW : GuessResult::TOO_HIGH;
}

bool NumberGame::revealSecret(int& out) const {
  if (phase_ == GamePhase::FINISHED_WIN || phase_ == GamePhase::FINISHED_LOSE) { out = secret_; return true; }
  return false;
}

bool NumberGame::parseNumber(const char* t, int& out) {
  if (!t) return false;
  while (*t && !(*t >= '0' && *t <= '9')) ++t;
  if (!*t) return false;
  long v = 0; int digits = 0;
  while (*t >= '0' && *t <= '9') {
    if (++digits > 6) return false;
    v = v * 10 + (*t - '0');
    ++t;
  }
  out = (int)v;
  return true;
}

}  // namespace weby
