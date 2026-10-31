# Weby - AI pet robot (ESP32-S3), firmware V1

**Current status: Phase 1 (foundation + display + face). Not hardware-verified.**
See `docs/PHASE0_AUDIT.md` for the hardware/software compatibility report, GPIO map and wake-word findings.

## Build / flash / monitor
Use the board's **USB** (native) port, not COM.
For N16R8 modules:
    pio run -e esp32-s3-n16r8 -t upload
Safe fallback (any variant):
    pio run -e esp32-s3 -t upload
    pio device monitor

## Host unit tests (no hardware; needs gcc/clang)
    pio test -e native

## First run
Open the serial monitor and copy the "Weby boot report" (flash/PSRAM/heap). The firmware then
cycles through all 14 emotions every 3 s (disable by removing `-DWEBY_BRINGUP_CYCLE=1`).

## Layout
    include/   pins.h (all GPIOs), config.h, error_codes.h, version.h
    src/system/        state_manager (pure logic), diagnostics, log.h
    src/personality/   emotions (pure data)
    src/hardware/      display_driver (ST7789V)
    src/display/       face_renderer
    test/              Unity host tests
    docs/              audit and design notes
