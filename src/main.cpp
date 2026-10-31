// Weby V1 firmware - Phase 1 (foundation + display + face)
// Hardware bring-up build: boots, prints a diagnostic report, initialises the
// display, and (when WEBY_BRINGUP_CYCLE is set) cycles through every emotion.
#include <Arduino.h>
#include "config.h"
#include "version.h"
#include "error_codes.h"
#include "hardware/display_driver.h"
#include "display/face_renderer.h"
#include "personality/emotions.h"
#include "system/state_manager.h"
#include "system/diagnostics.h"
#include "system/log.h"

using namespace weby;

static DisplayDriver g_display;
static FaceRenderer g_face(g_display.gfx());
static StateManager g_state;

static void onStateChange(AppState from, AppState to, AppEvent ev, void*) {
  LOGI("state", "%s --%s--> %s", stateName(from), eventName(ev), stateName(to));
}

void setup() {
  Serial.begin(115200);
  // Native USB-CDC: give the host a moment to attach, but never block forever.
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 2000) { delay(10); }

  printBootReport();
  g_state.setCallback(onStateChange, nullptr);
  g_state.dispatch(AppEvent::START);

  ErrorCode err = g_display.begin();
  if (!ok(err)) {
    LOGE("main", "display init failed: 0x%04X", (unsigned)err);
    g_state.dispatch(AppEvent::FATAL_ERROR);
    return;   // loop() keeps the serial console alive for diagnosis; no reboot loop
  }

  g_face.setEmotion(Emotion::IDLE);
  g_face.begin(millis());

  // Phase 1 has no network stack yet: report "no connectivity" so we land in OFFLINE_IDLE.
  g_state.dispatch(AppEvent::INIT_OK);
  g_state.dispatch(AppEvent::NET_DOWN);
}

void loop() {
  const uint32_t now = millis();

  if (g_display.ready()) {
    g_face.update(now);

#if defined(WEBY_BRINGUP_CYCLE)
    static uint32_t nextSwitch = 0;
    static uint8_t idx = 0;
    if ((int32_t)(now - nextSwitch) >= 0) {
      nextSwitch = now + config::BRINGUP_EMOTION_MS;
      Emotion e = static_cast<Emotion>(idx);
      idx = (uint8_t)((idx + 1) % static_cast<uint8_t>(Emotion::COUNT));
      LOGI("bringup", "emotion -> %s", emotionName(e));
      g_face.setSpeaking(e == Emotion::SPEAKING);
      g_face.setEmotion(e);
    }
#endif
  }

  delay(5);  // yield to the IDLE task / watchdog; real scheduling comes with FreeRTOS tasks
}
