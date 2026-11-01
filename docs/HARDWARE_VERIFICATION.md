# Hardware verification register

Nothing in the "Evidence" column is a measurement unless it says so. **No physical hardware was available to the
author; every item below is UNVERIFIED until the owner reports results.**

| Item | Current assumption | Evidence | Status |
|---|---|---|---|
| MCU module | ESP32-S3-WROOM-1 N16R8 (16 MB flash, 8 MB octal PSRAM) | Owner statement; module shield shows an unexplained "P2N8"; CI log's "N8 / No PSRAM" is only PlatformIO's generic board label | **Unverified** - serial boot report must show PSRAM ~8 MB |
| Board | 44-pin devkit, two USB-C (COM / USB) | Owner photo | Visually identified |
| Display | ST7789V 240x320, 4-wire SPI, no touch | Owner spec | **Unverified** |
| TFT logic level | Module accepts 3.3 V logic | **Not assumed verified.** A module powered from 5 V may still expect 3.3 V logic; confirm from the seller datasheet before first power-up | **Unverified** |
| TFT backlight | Not controlled (`TFT_BL = -1`) | No separate BL pin assumed | Configurable |
| SPI mode | Mode 0; some modules need mode 3 | - | Try `SPI_MODE3` on blank/garbled screen |
| INMP441 | L/R -> GND (left slot); 3.3 V | Owner photo shows SCK/WS/L/R/SD/VDD/GND | **Unverified** - channel configurable; test left vs right |
| MAX98357A | Vin 5 V, SD -> GPIO 2, GAIN open (9 dB), mono mix | Owner photo of purple clone | **Unverified** - add 470-1000 uF near Vin |
| Speaker | 4-8 ohm, ~3 W | Owner spec | Unverified |
| microSD | External SPI module on its own bus; **optional** | Owner photo shows back only | **Unverified**; `WEBY_HAS_SD=0` until verified. Level shifter / 5 V vs 3.3 V on VCC unknown |
| Battery / regulator | **Not purchased / not designed** | - | **Open.** USB power only; no battery UI/low-battery sound |

GPIO map: `include/pins.h` (single source). Avoided: 0, 3, 45, 46 (strapping); 19, 20 (USB); 26-32 (flash);
33-37 (octal PSRAM); 43, 44 (UART0). GPIO 48 is the on-board RGB LED (unused).

## Procedures (to be executed by the owner)
1. **Boot report:** flash, open serial on the **USB** port, copy the "Weby boot report". Record flash size and PSRAM.
2. **Display:** confirm landscape 320x240, colours correct (cyan eyes), no mirrored image; watch the 3 s emotion cycle for 10 minutes.
3. **Microphone (Phase 3):** record 5 s, check which I2S slot carries data, measure noise-floor RMS in a quiet room and
   speech RMS at ~30 cm; set `VadConfig::speechRmsThreshold` between them.
4. **Speaker (Phase 3):** play a 440 Hz tone at low volume first; check for clipping and pops (use the amp SD pin).
5. **Half-duplex (Phase 3):** play speech while capturing; confirm the capture path is gated and no self-echo is uploaded.
6. **SD (when enabled):** mount, list, read a WAV; verify behaviour with the card removed.
7. **Power:** watch for brownout resets when the amp plays at higher volume.
