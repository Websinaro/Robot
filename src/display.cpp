#include "display.h"
#include "pins.h"
#include <SPI.h>

// Use ESP32-S3 hardware SPI with our selected pins.
static SPIClass displaySPI(FSPI);

RobotDisplay::RobotDisplay()
    : tft(&displaySPI, TFT_CS, TFT_DC, TFT_RST) {
}

bool RobotDisplay::begin() {
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    displaySPI.begin(
        TFT_SCLK,
        -1,          // MISO not required
        TFT_MOSI,
        TFT_CS
    );

    // ST7789V native resolution: 240 x 320.
    tft.init(240, 320);

    // Rotation 1 = landscape: 320 x 240.
    tft.setRotation(1);

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextWrap(false);

    return true;
}

void RobotDisplay::clear() {
    tft.fillScreen(ST77XX_BLACK);
}

void RobotDisplay::drawFace(
    uint16_t background,
    uint16_t eyeColor,
    bool sleepy
) {
    tft.fillScreen(background);

    // Two large expressive eyes.
    if (!sleepy) {
        tft.fillRoundRect(
            75, 65, 65, 95, 24, eyeColor
        );

        tft.fillRoundRect(
            180, 65, 65, 95, 24, eyeColor
        );

        // Eye highlights.
        tft.fillCircle(105, 90, 9, ST77XX_WHITE);
        tft.fillCircle(210, 90, 9, ST77XX_WHITE);
    } else {
        // Closed eyes.
        tft.drawLine(
            75, 115, 140, 115, eyeColor
        );

        tft.drawLine(
            180, 115, 245, 115, eyeColor
        );

        tft.drawLine(
            75, 118, 140, 118, eyeColor
        );

        tft.drawLine(
            180, 118, 245, 118, eyeColor
        );
    }

    // Mouth.
    if (sleepy) {
        tft.drawRoundRect(
            145, 175, 30, 9, 4, eyeColor
        );
    } else {
        tft.drawArc(
            120, 145, 40, 25, 20, 160, eyeColor
        );
    }
}

void RobotDisplay::showStartupFace() {
    drawFace(
        ST77XX_BLACK,
        ST77XX_CYAN,
        false
    );
}

void RobotDisplay::showHappyFace() {
    drawFace(
        ST77XX_BLACK,
        ST77XX_GREEN,
        false
    );
}

void RobotDisplay::showSleepyFace() {
    drawFace(
        ST77XX_BLACK,
        ST77XX_BLUE,
        true
    );
}
