// AI Calculator board v15-LCD: every ESP32 pin in one place (stage 15, 2026-10-08).
// This is hardware/pins_final.h (v14, e-paper) with the display block replaced by the 1.9in IPS LCD.
// When the v15 board is built, firmware-prototype/src/pins.h should include THIS file instead of
// ../../hardware/pins_final.h. Net names in [brackets] are the schematic's (hardware/kicad_v15_lcd/).
//
// What changed vs v14 (everything else is identical to pins_final.h):
//   - E-paper (SSD1680, J2, booster) is gone. 1.9in 170x320 IPS, ST7789V3, 4-wire SPI, on J5 (30-pin FPC).
//   - The five e-paper SPI GPIOs are re-used, but their ROLES were re-assigned so the board's B.Cu bus
//     meets the socket without crossings: IO5 MOSI, IO6 D/C, IO8 SCK, IO41 CS, IO42 RST (review 13, 2026-10-08:
//     J5's footprint pad numbers are mirrored like J1's, pad N = socket contact 31-N, so the roles swapped vs stage15_lcd.md).
//   - Backlight PWM on IO4 (was KEYPAD_INT). KEYPAD_INT moved to IO3 (both RTC pins: deep-sleep wake still works).
//   - LCD power switch (P-MOSFET Q4, active LOW) on IO33 (was EPD_BUSY). High / floating = LCD unpowered.
//   - IO3 is a strapping pin (JTAG source) only when the JTAG-select eFuse is burned; this project never burns
//     eFuses, so it is an ordinary RTC GPIO here. It has no internal pull at reset.
#pragma once
#include <stdint.h>

// ---- Camera: OV5640 autofocus, 24-pin 0.5 mm FPC socket J1 (unchanged from v14)
constexpr int PIN_CAM_XCLK  = 18;  // [CAM_XCLK]
constexpr int PIN_CAM_SIOD  = 40;  // [CAM_SIOD]
constexpr int PIN_CAM_SIOC  = 39;  // [CAM_SIOC]
constexpr int PIN_CAM_D0    = 13;  // [CAM_D0] Y2
constexpr int PIN_CAM_D1    = 11;  // [CAM_D1] Y3
constexpr int PIN_CAM_D2    = 10;  // [CAM_D2] Y4
constexpr int PIN_CAM_D3    = 12;  // [CAM_D3] Y5
constexpr int PIN_CAM_D4    = 14;  // [CAM_D4] Y6
constexpr int PIN_CAM_D5    = 16;  // [CAM_D5] Y7
constexpr int PIN_CAM_D6    = 17;  // [CAM_D6] Y8
constexpr int PIN_CAM_D7    = 21;  // [CAM_D7] Y9
constexpr int PIN_CAM_VSYNC = 36;  // [CAM_VSYNC]
constexpr int PIN_CAM_HREF  = 47;  // [CAM_HREF]
constexpr int PIN_CAM_PCLK  = 15;  // [CAM_PCLK]
constexpr int PIN_CAM_PWDN  = 48;  // [CAM_PWDN] active high; 10 k pull-down. Never high while CAM_2V8 is off.
constexpr int PIN_CAM_RESET = 38;  // [CAM_RESET] active low; 10 k pull-up to CAM_2V8 + 100 nF
constexpr int PIN_CAM_PWR_EN = 34; // [CAM_PWR_EN] high = camera regulators on; R10 100 k pull-down = off in sleep

