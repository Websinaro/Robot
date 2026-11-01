#include "games/rock_paper_scissors.h"

namespace weby {

RpsOutcome rpsJudge(RpsMove p, RpsMove r) {
  if (p == r) return RpsOutcome::DRAW;
  if (p == RpsMove::NONE) return RpsOutcome::LOSE;
  if (r == RpsMove::NONE) return RpsOutcome::WIN;
  if ((p == RpsMove::ROCK && r == RpsMove::SCISSORS) ||
      (p == RpsMove::PAPER && r == RpsMove::ROCK) ||
      (p == RpsMove::SCISSORS && r == RpsMove::PAPER))
    return RpsOutcome::WIN;
  return RpsOutcome::LOSE;
}

const char* rpsName(RpsMove m) {
  switch (m) {
    case RpsMove::ROCK: return "rock";
    case RpsMove::PAPER: return "paper";
    case RpsMove::SCISSORS: return "scissors";
    default: return "none";
  }
}

namespace {
bool isLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

bool wordEquals(const char* w, unsigned len, const char* lit) {
  unsigned i = 0;
  for (; i < len && lit[i]; ++i)
    if (lower(w[i]) != lit[i]) return false;
  return i == len && lit[i] == '\0';
}
}  // namespace

RpsMove rpsParse(const char* t) {
  if (!t) return RpsMove::NONE;
  RpsMove found = RpsMove::NONE;
  const char* p = t;
  while (*p) {
    while (*p && !isLetter(*p)) ++p;
    const char* start = p;
    while (*p && isLetter(*p)) ++p;
    unsigned len = (unsigned)(p - start);
    if (!len) continue;
    RpsMove m = RpsMove::NONE;
    if (wordEquals(start, len, "rock")) m = RpsMove::ROCK;
    else if (wordEquals(start, len, "paper")) m = RpsMove::PAPER;
    else if (wordEquals(start, len, "scissors") || wordEquals(start, len, "scissor")) m = RpsMove::SCISSORS;
    if (m != RpsMove::NONE) {
      if (found != RpsMove::NONE && found != m) return RpsMove::NONE;  // ambiguous
      found = m;
    }
  }
  return found;
}

void RpsScore::record(RpsOutcome o) {
  switch (o) {
    case RpsOutcome::WIN: ++wins; break;
    case RpsOutcome::LOSE: ++losses; break;
    default: ++draws; break;
  }
}

}  // namespace weby
