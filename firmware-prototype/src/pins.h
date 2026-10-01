// Every pin the prototype board uses, in one place. The board's schematic
// follows this file (and the key matrix in keys.cpp); change a number here
// and re-upload, nothing else needs to move.
//
// Module: ESP32-S3-WROOM-1-N16R8. Its PSRAM uses GPIO 33-37, so none of
// those appear here.
#pragma once

// Waveshare 2.13" e-paper panel + its driver parts on the board (SPI, write-only)
constexpr int PIN_EPD_CLK = 5;
constexpr int PIN_EPD_DIN = 6;
constexpr int PIN_EPD_CS = 8;
constexpr int PIN_EPD_DC = 41;
constexpr int PIN_EPD_RST = 42;
constexpr int PIN_EPD_BUSY = 3;  // input only; GPIO 3's boot strapping is unused by default

// Keypad: the Casio's pads wired as a 7 x 7 matrix into a TCA8418 keypad
// scanner (I2C). ON has its own pin so it can wake the chip.
constexpr int PIN_KEY_ON = 7;      // to GND when pressed (internal pull-up)
constexpr int PIN_I2C_SDA = 1;
constexpr int PIN_I2C_SCL = 2;
constexpr int PIN_KEYPAD_INT = 4;  // TCA8418 INT, low while a key event waits

constexpr int PIN_BATTERY = 9;  // battery through 2 x 100 kOhm: reads half the voltage

struct CameraPins {
  const char* name;
  int pwdn, reset, xclk, sda, scl, d0, d1, d2, d3, d4, d5, d6, d7, vsync, href, pclk;
};

constexpr CameraPins kCameraVariants[] = {
    // Seeed XIAO ESP32-S3 Sense mapping, plus a power-down pin (21)
    {"Prototype board", 21, -1, 10, 40, 39, 15, 17, 18, 16, 14, 12, 11, 48, 38, 47, 13},
};
