#pragma once
// Central application state machine (pure logic, no Arduino dependencies,
// unit-testable on the host).
//
// Threading: NOT internally synchronised. Exactly one owner task must call
// dispatch(); other tasks post events to that task through a queue (Phase 3+).
#include <stddef.h>
#include <stdint.h>

namespace weby {

enum class AppState : uint8_t {
  BOOT, INITIALISING, PROVISIONING, CONNECTING, OFFLINE_IDLE, ONLINE_IDLE,
  WAKE_DETECTED, LISTENING, UPLOADING_AUDIO, SERVER_PROCESSING, SEARCHING,
  RECEIVING_RESPONSE, DISPLAYING_CONTENT, SPEAKING, GAME_ACTIVE, SLEEP,
  ERROR_RECOVERY,
  COUNT
};

enum class AppEvent : uint8_t {
  START, INIT_OK, NEEDS_PROVISIONING, PROVISIONED, AUTH_REQUIRED,
  NET_UP, NET_DOWN, WAKE, LISTEN_START, UTTERANCE_END, UPLOAD_DONE,
  SEARCH_STARTED, RESPONSE_START, CONTENT_READY, AUDIO_READY, CONTENT_DONE,
  PLAYBACK_DONE, LOCAL_RESPONSE_DONE, TIMEOUT, SERVER_ERROR, GAME_START,
  GAME_END, SLEEP, FATAL_ERROR, RECOVERED, RECOVERY_FAILED,
  COUNT
};

enum class TransitionResult : uint8_t {
  TRANSITIONED,  // state changed
  HANDLED,       // accepted, state unchanged (e.g. connectivity flag update)
  REJECTED       // illegal in the current state; nothing changed
};

typedef void (*TransitionCallback)(AppState from, AppState to, AppEvent ev, void* ctx);

const char* stateName(AppState s);
const char* eventName(AppEvent e);

class StateManager {
 public:
  explicit StateManager(AppState initial = AppState::BOOT);

  TransitionResult dispatch(AppEvent ev);

  AppState state() const { return state_; }
  bool online() const { return online_; }
  bool isIdle() const {
    return state_ == AppState::OFFLINE_IDLE || state_ == AppState::ONLINE_IDLE;
  }
  uint32_t transitionCount() const { return transitions_; }
  uint32_t rejectedCount() const { return rejected_; }
  // Consecutive entries into ERROR_RECOVERY since the last RECOVERED/RECOVERY_FAILED.
  // The caller uses this to back off and eventually give up (no endless reboot loops).
  uint8_t recoveryAttempts() const { return recoveryAttempts_; }

  void setCallback(TransitionCallback cb, void* ctx) { cb_ = cb; cbCtx_ = ctx; }

 private:
  AppState idleState() const;
  void enter(AppState to, AppEvent ev);

  AppState state_;
  bool online_;
  uint32_t transitions_;
  uint32_t rejected_;
  uint8_t recoveryAttempts_;
  TransitionCallback cb_;
  void* cbCtx_;
};

}  // namespace weby
