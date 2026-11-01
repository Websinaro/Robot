#pragma once
#include <stdint.h>

namespace weby {

enum class RpsMove : uint8_t { ROCK, PAPER, SCISSORS, NONE };
enum class RpsOutcome : uint8_t { WIN, LOSE, DRAW };   // from the PLAYER's point of view

RpsOutcome rpsJudge(RpsMove player, RpsMove robot);     // NONE is treated as a loss for that side's owner
// Whole-word, case-insensitive match in a (server) transcript. Returns NONE if no move
// word is present or if the text names more than one different move.
RpsMove rpsParse(const char* transcript);
inline RpsMove rpsFromRandom(uint32_t r) { return static_cast<RpsMove>(r % 3); }
const char* rpsName(RpsMove m);

struct RpsScore {
  uint16_t wins, losses, draws;
  RpsScore() : wins(0), losses(0), draws(0) {}
  void record(RpsOutcome o);
  uint16_t rounds() const { return (uint16_t)(wins + losses + draws); }
};

}  // namespace weby
