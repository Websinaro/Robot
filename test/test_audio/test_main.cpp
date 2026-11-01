#include <unity.h>
#include <math.h>
#include "audio/sample_ring.h"
#include "voice/voice_activity.h"

using namespace weby;
void setUp(void) {}
void tearDown(void) {}
#define EQ(e, a) TEST_ASSERT_EQUAL_INT((long)(e), (long)(a))

void test_ring_basic_write_read_and_wrap() {
  int16_t store[8]; SampleRing r(store, 8);           // usable capacity 7
  EQ(7, r.capacity());
  int16_t in[5] = {1, 2, 3, 4, 5}, out[8] = {0};
  EQ(5, r.write(in, 5));
  EQ(3, r.read(out, 3)); EQ(1, out[0]); EQ(3, out[2]);
  EQ(5, r.write(in, 5));                              // wraps around the end
  EQ(7, r.available());
  EQ(7, r.read(out, 8));
  int16_t expect[7] = {4, 5, 1, 2, 3, 4, 5};
  for (int i = 0; i < 7; ++i) EQ(expect[i], out[i]);
  EQ(0, r.available());
}

void test_ring_overflow_is_counted_not_silent() {
  int16_t store[4]; SampleRing r(store, 4);           // capacity 3
  int16_t in[6] = {1, 2, 3, 4, 5, 6};
  EQ(3, r.write(in, 6));
  EQ(3, r.droppedSamples());
  EQ(1, r.overflowEvents());
  EQ(0, r.write(in, 2));
  EQ(5, r.droppedSamples()); EQ(2, r.overflowEvents());
  EQ(0, r.freeSpace());
}

void test_ring_underflow_and_bad_args() {
  int16_t store[4]; SampleRing r(store, 4);
  int16_t out[4];
  EQ(0, r.read(out, 4));
  EQ(0, r.write(0, 3));
  EQ(0, r.read(0, 3));
  int16_t dead[1]; SampleRing tiny(dead, 1);          // too small to hold anything
  int16_t x = 1;
  EQ(0, tiny.capacity()); EQ(0, tiny.write(&x, 1)); EQ(0, tiny.available());
}

void test_ring_clear() {
  int16_t store[4]; SampleRing r(store, 4); int16_t x[2] = {1, 2};
  r.write(x, 2); r.clear(); EQ(0, r.available());
}

// --- VAD helpers ---
static const int N = 320;                              // 20 ms @ 16 kHz
static void tone(int16_t* b, int amp) { for (int i = 0; i < N; ++i) b[i] = (int16_t)(amp * sin(2 * 3.14159265 * 440 * i / 16000.0)); }
static void silence(int16_t* b) { for (int i = 0; i < N; ++i) b[i] = 0; }

void test_rms() {
  int16_t b[N]; tone(b, 1000);
  uint32_t r = VoiceActivity::rms(b, N);
  TEST_ASSERT_TRUE(r > 690 && r < 720);               // 1000/sqrt(2)
  EQ(0, VoiceActivity::rms(0, 10)); EQ(0, VoiceActivity::rms(b, 0));
}

void test_vad_detects_start_and_end() {
  VoiceActivity v; int16_t s[N], t[N]; silence(s); tone(t, 3000);
  for (int i = 0; i < 10; ++i) EQ(VadEvent::NONE, v.processFrame(s, N, 20));
  EQ(VadEvent::NONE, v.processFrame(t, N, 20));
  EQ(VadEvent::NONE, v.processFrame(t, N, 20));
  EQ(VadEvent::SPEECH_STARTED, v.processFrame(t, N, 20));
  TEST_ASSERT_TRUE(v.inSpeech());
  for (int i = 0; i < 30; ++i) v.processFrame(t, N, 20);     // 600 ms speech
  VadEvent e = VadEvent::NONE;
  for (int i = 0; i < 50 && e == VadEvent::NONE; ++i) e = v.processFrame(s, N, 20);
  EQ(VadEvent::UTTERANCE_END, e);
  TEST_ASSERT_TRUE(v.finished());
  EQ(VadEvent::NONE, v.processFrame(t, N, 20));              // terminal until reset
  v.reset(); TEST_ASSERT_FALSE(v.finished());
}

void test_vad_no_speech_timeout() {
  VadConfig c; c.noSpeechTimeoutMs = 1000; VoiceActivity v(c); int16_t s[N]; silence(s);
  VadEvent e = VadEvent::NONE; int frames = 0;
  while (e == VadEvent::NONE && frames < 200) { e = v.processFrame(s, N, 20); ++frames; }
  EQ(VadEvent::NO_SPEECH_TIMEOUT, e); EQ(50, frames);
}

void test_vad_rejects_short_click() {
  VoiceActivity v; int16_t s[N], t[N]; silence(s); tone(t, 5000);
  for (int i = 0; i < 3; ++i) v.processFrame(t, N, 20);      // just enough to start (60 ms)
  VadEvent e = VadEvent::NONE;
  for (int i = 0; i < 100 && e == VadEvent::NONE; ++i) e = v.processFrame(s, N, 20);
  EQ(VadEvent::TOO_SHORT, e);
}

void test_vad_max_duration() {
  VadConfig c; c.maxUtteranceMs = 1000; VoiceActivity v(c); int16_t t[N]; tone(t, 4000);
  VadEvent e = VadEvent::NONE; int frames = 0;
  while (e != VadEvent::MAX_DURATION && frames < 200) { e = v.processFrame(t, N, 20); ++frames; }
  EQ(VadEvent::MAX_DURATION, e); EQ(50, frames);
}

void test_vad_ignores_brief_dropouts_inside_speech() {
  VoiceActivity v; int16_t s[N], t[N]; silence(s); tone(t, 3000);
  for (int i = 0; i < 5; ++i) v.processFrame(t, N, 20);
  for (int i = 0; i < 10; ++i) EQ(VadEvent::NONE, v.processFrame(s, N, 20));  // 200 ms pause < 800 ms
  EQ(VadEvent::NONE, v.processFrame(t, N, 20));
  TEST_ASSERT_TRUE(v.inSpeech());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_ring_basic_write_read_and_wrap);
  RUN_TEST(test_ring_overflow_is_counted_not_silent);
  RUN_TEST(test_ring_underflow_and_bad_args);
  RUN_TEST(test_ring_clear);
  RUN_TEST(test_rms);
  RUN_TEST(test_vad_detects_start_and_end);
  RUN_TEST(test_vad_no_speech_timeout);
  RUN_TEST(test_vad_rejects_short_click);
  RUN_TEST(test_vad_max_duration);
  RUN_TEST(test_vad_ignores_brief_dropouts_inside_speech);
  return UNITY_END();
}
