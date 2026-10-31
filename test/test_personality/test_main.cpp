#include <unity.h>
#include "personality/emotions.h"

using namespace weby;

void setUp(void) {}
void tearDown(void) {}

void test_every_emotion_has_a_distinct_name_and_sane_profile() {
  for (uint8_t i = 0; i < (uint8_t)Emotion::COUNT; ++i) {
    const EmotionProfile& p = emotionProfile((Emotion)i);
    TEST_ASSERT_NOT_NULL(p.name);
    TEST_ASSERT_TRUE(p.eyeOpenPct >= 10 && p.eyeOpenPct <= 100);
    TEST_ASSERT_TRUE((uint8_t)p.mouth < (uint8_t)MouthShape::COUNT);
    TEST_ASSERT_TRUE(p.lidTilt >= -2 && p.lidTilt <= 2);
    for (uint8_t j = 0; j < i; ++j)
      TEST_ASSERT_TRUE(emotionProfile((Emotion)j).name != p.name);
  }
}

void test_profile_order_matches_enum() {
  TEST_ASSERT_EQUAL_STRING("HAPPY", emotionName(Emotion::HAPPY));
  TEST_ASSERT_EQUAL_STRING("SEARCHING", emotionName(Emotion::SEARCHING));
  TEST_ASSERT_EQUAL_STRING("IDLE", emotionName(Emotion::IDLE));
}

void test_lookup_by_name_is_case_insensitive() {
  Emotion e = Emotion::IDLE;
  TEST_ASSERT_TRUE(emotionFromName("sUrPrIsEd", e));
  TEST_ASSERT_EQUAL_INT((int)Emotion::SURPRISED, (int)e);
}

void test_unknown_or_null_name_rejected_and_output_untouched() {
  Emotion e = Emotion::SAD;
  TEST_ASSERT_FALSE(emotionFromName("DANCING", e));
  TEST_ASSERT_FALSE(emotionFromName("", e));
  TEST_ASSERT_FALSE(emotionFromName("HAPP", e));
  TEST_ASSERT_FALSE(emotionFromName("HAPPYY", e));
  TEST_ASSERT_FALSE(emotionFromName(0, e));
  TEST_ASSERT_EQUAL_INT((int)Emotion::SAD, (int)e);
}

void test_out_of_range_emotion_falls_back_to_idle() {
  TEST_ASSERT_EQUAL_STRING("IDLE", emotionName((Emotion)99));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_every_emotion_has_a_distinct_name_and_sane_profile);
  RUN_TEST(test_profile_order_matches_enum);
  RUN_TEST(test_lookup_by_name_is_case_insensitive);
  RUN_TEST(test_unknown_or_null_name_rejected_and_output_untouched);
  RUN_TEST(test_out_of_range_emotion_falls_back_to_idle);
  return UNITY_END();
}
