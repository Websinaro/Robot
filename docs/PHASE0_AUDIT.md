# Weby V1 - Phase 0 audit and compatibility report

Status date: 2026-10-31. Read the "Verification status" column: nothing here is
marked verified unless it was actually executed.

## 1. What was verified, and how

| Item | Status |
|---|---|
| State machine logic (18 tests) and emotion table (5 tests) | **Run and passing** on the host with g++ (`-std=gnu++11 -Wall -Wextra`, no warnings). |
| Face renderer layout, all 14 emotions | **Rendered through a software framebuffer** and inspected visually. Not seen on the real TFT. |
| `display_driver`, `face_renderer`, `diagnostics`, `main` | **Syntax-checked only**, against hand-written stubs of Arduino/Adafruit headers. Proves internal consistency, **not** that the real library API matches. |
| Full ESP32-S3 build (`pio run`) | **NOT done.** No PlatformIO and no network in the authoring sandbox. First real build is yours. |
| Anything on physical hardware | **NOT done.** |

## 2. Defects found in the uploaded starter (all fixed)

1. `pins.h`: `#define TFT RST 15` (space instead of underscore). `TFT_RST` was undefined, so the build fails.
2. `display.h`: the constructor `RobotDisplay()` was defined in the .cpp but never declared. Compile error.
3. `display.cpp`: called `tft.drawArc()`, which does not exist in Adafruit_GFX. Replaced with an arc routine built from `fillCircle`.
4. `platformio.ini`: forced `flash_size = 16MB` with no verification. If your module is an 8 MB part this can boot-loop. Removed; see section 3.
5. Unpinned platform and libraries (non-reproducible builds). Pinned.
6. `main.cpp` never used the display. Rewritten.
7. CI ran only `pio run`. It now also runs the host tests.

## 3. Board and memory: must be confirmed by you

"ESP32-S3-WROOM-1" is a family. Flash and PSRAM depend on the suffix printed on the module shield:

| Module | Flash | PSRAM | Reserved GPIOs |
|---|---|---|---|
| N4 / N4R2 | 4 MB | none / 2 MB quad | R2: GPIO 26-32 |
| N8 / N8R2 | 8 MB | none / 2 MB quad | |
| N16R8 | 16 MB | 8 MB **octal** | GPIO 33-37 unusable |
| N8R8 | 8 MB | 8 MB **octal** | GPIO 33-37 unusable |

**Action:** flash the Phase 1 firmware, open the serial monitor, and send me the "Weby boot report"
(it prints flash size, PSRAM found/size, heap). I will set `board_build.flash_size`,
`board_build.arduino.memory_type` and the partition table from that, not from assumption.
You can also run `esptool.py flash_id`.

The chosen GPIOs avoid every reserved range in the table, so they are safe on all variants.

## 4. Libraries (Phase 1)

| Dependency | Pinned | Note |
|---|---|---|
| PlatformIO `espressif32` | 6.9.0 | Arduino-ESP32 core 2.0.17 / ESP-IDF 4.4.x. |
| Adafruit GFX Library | 1.11.11 | |
| Adafruit ST7735 and ST7789 Library | 1.10.4 | |

**These version numbers are from memory; I could not query the registry.** Run
`pio pkg show adafruit/Adafruit\ GFX\ Library` (or just build) and tell me if any pin fails.

Display caveats: some 2.4" ST7789V modules need SPI mode 3 (`tft.init(240,320,SPI_MODE3)`); if you get
a blank or garbled screen, try that first. `FSPI` as the SPI bus id is used as in your original code and
must be confirmed by the first build. 40 MHz SPI may need lowering to 27 MHz on long wires.

## 5. Wake word "Hey Weby": the critical finding

- ESP-SR (Espressif) supports **WakeNet9 on ESP32-S3**, plus MultiNet (offline commands, English models
  `mn5q8_en`, `mn6_en`, `mn7_en`) and an Audio Front-End (AEC, VAD, noise suppression).
- **"Hey Weby" is not a shipped model.** Published models are fixed phrases (e.g. "Hi,ESP", "Alexa",
  "Jarvis", "Hey,Willow"). Custom phrases are produced by **Espressif**: either their customization
  service or their TTS-sample training pipeline (their README states TTS Pipeline V3 added English
  wake-word training on 2026-04-23).
- **A device cannot train WakeNet on-device from a few user recordings.** The enrollment flow in the
  spec therefore cannot "create a model" locally. Honest alternatives, in order of preference:
  1. **Request a custom "Hey Weby" WakeNet9 model from Espressif** (model file goes on the SD card or in a
     flash partition). Licensing and cost must be confirmed with Espressif before commercial use.
     User enrollment samples are then used only for **threshold calibration and testing**, with consent.
  2. **Development stand-in:** use a pretrained phrase (e.g. "Hi,ESP") so Phases 4-5 can be built and
     tested end to end while the custom model is being produced.
  3. **Fallback:** MultiNet7-English matching the phrase "hey weby". Command recognition is not designed
     as an always-on wake-word detector, so false-accept/CPU must be measured before adopting it.
