// Every pin the prototype board uses, in one place, copied from the board's
// schematic (hardware/kicad/ai_calc.kicad_sch, net names in brackets). If the
// schematic changes a pin, change it here and re-upload.
//
// Module: ESP32-S3-MINI-1-N4R2 (4 MB flash, 2 MB quad PSRAM). Its PSRAM uses
// GPIO 26, so that pad stays unconnected.
#pragma once

// 2.13" e-paper panel + its driver parts on the board (SPI, write-only)
constexpr int PIN_EPD_CLK = 5;    // [EPD_CLK]
constexpr int PIN_EPD_DIN = 6;    // [EPD_DIN]
constexpr int PIN_EPD_CS = 8;     // [EPD_CS]
constexpr int PIN_EPD_DC = 41;    // [EPD_DC]
constexpr int PIN_EPD_RST = 42;   // [EPD_RST]
constexpr int PIN_EPD_BUSY = 33;  // [EPD_BUSY]

// Keypad: 49 keys as an 8 x 10 matrix into a TCA8418 keypad scanner (I2C,
// 10 kOhm pull-up on INT); ON on its own pin (100 kOhm pull-up, key to GND).
constexpr int PIN_I2C_SDA = 1;      // [I2C_SDA]
constexpr int PIN_I2C_SCL = 2;      // [I2C_SCL]
constexpr int PIN_KEYPAD_INT = 4;   // [KEYPAD_INT] low while a key event waits
constexpr int PIN_KEY_ON = 7;       // [KEY_ON]

// Power
constexpr int PIN_BATTERY = 9;    // [VBAT_SENSE] battery through 1 M / 1 M: half the voltage
constexpr int PIN_CHG_STAT = 35;  // [CHG_STAT] MCP73831 STAT through a Schottky + 100 k pull-up: low while charging
constexpr int PIN_VBUS = 37;      // [VBUS_SENSE] USB 5 V through 10 k / 20 k: high when plugged in

// Camera power: both camera regulators (2.8 V, 1.5 V) are off until this is
// driven high (100 kOhm pull-down).
constexpr int PIN_CAM_PWR_EN = 34;  // [CAM_PWR_EN]

struct CameraPins {
  const char* name;
  int pwdn, reset, xclk, sda, scl, d0, d1, d2, d3, d4, d5, d6, d7, vsync, href, pclk;
};

constexpr CameraPins kCameraVariants[] = {
    // Board stage 6 order, so the bus leaves the module in the socket's pin order:
    // PWDN 48, RESET 38, XCLK 18, SIOD 40, SIOC 39, D0..D7 13 11 10 12 14 16 17 21,
    // VSYNC 36, HREF 47, PCLK 15 (same as hardware/pins_final.h)
    {"Prototype board", 48, 38, 18, 40, 39, 13, 11, 10, 12, 14, 16, 17, 21, 36, 47, 15},
};