// ---- LCD: 1.9in 170x320 IPS, ST7789V3 (panel 190-1732TBWPG01 family = LilyGO T-Display-S3 / Heltec HT-VMT190
//      panel), 30-pin 0.5 mm FPC socket J5 (HDGC 0.5K-HX-30PWB, dual contact). 4-wire SPI, write-only (SDO open).
//      Panel pins: 1 GND, 2 VDD, 3 IM2, 4 IM1 (both tied to VDD = 4-line SPI), 5 RESET, 6 CS, 7 SCL, 8 RS (D/C),
//      9 RD (GND), 10 SDA (MOSI), 11-18 DB0-7 (GND), 19 SDO (open), 20 LEDA, 21-24 LEDK1-4, 25/30 GND, 26-29 open.
//      No TE pin on this panel.
constexpr int PIN_LCD_SCK   = 8;   // [LCD_SCK]  (was EPD_CS)
constexpr int PIN_LCD_MOSI  = 5;   // [LCD_MOSI] (was EPD_CLK)  review 13: roles swapped with the mirrored J5 numbering
constexpr int PIN_LCD_CS    = 41;  // [LCD_CS]   (was EPD_DC)   review 13
constexpr int PIN_LCD_DC    = 6;   // [LCD_DC]   (was EPD_DIN)  review 13
constexpr int PIN_LCD_RST   = 42;  // [LCD_RST]  (was EPD_RST)  review 13
constexpr int PIN_LCD_PWR_N = 33;  // [LCD_PWR_N] LOW = Q4 on = LCD_VDD (3.3 V) to the panel. R21 100 k pull-up:
                                   // the panel is unpowered in deep sleep and before the firmware runs.
                                   // Not an RTC pin: it floats in deep sleep, which R21 turns into "off".
constexpr int PIN_LCD_BL    = 4;   // [LCD_BL_EN] gate of Q5 (AO3400A) -> backlight cathodes. LEDC PWM, active high.
                                   // R22 100 k pull-down = off in sleep. 100 % duty = (3.3 V - Vf) / 15 R = ~16-37 mA (26 typ; review 14)
                                   // through the 4 parallel LEDs (panel rating 60 mA). RTC pin.
constexpr int PIN_LCD_MISO  = -1;  // SDO (pin 19) left open
constexpr int PIN_LCD_TE    = -1;  // the panel has no tearing-effect pin
constexpr int LCD_WIDTH  = 170;    // native portrait; the firmware uses it rotated to 320 x 170 landscape
constexpr int LCD_HEIGHT = 320;
constexpr int LCD_COL_OFFSET = 35; // ST7789 170-px panels: RAM column offset 35 (TFT_eSPI: TFT_WIDTH 170 handles it)
constexpr uint32_t LCD_SPI_HZ = 40000000;  // 40 MHz through the GPIO matrix (80 MHz needs IOMUX pins, not used here)
// Power-down order (hardware/stage15_lcd.md): backlight off (BL duty 0), SLPIN, then every LCD GPIO low or
// input-no-pull, THEN PIN_LCD_PWR_N high. A high SPI line into an unpowered panel back-feeds it through its ESD diodes.

// ---- Keypad: 49 keys as an 8 x 10 matrix on a TCA8418 (U6, I2C 0x34); ON has its own pin
constexpr int PIN_I2C_SDA    = 1;  // [I2C_SDA]
constexpr int PIN_I2C_SCL    = 2;  // [I2C_SCL]
constexpr int PIN_KEYPAD_INT = 3;  // [KEYPAD_INT] (was IO4 in v14) TCA8418 INT, open drain, 10 k pull-up R15, low while a
                                   // key event waits; RTC pin: wakes the chip from deep sleep (EXT1). Clear it before sleeping.
constexpr int PIN_KEY_ON     = 7;  // [KEY_ON] ON pad to GND, R17 100 k pull-up; RTC pin: wakes the chip (EXT1, low)
constexpr uint8_t TCA8418_I2C_ADDR = 0x34;
constexpr int PIN_TCA_RESET  = -1; // R16 pull-up only (as in v14)

// ---- Power (unchanged)
constexpr int PIN_VBAT_SENSE = 9;  // [VBAT_SENSE] ADC1_CH8, 1 M / 1 M divider
constexpr int PIN_VBUS_SENSE = 37; // [VBUS_SENSE] high while a cable is on the magnetic connector
constexpr int PIN_CHG_STAT   = 35; // [CHG_STAT] only valid while VBUS_SENSE is high
constexpr int PIN_BATTERY    = PIN_VBAT_SENSE;

// ---- USB (native): IO19 = D-, IO20 = D+ (unchanged)
constexpr int PIN_USB_DM = 19;
constexpr int PIN_USB_DP = 20;

// ---- Test pads only (unchanged): IO0 = BOOT (TP1), IO43/IO44 = UART0 TX/RX (TP2/TP3), EN = TP7, GND = TP4.
//      Unused: IO45, IO46 (straps, leave unconnected). Recovery = hold TP1 to TP4, tap TP7 (see pins_final.h).
constexpr int PIN_BOOT     = 0;
constexpr int PIN_UART_TX  = 43;
constexpr int PIN_UART_RX  = 44;

