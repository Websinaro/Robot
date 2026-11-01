# Weby V1 - Completion audit

Audit date: 2026-11-01. Repository: github.com/Websinaro/Robot (branch `main`).

## 0. Scope and limits of this audit

GitHub blocks automated browsing of the `src/` and `include/` trees, so I could **not** read the files as they
currently sit in the repo. I could read the repo root and `platformio.ini`. This audit is therefore based on
(a) the files I authored and delivered, (b) your original starter (read in full from your first upload), and
(c) your CI log. Anything that may differ in the repo is listed under "Repository hygiene".

## 1. Existing architecture

Layered, event-driven, Arduino framework on PlatformIO. Pure-logic modules have **no Arduino includes** and are
host-tested; hardware modules wrap Arduino/Adafruit. One owner task will dispatch `AppEvent`s to the
`StateManager`; other tasks post to it (not yet built). I see no reason to change this architecture.

The prompt's suggested state names (BOOTING, IDLE, UPLOADING, GAME, OFFLINE, ...) are covered by the existing
17 states (`INITIALISING`, `ONLINE_IDLE`/`OFFLINE_IDLE`, `UPLOADING_AUDIO`, `GAME_ACTIVE`, ...). Not renamed: a rename
would add churn without changing behaviour.

## 2. Module status

| Module | Files | Status |
|---|---|---|
| State machine | `system/state_manager` | **Implemented, host-tested** (18 tests). |
| Emotions (17 incl. PLAYFUL, FRUSTRATED, OFFLINE; server-name mapping with neutral fallback) | `personality/emotions` | **Implemented, host-tested.** |
| Face renderer (blink, gaze drift, talking mouth, partial redraw) | `display/face_renderer` | **Implemented; rendered in a software framebuffer; never run on the TFT.** |
| Display driver ST7789V | `hardware/display_driver` | **Written; compile-unverified against real libs; hardware-unverified.** |
| Boot screen / logo | - | **Missing.** |
| Smooth emotion transitions | - | **Missing** (expressions switch instantly). |
| Display state interface (request expressions without drawing) | - | **Missing.** `FaceRenderer::setEmotion` is the only entry point. |
| Diagnostics | `system/diagnostics` | **Partial:** boot report only (chip/flash/PSRAM/heap). No subsystem/Wi-Fi/server/last-error report. |
| Audio ring buffer | `audio/sample_ring.h` | **Implemented, host-tested.** |
| Voice-activity endpointing | `voice/voice_activity` | **Implemented, host-tested on synthetic signals. Thresholds untuned for the real INMP441.** |
| Microphone (I2S RX) / speaker (I2S TX) drivers | - | **Missing.** Pins defined only. |
| Wake word "Hey Weby" | - | **Missing; blocked** (see section 5). |
| Retry/backoff + HTTP status policy | `network/retry_policy` | **Implemented, host-tested.** |
| Response validation (bounded struct, strict checks) | `server/response_validator` | **Implemented, host-tested.** JSON -> struct parsing **missing** (needs a JSON library; see section 6). |
| Wi-Fi manager, HTTPS client, token storage (NVS), provisioning | - | **Missing.** Contract written (`API_CONTRACT.md`). |
| Conversation pipeline | - | **Missing.** |
| Games: rock-paper-scissors, number guessing, reaction | `games/*` | **Game logic implemented, host-tested.** No display/voice/button integration, no score persistence. |
| Memory sync logic (bounded store, dedupe, stale-update rejection, safe ack) | `memory/memory_sync` | **Implemented, host-tested.** NVS persistence and network sync **missing.** |
| SD sounds | - | **Missing;** feature flag `WEBY_HAS_SD` defaults to 0. |
| Watchdog, task supervision, OTA | - | **Missing.** OTA stays out until signed updates can be done correctly. |

## 3. Bugs and technical debt found

