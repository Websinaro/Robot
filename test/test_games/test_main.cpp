#include <unity.h>
#include "games/rock_paper_scissors.h"
#include "games/number_game.h"
#include "games/reaction_game.h"

using namespace weby;
void setUp(void) {}
void tearDown(void) {}

#define EQ(e, a) TEST_ASSERT_EQUAL_INT((int)(e), (int)(a))

void test_rps_full_truth_table() {
  const RpsMove m[3] = {RpsMove::ROCK, RpsMove::PAPER, RpsMove::SCISSORS};
  // rows: player, cols: robot. WIN=0 LOSE=1 DRAW=2
  const RpsOutcome exp[3][3] = {
    {RpsOutcome::DRAW, RpsOutcome::LOSE, RpsOutcome::WIN},
    {RpsOutcome::WIN,  RpsOutcome::DRAW, RpsOutcome::LOSE},
    {RpsOutcome::LOSE, RpsOutcome::WIN,  RpsOutcome::DRAW}};
  for (int p = 0; p < 3; ++p) for (int r = 0; r < 3; ++r) EQ(exp[p][r], rpsJudge(m[p], m[r]));
}

void test_rps_parse() {
  EQ(RpsMove::ROCK, rpsParse("I choose ROCK!"));
  EQ(RpsMove::SCISSORS, rpsParse("scissor"));
  EQ(RpsMove::PAPER, rpsParse("paper, paper"));
  EQ(RpsMove::NONE, rpsParse("rock or paper"));       // ambiguous
  EQ(RpsMove::NONE, rpsParse("rocket"));              // whole words only
  EQ(RpsMove::NONE, rpsParse(""));
  EQ(RpsMove::NONE, rpsParse(0));
}

void test_rps_random_mapping_and_score() {
  EQ(RpsMove::ROCK, rpsFromRandom(3)); EQ(RpsMove::PAPER, rpsFromRandom(4)); EQ(RpsMove::SCISSORS, rpsFromRandom(5));
  RpsScore s; s.record(RpsOutcome::WIN); s.record(RpsOutcome::WIN); s.record(RpsOutcome::DRAW); s.record(RpsOutcome::LOSE);
  EQ(2, s.wins); EQ(1, s.losses); EQ(1, s.draws); EQ(4, s.rounds());
}

void test_number_game_flow_win() {
  NumberGame g;
  EQ(GuessResult::NOT_STARTED, g.guess(5));
  g.start(1, 10, 7, 3);
  TEST_ASSERT_TRUE(g.active());
  EQ(GuessResult::TOO_LOW, g.guess(3));
  EQ(GuessResult::TOO_HIGH, g.guess(9));
  EQ(GuessResult::CORRECT, g.guess(7));
  EQ(GamePhase::FINISHED_WIN, g.phase());
  EQ(GuessResult::GAME_OVER, g.guess(7));
}

void test_number_game_out_of_range_costs_nothing_and_loss_reveals() {
  NumberGame g; g.start(1, 10, 7, 2);
  int s = 0;
  TEST_ASSERT_FALSE(g.revealSecret(s));               // no peeking mid-game
  EQ(GuessResult::OUT_OF_RANGE, g.guess(11));
  EQ(GuessResult::OUT_OF_RANGE, g.guess(0));
  EQ(0, g.triesUsed());
  g.guess(1); g.guess(2);
  EQ(GamePhase::FINISHED_LOSE, g.phase());
  TEST_ASSERT_TRUE(g.revealSecret(s)); EQ(7, s);
}

void test_number_game_start_sanitises_inputs() {
  NumberGame g; g.start(10, 1, 99, 0);                // swapped bounds, secret out of range, 0 tries
  EQ(1, g.minValue()); EQ(10, g.maxValue()); EQ(1, g.triesLeft());
  EQ(GuessResult::CORRECT, g.guess(10));              // secret clamped to 10
}

void test_number_parse() {
  int v = 0;
  TEST_ASSERT_TRUE(NumberGame::parseNumber("is it 42 maybe", v)); EQ(42, v);
  TEST_ASSERT_FALSE(NumberGame::parseNumber("no digits", v));
  TEST_ASSERT_FALSE(NumberGame::parseNumber("1234567", v));    // too long
  TEST_ASSERT_FALSE(NumberGame::parseNumber(0, v));
}

void test_reaction_ok_and_best() {
  ReactionGame g;
  g.start(1000, 2000);                                // GO at 3000
  g.update(2500); EQ(ReactionPhase::WAITING, g.phase());
  g.update(3000); EQ(ReactionPhase::GO, g.phase());
  g.press(3350);  EQ(ReactionPhase::DONE_OK, g.phase());
  EQ(350, g.reactionMs()); EQ(350, g.bestMs());
  g.start(5000, 100); g.press(5100 + 200);
  EQ(200, g.bestMs());
  g.start(9000, 100); g.press(9100 + 900);
  EQ(200, g.bestMs());                                // slower round does not replace best
}

void test_reaction_early_and_timeout() {
  ReactionGame g; g.start(0, 1000);
  g.press(500); EQ(ReactionPhase::DONE_EARLY, g.phase()); EQ(0, g.bestMs());
  g.start(0, 1000, 2000);
  g.update(3500); EQ(ReactionPhase::DONE_TIMEOUT, g.phase());
  g.press(3600); EQ(ReactionPhase::DONE_TIMEOUT, g.phase());   // late press ignored
}

void test_reaction_survives_millis_rollover() {
  ReactionGame g; g.start(0xFFFFFF00u, 0x200);        // GO at 0x100 after wrap
  g.update(0x50);  EQ(ReactionPhase::WAITING, g.phase());
  g.press(0x100 + 120); EQ(ReactionPhase::DONE_OK, g.phase()); EQ(120, g.reactionMs());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_rps_full_truth_table);
  RUN_TEST(test_rps_parse);
  RUN_TEST(test_rps_random_mapping_and_score);
  RUN_TEST(test_number_game_flow_win);
  RUN_TEST(test_number_game_out_of_range_costs_nothing_and_loss_reveals);
  RUN_TEST(test_number_game_start_sanitises_inputs);
  RUN_TEST(test_number_parse);
  RUN_TEST(test_reaction_ok_and_best);
  RUN_TEST(test_reaction_early_and_timeout);
  RUN_TEST(test_reaction_survives_millis_rollover);
  return UNITY_END();
}