- **Integration risk:** ESP-SR V2 targets ESP-IDF 5.x. Arduino-ESP32 2.0.17 (IDF 4.4) may not expose it.
  Phase 4 may require moving to Arduino-ESP32 3.x (pioarduino platform) or ESP-IDF with Arduino as a
  component. This decision will be made and compiled in Phase 4; it can change `platformio.ini`.
- ESP-SR AFE + WakeNet is documented for S3 with PSRAM; whether it fits your exact module is verified
  with the boot report above.

## 6. Missing / required hardware

- **SD card:** devkits have no slot. Add an external 3.3 V SPI microSD module (pins in `pins.h`, separate bus from the TFT).
- **Battery measurement:** none. Battery UI and low-battery sound stay disabled until a divider on an ADC pin exists.
- **Power:** MAX98357A at 5 V can draw high peaks; add a bulk capacitor (470-1000 uF) near it, or brownouts will reset the board.
- **INMP441:** L/R pin must be tied to GND (left slot) or 3V3 (right) to match the I2S config in Phase 2.

## 7. GPIO map (all in `include/pins.h`; not yet hardware-verified)

| Function | GPIO |
|---|---|
| TFT SCLK / MOSI / CS / DC / RST / BL | 4 / 5 / 6 / 7 / 15 / 1 |
| INMP441 SCK / WS / SD | 17 / 18 / 8 |
| MAX98357A BCLK / LRC / DIN | 9 / 10 / 11 |
| SD SCK / MISO / MOSI / CS | 12 / 13 / 14 / 21 |

Avoided: 0, 3, 45, 46 (strapping), 19, 20 (USB), 26-32 (flash), 33-37 (octal PSRAM), 43, 44 (UART0).

## 8. Architecture decisions

- Pure-logic modules (state machine, emotions, later protocol/games/storage logic) have **no Arduino includes**, so they are tested on the host.
- One owner task dispatches state events; other tasks post to it via a queue (Phase 3+). Display rendering gets its own task so SPI fills never starve audio.
- Two I2S peripherals: mic on I2S_NUM_0 (RX), amp on I2S_NUM_1 (TX), each owned by one task. Half-duplex rule enforced by the state machine.
- Audio formats: mic 16 kHz / 16-bit mono PCM; SD sounds WAV PCM16; **server TTS streamed as raw PCM16 over WebSocket binary frames** so no MP3/Opus decoder is needed on the device (revisit if bandwidth requires Opus).
- Face rendering draws only changed regions (no full-frame buffer), so it does not depend on PSRAM.
- Coding standard: C++11 compatible (matches Arduino-ESP32 2.x toolchain).

## 9. Phase 1 acceptance criteria

Verified here: state transition tests pass; emotion table valid; face renders all 14 emotions in the software framebuffer.
**You must verify on hardware:** `pio run` builds with zero errors; boot report prints; TFT shows the face in landscape (320x240, correct orientation and colours); blink, gaze drift and talking mouth animate; the 3-second emotion cycle runs for 10+ minutes without a reset.
Please paste the serial output and a photo of the screen; I won't mark the phase complete without it.

## 10. Hardware identified from owner photos (2026-10-31)

| Part | What the photo shows | Consequence |
|---|---|---|
| Dev board | ESP32-S3-WROOM-1 on a 44-pin devkit with **two USB-C ports: COM and USB**. Owner states N16R8. The shield shows a "P2N8" code that I cannot interpret, so N16R8 is **unconfirmed until the boot report** (octal PSRAM must show ~8 MB). | New env `esp32-s3-n16r8`. GPIO 35-37 are exposed on the header but must stay unused. |
| INMP441 | Pins SCK, WS, L/R, SD, VDD, GND | Wire SCK->17, WS->18, SD->8, VDD->3V3, **L/R->GND** (left channel). |
| MAX98357A (purple clone) | Pins LRC, BCLK, DIN, GAIN, SD, GND, Vin; 2.5-5.5 V; 3 W; default 9 dB; outputs (L+R)/2 | LRC->10, BCLK->9, DIN->11, **SD->2** (firmware mute), GAIN left open (9 dB), Vin->5V. Speaker on the screw terminal. |
| microSD adapter (blue) | Pins CS, SCK, MOSI, MISO, VCC, GND (back view only) | CS->21, SCK->12, MOSI->14, MISO->13, GND. Typical blue adapters have a 3.3 V regulator + level shifter and want **5 V on VCC**, but some clones lack the shifter. Front-side photo needed to confirm. |

**USB ports:** `ARDUINO_USB_CDC_ON_BOOT=1` sends `Serial` to the **USB** port (native). If you plug
into **COM**, you will see no serial output. Flash and monitor through **USB**; use COM only if you remove those two flags.
