// AI Calculator final board: every ESP32 pin in one place.
// Module: ESP32-S3-MINI-1-N4R2 (4 MB quad flash + 2 MB quad PSRAM in the chip package).
// Checked against the ESP32-S3-MINI-1 datasheet v1.7, Table 3-1 + chapter 4:
//   - IO26 is taken by the PSRAM on -N4R2 parts: never use it.
//   - IO33..IO37 are exposed and free on -N4R2 (only octal-PSRAM parts lose them).
//   - Strapping pins 0, 3, 45, 46 are left unused (0 = BOOT pad only; 45 must stay low = 3.3 V flash).
//   - RTC GPIOs (can wake from deep sleep) are IO0..IO21.
//   - ADC1 (works while Wi-Fi is on) is IO1..IO10; ADC2 is not usable with Wi-Fi.
// The tester (firmware/src/pins.h) and the WROOM prototype (firmware-prototype/src/pins.h) differ.
#pragma once

// ---- Camera: OV5640 autofocus, 24-pin 0.5 mm FPC socket (esp32-camera naming: D0..D7 = Y2..Y9)
// Stage 6 (2026-10-02): pins re-ordered so the bus leaves the module in the same order as the
// socket pins (no crossings, 0.6 mm track pitch). Same set of GPIOs as before, different roles.
constexpr int PIN_CAM_XCLK  = 18;  // 20 MHz clock out (LEDC)
constexpr int PIN_CAM_SIOD  = 40;  // SCCB data, own bus (not shared with the keypad)
constexpr int PIN_CAM_SIOC  = 39;  // SCCB clock
constexpr int PIN_CAM_D0    = 13;  // Y2
constexpr int PIN_CAM_D1    = 11;  // Y3
constexpr int PIN_CAM_D2    = 10;  // Y4
constexpr int PIN_CAM_D3    = 12;  // Y5
constexpr int PIN_CAM_D4    = 14;  // Y6
constexpr int PIN_CAM_D5    = 16;  // Y7
constexpr int PIN_CAM_D6    = 17;  // Y8
constexpr int PIN_CAM_D7    = 21;  // Y9
constexpr int PIN_CAM_VSYNC = 36;
constexpr int PIN_CAM_HREF  = 47;
constexpr int PIN_CAM_PCLK  = 15;
constexpr int PIN_CAM_PWDN  = 48;  // high = sensor powered down (OV5640 PWDN is active high)
constexpr int PIN_CAM_RESET = 38;  // low = reset (active low); 10 k pull-up on the board
constexpr int PIN_CAM_PWR_EN = 34; // high = 2.8 V + 1.5 V camera regulators on; 100 k pull-down = off in sleep

// ---- E-paper: 2.13" 250x122 bare panel, 24-pin 0.5 mm FPC socket + booster on the board (SPI, write-only)
constexpr int PIN_EPD_CLK  = 5;
constexpr int PIN_EPD_DIN  = 6;
constexpr int PIN_EPD_CS   = 8;
constexpr int PIN_EPD_DC   = 41;
constexpr int PIN_EPD_RST  = 42;
constexpr int PIN_EPD_BUSY = 33;   // input; high while the panel is refreshing

// ---- Keypad: 49 keys as an 8 x 10 matrix on a TCA8418 (I2C 0x34); ON has its own pin
constexpr int PIN_I2C_SDA    = 1;  // 4.7 k pull-ups to 3.3 V on the board
constexpr int PIN_I2C_SCL    = 2;
constexpr int PIN_KEYPAD_INT = 4;  // TCA8418 INT (open drain, low while a key event waits); RTC pin
constexpr int PIN_KEY_ON     = 7;  // ON pad to GND, 100 k pull-up; RTC pin: wakes the chip from deep sleep (EXT0, low)

// ---- Power
constexpr int PIN_BATTERY    = 9;  // ADC1_CH8: battery through 1 M / 1 M (+100 nF), reads half the voltage
constexpr int PIN_VBUS_SENSE = 37; // high while a cable is on the magnetic connector (10 k / 20 k from VBUS = 3.3 V at 5 V). Not an RTC pin: a cable cannot wake deep sleep
constexpr int PIN_CHG_STAT   = 35; // MCP73831 STAT via a Schottky clamp + 100 k pull-up: low = charging, high = done / no cable
                                   // (STAT drives 5 V when done; D6 blocks it, so IO35 never sees more than 3.3 V)

// ---- USB (native USB-Serial-JTAG on the magnetic connector): IO19 = D-, IO20 = D+. Fixed by the chip.
constexpr int PIN_USB_DM = 19;
constexpr int PIN_USB_DP = 20;

// ---- Test pads only: IO0 = BOOT (short to GND while plugging in to force download mode),
//      IO43/IO44 = UART0 TX/RX (debug log). Unused: IO3, IO45, IO46 (straps, leave unconnected).

// ---- Magnetic connector (Adafruit 5358 / MG04254FRA1S1N, 4 pins at 2.50 mm), pin 1 nearest the "N" magnet.
// One-line change: the board wires pin N to MAG_PIN_ORDER[N-1]. VBUS sits next to GND only, so a
// slipped contact shorts VBUS to GND (cable current limit) and never puts 5 V on D+ / D-.
// constexpr const char* MAG_PIN_ORDER[4] = {"VBUS", "GND", "D-", "D+"};

struct CameraPins {
  const char* name;
  int pwdn, reset, xclk, sda, scl, d0, d1, d2, d3, d4, d5, d6, d7, vsync, href, pclk;
};

constexpr CameraPins kCameraVariants[] = {
    {"Final board (OV5640 AF)", PIN_CAM_PWDN, PIN_CAM_RESET, PIN_CAM_XCLK, PIN_CAM_SIOD, PIN_CAM_SIOC,
     PIN_CAM_D0, PIN_CAM_D1, PIN_CAM_D2, PIN_CAM_D3, PIN_CAM_D4, PIN_CAM_D5, PIN_CAM_D6, PIN_CAM_D7,
     PIN_CAM_VSYNC, PIN_CAM_HREF, PIN_CAM_PCLK},
};
