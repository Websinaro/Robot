#include <unity.h>
#include "system/state_manager.h"

using namespace weby;
typedef AppState S;
typedef AppEvent E;

#define EXPECT_STATE(exp, mgr) \
  TEST_ASSERT_EQUAL_STRING(stateName(exp), stateName((mgr).state()))

#define EXPECT_RESULT(exp, actual) \
  TEST_ASSERT_EQUAL_INT((int)(exp), (int)(actual))

void setUp(void) {}
void tearDown(void) {}

static void bootOnline(StateManager& m) {
  m.dispatch(E::START);
  m.dispatch(E::INIT_OK);
  m.dispatch(E::NET_UP);
}

void test_boot_to_online_idle() {
  StateManager m;
  EXPECT_STATE(S::BOOT, m);
  EXPECT_RESULT(TransitionResult::TRANSITIONED, m.dispatch(E::START));
  EXPECT_STATE(S::INITIALISING, m);
  m.dispatch(E::INIT_OK);
  EXPECT_STATE(S::CONNECTING, m);
  m.dispatch(E::NET_UP);
  EXPECT_STATE(S::ONLINE_IDLE, m);
  TEST_ASSERT_TRUE(m.online());
}

void test_first_boot_goes_through_provisioning() {
  StateManager m;
  m.dispatch(E::START);
  m.dispatch(E::NEEDS_PROVISIONING);
  EXPECT_STATE(S::PROVISIONING, m);
  m.dispatch(E::PROVISIONED);
  EXPECT_STATE(S::CONNECTING, m);
}

void test_connect_failure_falls_back_to_offline_idle() {
  StateManager m(S::CONNECTING);
  m.dispatch(E::NET_DOWN);
  EXPECT_STATE(S::OFFLINE_IDLE, m);
  TEST_ASSERT_FALSE(m.online());
}

void test_full_online_conversation() {
  StateManager m;
  bootOnline(m);
  m.dispatch(E::WAKE);            EXPECT_STATE(S::WAKE_DETECTED, m);
  m.dispatch(E::LISTEN_START);    EXPECT_STATE(S::LISTENING, m);
  m.dispatch(E::UTTERANCE_END);   EXPECT_STATE(S::UPLOADING_AUDIO, m);
  m.dispatch(E::UPLOAD_DONE);     EXPECT_STATE(S::SERVER_PROCESSING, m);
  m.dispatch(E::SEARCH_STARTED);  EXPECT_STATE(S::SEARCHING, m);
  m.dispatch(E::RESPONSE_START);  EXPECT_STATE(S::RECEIVING_RESPONSE, m);
  m.dispatch(E::CONTENT_READY);   EXPECT_STATE(S::DISPLAYING_CONTENT, m);
  m.dispatch(E::AUDIO_READY);     EXPECT_STATE(S::SPEAKING, m);
  m.dispatch(E::PLAYBACK_DONE);   EXPECT_STATE(S::ONLINE_IDLE, m);
}

void test_offline_wake_gives_local_response_only() {
  StateManager m(S::OFFLINE_IDLE);
  m.dispatch(E::WAKE);
  EXPECT_STATE(S::WAKE_DETECTED, m);
  // A cloud voice session must NOT start while offline.
  EXPECT_RESULT(TransitionResult::REJECTED, m.dispatch(E::LISTEN_START));
  EXPECT_STATE(S::WAKE_DETECTED, m);
  m.dispatch(E::LOCAL_RESPONSE_DONE);
  EXPECT_STATE(S::OFFLINE_IDLE, m);
}

void test_illegal_transitions_are_rejected_and_state_unchanged() {
  StateManager m(S::ONLINE_IDLE);
  EXPECT_RESULT(TransitionResult::REJECTED, m.dispatch(E::PLAYBACK_DONE));
  EXPECT_RESULT(TransitionResult::REJECTED, m.dispatch(E::UPLOAD_DONE));
  EXPECT_RESULT(TransitionResult::REJECTED, m.dispatch(E::START));
  EXPECT_STATE(S::ONLINE_IDLE, m);
  TEST_ASSERT_EQUAL_UINT32(3, m.rejectedCount());
  TEST_ASSERT_EQUAL_UINT32(0, m.transitionCount());
}

void test_out_of_range_event_rejected() {
  StateManager m;
  EXPECT_RESULT(TransitionResult::REJECTED, m.dispatch((AppEvent)200));
  EXPECT_STATE(S::BOOT, m);
}

void test_network_loss_mid_pipeline_goes_offline() {
  const AppState pipeline[] = {S::LISTENING, S::UPLOADING_AUDIO, S::SERVER_PROCESSING,
                               S::SEARCHING, S::RECEIVING_RESPONSE};
  for (unsigned i = 0; i < sizeof(pipeline) / sizeof(pipeline[0]); ++i) {
    StateManager m(pipeline[i]);
    m.dispatch(E::NET_UP);  // flag only
    m.dispatch(E::NET_DOWN);
    EXPECT_STATE(S::OFFLINE_IDLE, m);
  }
}

void test_network_loss_while_speaking_does_not_cut_playback() {
  StateManager m(S::SPEAKING);
  m.dispatch(E::NET_UP);
  EXPECT_RESULT(TransitionResult::HANDLED, m.dispatch(E::NET_DOWN));
  EXPECT_STATE(S::SPEAKING, m);
  m.dispatch(E::PLAYBACK_DONE);
  EXPECT_STATE(S::OFFLINE_IDLE, m);   // idle resolves using the NEW connectivity
}

