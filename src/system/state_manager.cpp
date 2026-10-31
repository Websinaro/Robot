#include "system/state_manager.h"

namespace weby {
namespace {

typedef AppState S;
typedef AppEvent E;

// Sentinel meaning "go to ONLINE_IDLE or OFFLINE_IDLE depending on connectivity".
const AppState IDLE = AppState::COUNT;

struct Transition {
  AppState from;
  AppEvent ev;
  AppState to;
};

const Transition kTable[] = {
  // --- startup ---
  {S::BOOT,           E::START,              S::INITIALISING},
  {S::INITIALISING,   E::INIT_OK,            S::CONNECTING},
  {S::INITIALISING,   E::NEEDS_PROVISIONING, S::PROVISIONING},
  {S::PROVISIONING,   E::PROVISIONED,        S::CONNECTING},
  {S::PROVISIONING,   E::NET_DOWN,           S::OFFLINE_IDLE},
  {S::CONNECTING,     E::NET_UP,             S::ONLINE_IDLE},
  {S::CONNECTING,     E::NET_DOWN,           S::OFFLINE_IDLE},
  {S::CONNECTING,     E::AUTH_REQUIRED,      S::PROVISIONING},

  // --- idle ---
  {S::OFFLINE_IDLE,   E::NET_UP,             S::ONLINE_IDLE},
  {S::OFFLINE_IDLE,   E::WAKE,               S::WAKE_DETECTED},
  {S::OFFLINE_IDLE,   E::GAME_START,         S::GAME_ACTIVE},
  {S::OFFLINE_IDLE,   E::SLEEP,              S::SLEEP},
  {S::ONLINE_IDLE,    E::NET_DOWN,           S::OFFLINE_IDLE},
  {S::ONLINE_IDLE,    E::WAKE,               S::WAKE_DETECTED},
  {S::ONLINE_IDLE,    E::GAME_START,         S::GAME_ACTIVE},
  {S::ONLINE_IDLE,    E::SLEEP,              S::SLEEP},
  {S::ONLINE_IDLE,    E::AUTH_REQUIRED,      S::PROVISIONING},

  // --- voice pipeline ---
  {S::WAKE_DETECTED,  E::LISTEN_START,       S::LISTENING},   // guarded: online only
  {S::WAKE_DETECTED,  E::LOCAL_RESPONSE_DONE, IDLE},
  {S::WAKE_DETECTED,  E::TIMEOUT,            IDLE},
  {S::LISTENING,      E::UTTERANCE_END,      S::UPLOADING_AUDIO},
  {S::LISTENING,      E::TIMEOUT,            IDLE},
  {S::LISTENING,      E::NET_DOWN,           S::OFFLINE_IDLE},
  {S::UPLOADING_AUDIO, E::UPLOAD_DONE,       S::SERVER_PROCESSING},
  {S::UPLOADING_AUDIO, E::SERVER_ERROR,      IDLE},
  {S::UPLOADING_AUDIO, E::NET_DOWN,          S::OFFLINE_IDLE},
  {S::SERVER_PROCESSING, E::SEARCH_STARTED,  S::SEARCHING},
  {S::SERVER_PROCESSING, E::RESPONSE_START,  S::RECEIVING_RESPONSE},
  {S::SERVER_PROCESSING, E::SERVER_ERROR,    IDLE},
  {S::SERVER_PROCESSING, E::NET_DOWN,        S::OFFLINE_IDLE},
  {S::SEARCHING,      E::RESPONSE_START,     S::RECEIVING_RESPONSE},
  {S::SEARCHING,      E::SERVER_ERROR,       IDLE},
  {S::SEARCHING,      E::NET_DOWN,           S::OFFLINE_IDLE},
  {S::RECEIVING_RESPONSE, E::CONTENT_READY,  S::DISPLAYING_CONTENT},
  {S::RECEIVING_RESPONSE, E::AUDIO_READY,    S::SPEAKING},
  {S::RECEIVING_RESPONSE, E::SERVER_ERROR,   IDLE},
  {S::RECEIVING_RESPONSE, E::NET_DOWN,       S::OFFLINE_IDLE},
  {S::DISPLAYING_CONTENT, E::AUDIO_READY,    S::SPEAKING},
  {S::DISPLAYING_CONTENT, E::CONTENT_DONE,   IDLE},
  {S::SPEAKING,       E::PLAYBACK_DONE,      IDLE},

  // --- games / sleep ---
  {S::GAME_ACTIVE,    E::GAME_END,           IDLE},
  {S::SLEEP,          E::WAKE,               S::WAKE_DETECTED},

  // --- recovery (FATAL_ERROR from any state is handled in dispatch()) ---
  {S::ERROR_RECOVERY, E::RECOVERED,          S::CONNECTING},
  {S::ERROR_RECOVERY, E::RECOVERY_FAILED,    S::OFFLINE_IDLE},
};

const size_t kTableSize = sizeof(kTable) / sizeof(kTable[0]);

const char* const kStateNames[] = {
  "BOOT", "INITIALISING", "PROVISIONING", "CONNECTING", "OFFLINE_IDLE",
  "ONLINE_IDLE", "WAKE_DETECTED", "LISTENING", "UPLOADING_AUDIO",
  "SERVER_PROCESSING", "SEARCHING", "RECEIVING_RESPONSE", "DISPLAYING_CONTENT",
  "SPEAKING", "GAME_ACTIVE", "SLEEP", "ERROR_RECOVERY",
};

const char* const kEventNames[] = {
  "START", "INIT_OK", "NEEDS_PROVISIONING", "PROVISIONED", "AUTH_REQUIRED",
  "NET_UP", "NET_DOWN", "WAKE", "LISTEN_START", "UTTERANCE_END", "UPLOAD_DONE",
  "SEARCH_STARTED", "RESPONSE_START", "CONTENT_READY", "AUDIO_READY",
  "CONTENT_DONE", "PLAYBACK_DONE", "LOCAL_RESPONSE_DONE", "TIMEOUT",
  "SERVER_ERROR", "GAME_START", "GAME_END", "SLEEP", "FATAL_ERROR",
  "RECOVERED", "RECOVERY_FAILED",
};

}  // namespace

const char* stateName(AppState s) {
  return static_cast<uint8_t>(s) < static_cast<uint8_t>(AppState::COUNT)
             ? kStateNames[static_cast<uint8_t>(s)] : "?";
}

const char* eventName(AppEvent e) {
  return static_cast<uint8_t>(e) < static_cast<uint8_t>(AppEvent::COUNT)
             ? kEventNames[static_cast<uint8_t>(e)] : "?";
}

StateManager::StateManager(AppState initial)
    : state_(initial), online_(false), transitions_(0), rejected_(0),
      recoveryAttempts_(0), cb_(0), cbCtx_(0) {}

AppState StateManager::idleState() const {
  return online_ ? AppState::ONLINE_IDLE : AppState::OFFLINE_IDLE;
}

void StateManager::enter(AppState to, AppEvent ev) {
  AppState from = state_;
  state_ = to;
  ++transitions_;
  if (to == AppState::ERROR_RECOVERY) {
    if (recoveryAttempts_ < 255) ++recoveryAttempts_;
  }
  if (cb_) cb_(from, to, ev, cbCtx_);
}

TransitionResult StateManager::dispatch(AppEvent ev) {
  if (static_cast<uint8_t>(ev) >= static_cast<uint8_t>(AppEvent::COUNT)) {
    ++rejected_;
    return TransitionResult::REJECTED;
  }

  // Connectivity is tracked regardless of the current state.
  if (ev == E::NET_UP) online_ = true;
  else if (ev == E::NET_DOWN) online_ = false;

  if (ev == E::FATAL_ERROR) {
    if (state_ == S::ERROR_RECOVERY) return TransitionResult::HANDLED;
    enter(S::ERROR_RECOVERY, ev);
    return TransitionResult::TRANSITIONED;
  }

  // Guard: a cloud voice session cannot start while offline.
  if (ev == E::LISTEN_START && !online_) {
    ++rejected_;
    return TransitionResult::REJECTED;
  }

  for (size_t i = 0; i < kTableSize; ++i) {
    if (kTable[i].from == state_ && kTable[i].ev == ev) {
      AppState to = (kTable[i].to == IDLE) ? idleState() : kTable[i].to;
      if (ev == E::RECOVERED || ev == E::RECOVERY_FAILED) recoveryAttempts_ = 0;
      enter(to, ev);
      return TransitionResult::TRANSITIONED;
    }
  }

  if (ev == E::NET_UP || ev == E::NET_DOWN) return TransitionResult::HANDLED;

  ++rejected_;
  return TransitionResult::REJECTED;
}

}  // namespace weby
