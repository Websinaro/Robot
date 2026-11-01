#pragma once
#include <stdint.h>

namespace weby {

enum class GuessResult : uint8_t { TOO_LOW, TOO_HIGH, CORRECT, OUT_OF_RANGE, GAME_OVER, NOT_STARTED };
enum class GamePhase : uint8_t { IDLE, WAITING_FOR_INPUT, FINISHED_WIN, FINISHED_LOSE };

class NumberGame {
 public:
  NumberGame() : phase_(GamePhase::IDLE), min_(1), max_(10), secret_(1), maxTries_(0), used_(0) {}
  // The secret is supplied by the caller (from a hardware RNG) so tests are deterministic.
  void start(int minValue, int maxValue, int secret, uint8_t maxTries);
  GuessResult guess(int value);     // OUT_OF_RANGE does not consume a try
  GamePhase phase() const { return phase_; }
  bool active() const { return phase_ == GamePhase::WAITING_FOR_INPUT; }
  uint8_t triesUsed() const { return used_; }
  uint8_t triesLeft() const { return (uint8_t)(maxTries_ - used_); }
  int minValue() const { return min_; }
  int maxValue() const { return max_; }
  // Only reveals the secret once the game is finished.
  bool revealSecret(int& out) const;

  // Extracts the first run of digits from a transcript ("is it 7?" -> 7). Digits only:
  // spelled-out numbers depend on server STT normalisation. Returns false if none / too long.
  static bool parseNumber(const char* text, int& out);

 private:
  GamePhase phase_;
  int min_, max_, secret_;
  uint8_t maxTries_, used_;
};

}  // namespace weby