void test_server_error_returns_to_idle_matching_connectivity() {
  StateManager m(S::SERVER_PROCESSING);
  m.dispatch(E::NET_UP);
  m.dispatch(E::SERVER_ERROR);
  EXPECT_STATE(S::ONLINE_IDLE, m);
}

void test_timeout_cancels_listening() {
  StateManager m(S::LISTENING);
  m.dispatch(E::NET_UP);
  m.dispatch(E::TIMEOUT);
  EXPECT_STATE(S::ONLINE_IDLE, m);
}

void test_fatal_error_enters_recovery_from_every_state() {
  for (uint8_t i = 0; i < (uint8_t)S::COUNT; ++i) {
    if ((AppState)i == S::ERROR_RECOVERY) continue;
    StateManager m((AppState)i);
    EXPECT_RESULT(TransitionResult::TRANSITIONED, m.dispatch(E::FATAL_ERROR));
    EXPECT_STATE(S::ERROR_RECOVERY, m);
    TEST_ASSERT_EQUAL_UINT8(1, m.recoveryAttempts());
  }
}

void test_repeated_fatal_in_recovery_is_idempotent() {
  StateManager m(S::ERROR_RECOVERY);
  EXPECT_RESULT(TransitionResult::HANDLED, m.dispatch(E::FATAL_ERROR));
  EXPECT_STATE(S::ERROR_RECOVERY, m);
}

void test_recovery_success_and_giveup_paths() {
  StateManager a(S::ONLINE_IDLE);
  a.dispatch(E::FATAL_ERROR);
  a.dispatch(E::RECOVERED);
  EXPECT_STATE(S::CONNECTING, a);
  TEST_ASSERT_EQUAL_UINT8(0, a.recoveryAttempts());

  // Giving up goes to OFFLINE_IDLE, never to an endless restart.
  StateManager b(S::ONLINE_IDLE);
  b.dispatch(E::FATAL_ERROR);
  b.dispatch(E::RECOVERY_FAILED);
  EXPECT_STATE(S::OFFLINE_IDLE, b);
  TEST_ASSERT_EQUAL_UINT8(0, b.recoveryAttempts());
}

void test_auth_expiry_triggers_reprovisioning() {
  StateManager m(S::ONLINE_IDLE);
  m.dispatch(E::AUTH_REQUIRED);
  EXPECT_STATE(S::PROVISIONING, m);
}

void test_game_and_sleep_paths() {
  StateManager m(S::OFFLINE_IDLE);
  m.dispatch(E::GAME_START);  EXPECT_STATE(S::GAME_ACTIVE, m);
  m.dispatch(E::GAME_END);    EXPECT_STATE(S::OFFLINE_IDLE, m);
  m.dispatch(E::SLEEP);       EXPECT_STATE(S::SLEEP, m);
  m.dispatch(E::WAKE);        EXPECT_STATE(S::WAKE_DETECTED, m);
}

static int g_cbCount = 0;
static AppState g_lastFrom, g_lastTo;
static void onTransition(AppState from, AppState to, AppEvent, void* ctx) {
  ++g_cbCount;
  g_lastFrom = from;
  g_lastTo = to;
  TEST_ASSERT_NOT_NULL(ctx);
}

void test_callback_fires_only_on_real_transitions() {
  int ctx = 1;
  StateManager m;
  m.setCallback(onTransition, &ctx);
  g_cbCount = 0;
  m.dispatch(E::START);
  m.dispatch(E::WAKE);  // rejected
  TEST_ASSERT_EQUAL_INT(1, g_cbCount);
  EXPECT_STATE(S::BOOT, StateManager(g_lastFrom));
  EXPECT_STATE(S::INITIALISING, StateManager(g_lastTo));
}

void test_names_defined_for_all_values() {
  for (uint8_t i = 0; i < (uint8_t)S::COUNT; ++i)
    TEST_ASSERT_TRUE(stateName((AppState)i)[0] != '?');
  for (uint8_t i = 0; i < (uint8_t)E::COUNT; ++i)
    TEST_ASSERT_TRUE(eventName((AppEvent)i)[0] != '?');
  TEST_ASSERT_EQUAL_STRING("?", stateName(S::COUNT));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_boot_to_online_idle);
  RUN_TEST(test_first_boot_goes_through_provisioning);
  RUN_TEST(test_connect_failure_falls_back_to_offline_idle);
  RUN_TEST(test_full_online_conversation);
  RUN_TEST(test_offline_wake_gives_local_response_only);
  RUN_TEST(test_illegal_transitions_are_rejected_and_state_unchanged);
  RUN_TEST(test_out_of_range_event_rejected);
  RUN_TEST(test_network_loss_mid_pipeline_goes_offline);
  RUN_TEST(test_network_loss_while_speaking_does_not_cut_playback);
  RUN_TEST(test_server_error_returns_to_idle_matching_connectivity);
  RUN_TEST(test_timeout_cancels_listening);
  RUN_TEST(test_fatal_error_enters_recovery_from_every_state);
  RUN_TEST(test_repeated_fatal_in_recovery_is_idempotent);
  RUN_TEST(test_recovery_success_and_giveup_paths);
  RUN_TEST(test_auth_expiry_triggers_reprovisioning);
  RUN_TEST(test_game_and_sleep_paths);
  RUN_TEST(test_callback_fires_only_on_real_transitions);
  RUN_TEST(test_names_defined_for_all_values);
  return UNITY_END();
}
