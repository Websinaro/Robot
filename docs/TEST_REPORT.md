# Test report

Legend: **PASSED** = executed, result observed. **FAILED**. **NOT RUN**. **HW** = requires physical hardware.
Nothing here is claimed beyond what was executed; dates are 2026-11-01 unless noted.

## Host unit tests (authoring sandbox, g++ `-std=gnu++11 -Wall -Wextra -DUNIT_TEST`)
Command (per test directory):
`g++ -std=gnu++11 -Wall -Wextra -DUNIT_TEST -Isrc -Iinclude -I<unity-shim> test/<dir>/test_main.cpp <all pure modules> && ./a.out`

| Suite | Tests | Result |
|---|---|---|
| `test_state_machine` (transitions, offline wake, guards, recovery, callbacks) | 18 | **PASSED** |
| `test_personality` (emotion table, server-name mapping, neutral fallback) | 7 | **PASSED** |
| `test_games` (RPS, number guess, reaction incl. millis rollover) | 10 | **PASSED** |
| `test_network` (HTTP classification, backoff, response validation) | 10 | **PASSED** |
| `test_audio` (ring buffer boundaries/overflow, VAD on synthetic signals) | 10 | **PASSED** |
| `test_memory` (validation, stale/replay rejection, dedupe, safe ack) | 7 | **PASSED** |
| **Total** | **62** | **62 passed, 0 failed, 0 compiler warnings** |

Caveats: ran against a minimal Unity-compatible shim, **not** PlatformIO's Unity. VAD tests use synthetic sine/zero
signals and verify the *logic only* - they say nothing about the real microphone.
`pio test -e native` itself: **NOT RUN** by the author (no PlatformIO in the sandbox). The owner/CI runs it.

Mutation check (are the tests capable of failing?): 8 deliberate bugs injected into 8 different modules
(401 handling, backoff attempt counter, RPS rule, best-time rule, short-speech rule, audio size bound, stale-version
rule, offline voice guard): **8/8 detected**; originals restored and suites re-run green.

## Firmware build
| Check | Result |
|---|---|
| `pio run` (esp32-s3) - CI run of 2026-10-31 | **FAILED** - cause: stale starter files `src/display.cpp` (and `native` env being built by bare `pio run`). Not a defect in the new sources, but no green build is recorded yet. |
| Dependency resolution (espressif32 6.9.0, Adafruit GFX 1.11.11, ST7789 1.10.4) | **PASSED** (installed in the CI log) |
| `pio run -e esp32-s3` after cleanup | **NOT RUN** (awaiting owner push) |
| `pio run -e esp32-s3-n16r8` | **NOT RUN** |
| Syntax-only check of hardware sources vs hand-written stubs | **PASSED** - proves internal consistency only, not library API correctness |
| Software-framebuffer render of all 17 emotions | Visual check done in the software framebuffer (not on the real TFT); `docs/face_emotions_software_render.png` |
| Static analysis (cppcheck/clang-tidy) | **NOT RUN** (not available in the sandbox) |

## Hardware tests - all NOT RUN (require physical hardware)
Display init, orientation, face on real panel; microphone capture; speaker playback; I2S config; Wi-Fi; HTTPS auth;
server communication; wake word; recovery after network loss. (Several are also not implemented yet: see `COMPLETION_AUDIT.md`.)

## Commands for the owner
```
pio test -e native                 # host tests
pio run -e esp32-s3-n16r8          # build for N16R8 (use -e esp32-s3 as the safe fallback)
pio run -e esp32-s3-n16r8 -t upload
pio device monitor
```