Fixed:
1. Starter `pins.h`: `#define TFT RST 15` (space) - compile error.
2. Starter `display.h/.cpp`: constructor defined but not declared; `drawArc()` does not exist in Adafruit_GFX.
3. Starter `platformio.ini`: forced 16 MB flash without verification; unpinned dependencies.
4. My Phase 1: `TFT_BL` was assumed to be GPIO 1 and driven by firmware. The prompt forbids assuming a backlight pin. **Now -1 (not controlled).**
5. CI ran bare `pio run`, which also built the `native` env (no `main()`) and failed. `default_envs = esp32-s3` added.

Open debt:
- Face renderer erases and redraws whole eye boxes (no frame buffer, by design - no PSRAM assumption). Flicker has not been evaluated on the real panel.
- `main.cpp` still contains the bring-up emotion cycle (`WEBY_BRINGUP_CYCLE`). It must be turned off for release.
- Many `rand`-like calls use Arduino `random()`; production use needs `esp_random()` seeding review.
- The unit-test shim used in the authoring sandbox is not PlatformIO's Unity; the first `pio test -e native` on your machine is the authoritative run.

## 4. Hardware uncertainties (all in `HARDWARE_VERIFICATION.md`)

Exact module (N16R8 stated by owner; shield shows an unexplained "P2N8"), TFT logic level and backlight wiring,
SD adapter voltage/level-shifter, INMP441 channel, MAX98357A supply and speaker, power/battery design.

## 5. Wake word

Unchanged from `PHASE0_AUDIT.md`: ESP-SR WakeNet9 runs on ESP32-S3, but "Hey Weby" is not a shipped model, and a
device cannot train one from user recordings. Needed: a custom model from Espressif (cost/licence to confirm) or
a stock phrase as a development stand-in; and a decision on whether ESP-SR's IDF 5.x requirement forces a move to
Arduino-ESP32 3.x. **No wake-word code exists, and none will be faked.** The prompt's "no general offline command
system" is respected: MultiNet is only ever a fallback for the single phrase, and only after measurement.

## 6. Dependency risks

| Dependency | Pin | Risk |
|---|---|---|
| espressif32 | 6.9.0 | **Confirmed installing** in your CI log (Arduino core 2.0.17). ESP-SR may need newer IDF. |
| Adafruit GFX / ST7789 | 1.11.11 / 1.10.4 | **Confirmed installing.** ST7789 lib pulls in `Adafruit seesaw` (harmless, larger download). |
| JSON library | not chosen | Proposed: ArduinoJson 7.x. Not added, because I could not compile against it here. Added together with the HTTP client, once the build is confirmed. |
| TLS | Arduino `WiFiClientSecure` | Root-CA pinning/bundle decision pending the real server host. |

## 7. Build and test status

See `TEST_REPORT.md`. Summary: 62 host tests pass in the authoring sandbox; the ESP32 firmware build has **not** yet
been confirmed green anywhere (last CI run failed on stale files, not on new code); nothing has run on hardware.

## 8. Repository hygiene (action for the owner)

Delete from the repo: `k.zip`, `.empty`, and (if still present) `src/display.cpp`, `include/display.h`.
Replace `platformio.ini` and `.github/workflows/build_bin.yml` with the delivered versions - the repo's
`platformio.ini` on `main` is still the older one without `default_envs`.

## 9. Implementation sequence (mapped to the prompt's phases)

| Phase | State | Next concrete step |
|---|---|---|
| 0 Audit | done (this file) | - |
| 1 Build foundation | **blocked on a green `pio run`** | Apply section 8, push, send the CI log / serial boot report. |
| 2 Display + personality | partial | Boot screen, display-state interface, transitions, then hardware check. |
| 3 Audio | not started | I2S RX/TX drivers (legacy `driver/i2s.h` on core 2.0.17), half-duplex gate, tuned on real mic. |
| 4 Wake word | blocked | Espressif model decision. |
| 5-6 Server + conversation | contract + validator + retry done | NVS token store, Wi-Fi manager, HTTPS client, JSON parse, voice upload/stream. |
| 7 Search/memory/games | logic done | UI integration, NVS persistence. |
| 8-9 Hardening/release | not started | Watchdog, task supervision, release notes. |
