// Every pin the tester uses, in one place. PROVISIONAL until the board's
// printed labels are checked (see the Build Book checklist); change a number
// here and re-upload, nothing else needs to move.
#pragma once

// Waveshare 2.13" e-paper HAT V4 (SPI, write-only)
constexpr int PIN_EPD_DIN = 3;
constexpr int PIN_EPD_CLK = 42;
constexpr int PIN_EPD_CS = 41;
constexpr int PIN_EPD_DC = 40;
constexpr int PIN_EPD_RST = 39;
constexpr int PIN_EPD_BUSY = 38;

// Keys: each between its pin and GND (internal pull-ups, pressed = LOW)
constexpr int PIN_KEY_EQ = 1;
constexpr int PIN_KEY_AC = 14;
constexpr int PIN_KEY_UP = 21;
constexpr int PIN_KEY_DOWN = 47;
constexpr int PIN_KEY_ON = 0;  // the board's own BOOT button

// Camera wiring variants seen on ESP32-S3 camera boards. The firmware tries
// them in order and keeps the first one whose sensor answers.
struct CameraPins {
  const char* name;
  int pwdn, reset, xclk, sda, scl, d0, d1, d2, d3, d4, d5, d6, d7, vsync, href, pclk;
};

constexpr CameraPins kCameraVariants[] = {
    // ESP32-S3-CAM / Freenove / ESP32-S3-EYE layout (Keyestudio MB0184 docs)
    {"ESP32-S3-CAM (Freenove/EYE)", -1, -1, 15, 4, 5, 11, 9, 8, 10, 12, 18, 17, 16, 6, 7, 13},
    // Seeed XIAO ESP32-S3 Sense (only if the tester board is swapped)
    {"XIAO ESP32-S3 Sense", -1, -1, 10, 40, 39, 15, 17, 18, 16, 14, 12, 11, 48, 38, 47, 13},
};