// ---- Magnetic connector J3 = 1 VBUS, 2 D-, 3 D+, 4 GND (N end at pin 1), unchanged.

struct CameraPins {
  const char* name;
  int pwdn, reset, xclk, sda, scl, d0, d1, d2, d3, d4, d5, d6, d7, vsync, href, pclk;
};
constexpr CameraPins kCameraVariants[] = {
    {"v15-LCD board (OV5640 AF)", PIN_CAM_PWDN, PIN_CAM_RESET, PIN_CAM_XCLK, PIN_CAM_SIOD, PIN_CAM_SIOC,
     PIN_CAM_D0, PIN_CAM_D1, PIN_CAM_D2, PIN_CAM_D3, PIN_CAM_D4, PIN_CAM_D5, PIN_CAM_D6, PIN_CAM_D7,
     PIN_CAM_VSYNC, PIN_CAM_HREF, PIN_CAM_PCLK},
};
constexpr int kCameraGpios[] = {PIN_CAM_XCLK, PIN_CAM_SIOD,  PIN_CAM_SIOC, PIN_CAM_D0,   PIN_CAM_D1,   PIN_CAM_D2,
                                PIN_CAM_D3,   PIN_CAM_D4,    PIN_CAM_D5,   PIN_CAM_D6,   PIN_CAM_D7,   PIN_CAM_VSYNC,
                                PIN_CAM_HREF, PIN_CAM_PCLK,  PIN_CAM_PWDN, PIN_CAM_RESET};
// Every LCD GPIO, for powering the panel down (drive low / float before PIN_LCD_PWR_N goes high).
constexpr int kLcdGpios[] = {PIN_LCD_SCK, PIN_LCD_MOSI, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_BL};

// ---- Compile-time checks
namespace pins_check {
constexpr int kAll[] = {PIN_CAM_XCLK, PIN_CAM_SIOD, PIN_CAM_SIOC, PIN_CAM_D0, PIN_CAM_D1, PIN_CAM_D2, PIN_CAM_D3,
                        PIN_CAM_D4, PIN_CAM_D5, PIN_CAM_D6, PIN_CAM_D7, PIN_CAM_VSYNC, PIN_CAM_HREF, PIN_CAM_PCLK,
                        PIN_CAM_PWDN, PIN_CAM_RESET, PIN_CAM_PWR_EN, PIN_LCD_SCK, PIN_LCD_MOSI, PIN_LCD_CS,
                        PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_PWR_N, PIN_LCD_BL, PIN_I2C_SDA, PIN_I2C_SCL, PIN_KEYPAD_INT,
                        PIN_KEY_ON, PIN_VBAT_SENSE, PIN_VBUS_SENSE, PIN_CHG_STAT, PIN_USB_DM, PIN_USB_DP,
                        PIN_BOOT, PIN_UART_TX, PIN_UART_RX};
constexpr int kCount = sizeof(kAll) / sizeof(kAll[0]);
constexpr bool unique() {
  for (int i = 0; i < kCount; ++i)
    for (int j = i + 1; j < kCount; ++j)
      if (kAll[i] == kAll[j]) return false;
  return true;
}
constexpr bool legal() {  // real S3 GPIOs, never the PSRAM pin or the two hard straps (45, 46). IO3 is allowed (see top).
  for (int i = 0; i < kCount; ++i) {
    const int p = kAll[i];
    if (p < 0 || p > 48 || (p >= 22 && p <= 25) || p == 26 || p == 45 || p == 46) return false;
  }
  return true;
}
static_assert(unique(), "pins_v15_lcd.h: two signals share one GPIO");
static_assert(legal(), "pins_v15_lcd.h: a pin is not usable on the ESP32-S3-MINI-1-N4R2");
static_assert(PIN_KEY_ON <= 21 && PIN_KEYPAD_INT <= 21, "deep-sleep wake pins must be RTC GPIOs (IO0..IO21)");
static_assert(PIN_VBAT_SENSE >= 1 && PIN_VBAT_SENSE <= 10, "battery sense must be on ADC1 (IO1..IO10) to work with Wi-Fi");
}  // namespace pins_check
