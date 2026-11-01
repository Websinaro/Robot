#include <unity.h>
#include <string.h>
#include "network/retry_policy.h"
#include "server/response_validator.h"

using namespace weby;
void setUp(void) {}
void tearDown(void) {}
#define EQ(e, a) TEST_ASSERT_EQUAL_INT((int)(e), (int)(a))

void test_http_status_classification() {
  EQ(HttpAction::SUCCESS, classifyHttpStatus(200));
  EQ(HttpAction::SUCCESS, classifyHttpStatus(204));
  EQ(HttpAction::REAUTH, classifyHttpStatus(401));
  EQ(HttpAction::FAIL, classifyHttpStatus(403));
  EQ(HttpAction::FAIL, classifyHttpStatus(404));
  EQ(HttpAction::FAIL, classifyHttpStatus(413));
  EQ(HttpAction::RETRY, classifyHttpStatus(408));
  EQ(HttpAction::RETRY, classifyHttpStatus(429));
  EQ(HttpAction::RETRY, classifyHttpStatus(503));
  EQ(HttpAction::FAIL, classifyHttpStatus(501));
  EQ(HttpAction::RETRY, classifyHttpStatus(0));      // transport error
  EQ(HttpAction::RETRY, classifyHttpStatus(-1));
  EQ(HttpAction::FAIL, classifyHttpStatus(302));
}

void test_backoff_grows_exponentially_and_caps() {
  BackoffConfig c; c.baseMs = 500; c.maxMs = 3000; c.maxAttempts = 6;
  Backoff b(c);
  EQ(500, b.nextDelayMs(1000));
  EQ(1000, b.nextDelayMs(1000));
  EQ(2000, b.nextDelayMs(1000));
  EQ(3000, b.nextDelayMs(1000));   // capped
  EQ(3000, b.nextDelayMs(1000));
}

void test_backoff_jitter_range_and_attempt_limit() {
  Backoff b;                                         // base 500, 5 attempts
  EQ(250, b.nextDelayMs(0));                         // lower bound d/2
  EQ(750, b.nextDelayMs(500));                       // d=1000: 500 + 250
  EQ(2000, b.nextDelayMs(5000));                     // jitter clamped to 1000 -> d=2000
  TEST_ASSERT_TRUE(b.canRetry());
  b.nextDelayMs(0); b.nextDelayMs(0);
  TEST_ASSERT_FALSE(b.canRetry());
  EQ(5, b.attempts());
  b.reset();
  TEST_ASSERT_TRUE(b.canRetry());
}

void test_backoff_no_overflow_with_huge_base() {
  BackoffConfig c; c.baseMs = 0xF0000000u; c.maxMs = 0xFFFFFFFFu; c.maxAttempts = 255;
  Backoff b(c);
  for (int i = 0; i < 40; ++i) b.nextDelayMs(1000);  // must not crash or wrap to tiny values
  TEST_ASSERT_TRUE(b.nextDelayMs(1000) >= 0x7FFFFFFFu);
}

static ConversationResponse good() {
  ConversationResponse r;
  strcpy(r.requestId, "req_123-ABC");
  strcpy(r.status, "ok");
  strcpy(r.replyText, "Hello there!");
  strcpy(r.emotion, "happy");
  strcpy(r.interactionType, "chat");
  r.audio.present = true;
  strcpy(r.audio.format, "pcm_s16le");
  r.audio.sampleRate = 24000; r.audio.channels = 1; r.audio.sizeBytes = 48000;
  return r;
}

void test_valid_response_accepted() {
  ConversationResponse r = good();
  EQ(ErrorCode::OK, validateResponse(r));
  EQ(Emotion::HAPPY, resolveEmotion(r));
}

void test_text_only_response_is_valid_but_empty_is_not() {
  ConversationResponse r = good(); r.audio.present = false;
  EQ(ErrorCode::OK, validateResponse(r));
  r.replyText[0] = 0;
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
}

void test_bad_identity_and_status_rejected() {
  ConversationResponse r = good(); r.requestId[0] = 0;
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); strcpy(r.requestId, "bad id/../");
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); strcpy(r.status, "error");
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); strcpy(r.interactionType, "firmware_update");
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
}

void test_audio_constraints() {
  ConversationResponse r = good(); strcpy(r.audio.format, "mp3");
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); r.audio.channels = 2;
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); r.audio.sampleRate = 8000;
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); r.audio.sizeBytes = 0;
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); r.audio.sizeBytes = (uint32_t)kMaxAudioBytes + 1;
  EQ(ErrorCode::PAYLOAD_TOO_LARGE, validateResponse(r));
  r = good(); r.audio.sizeBytes = (uint32_t)kMaxAudioBytes;
  EQ(ErrorCode::OK, validateResponse(r));
}

void test_unterminated_strings_rejected() {
  ConversationResponse r = good();
  memset(r.replyText, 'A', sizeof r.replyText);     // no NUL: would overrun if used
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); memset(r.emotion, 'x', sizeof r.emotion);
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  EQ(Emotion::IDLE, resolveEmotion(r));
}

void test_display_duration_bound_and_unknown_emotion() {
  ConversationResponse r = good(); r.displayDurationMs = kMaxDisplayMs + 1;
  EQ(ErrorCode::PAYLOAD_INVALID, validateResponse(r));
  r = good(); strcpy(r.emotion, "dancing_banana");
  EQ(ErrorCode::OK, validateResponse(r));            // unknown emotion is NOT fatal ...
  EQ(Emotion::IDLE, resolveEmotion(r));              // ... it becomes neutral
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_http_status_classification);
  RUN_TEST(test_backoff_grows_exponentially_and_caps);
  RUN_TEST(test_backoff_jitter_range_and_attempt_limit);
  RUN_TEST(test_backoff_no_overflow_with_huge_base);
  RUN_TEST(test_valid_response_accepted);
  RUN_TEST(test_text_only_response_is_valid_but_empty_is_not);
  RUN_TEST(test_bad_identity_and_status_rejected);
  RUN_TEST(test_audio_constraints);
  RUN_TEST(test_unterminated_strings_rejected);
  RUN_TEST(test_display_duration_bound_and_unknown_emotion);
  return UNITY_END();
}
