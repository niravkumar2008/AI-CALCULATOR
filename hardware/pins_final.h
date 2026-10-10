// AI Calculator final board: every ESP32 pin in one place.
// THIS FILE IS THE SINGLE SOURCE OF TRUTH. The board firmware includes it directly
// (firmware-prototype/src/pins.h is a one-line #include of this file), so a pin changed
// here is a pin changed in the firmware. Net names in [brackets] are the schematic's.
//
// Module: ESP32-S3-MINI-1-N4R2 (4 MB quad flash + 2 MB quad PSRAM in the chip package).
// Checked against the ESP32-S3-MINI-1 datasheet v1.7, Table 3-1 + chapter 4:
//   - IO26 is taken by the PSRAM on -N4R2 parts: never use it.
//   - IO33..IO37 are exposed and free on -N4R2 (only octal-PSRAM parts lose them).
//   - Strapping pins 0, 3, 45, 46 are left unused (0 = BOOT pad only; 45 must stay low = 3.3 V flash).
//   - RTC GPIOs (can wake from deep sleep) are IO0..IO21.
//   - ADC1 (works while Wi-Fi is on) is IO1..IO10; ADC2 is not usable with Wi-Fi.
// Verified 2026-10-03 against a netlist exported with kicad-cli from hardware/kicad (stage 13):
// every U1 pad below carries the net named in its comment. Re-check after any schematic change:
//   python firmware-prototype/tools/check_pins.py
// The tester (firmware/src/pins.h) is different hardware and keeps its own pins.
#pragma once
#include <stdint.h>

// ---- Camera: OV5640 autofocus, 24-pin 0.5 mm FPC socket J1 (esp32-camera naming: D0..D7 = Y2..Y9)
// Stage 6 (2026-10-02): pins re-ordered so the bus leaves the module in the same order as the
// socket pins (no crossings, 0.6 mm track pitch). Same set of GPIOs as before, different roles.
constexpr int PIN_CAM_XCLK  = 18;  // [CAM_XCLK] 20 MHz clock out (LEDC)
constexpr int PIN_CAM_SIOD  = 40;  // [CAM_SIOD] SCCB data, own bus (not shared with the keypad); MTDO (JTAG) pin
constexpr int PIN_CAM_SIOC  = 39;  // [CAM_SIOC] SCCB clock; MTCK (JTAG) pin. 4.7 k pull-ups R18/R19 go to CAM_2V8
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
constexpr int PIN_CAM_PWDN  = 48;  // [CAM_PWDN] high = sensor powered down (active high); 10 k pull-down.
                                   // Never drive it high while CAM_2V8 is off (~330 uA into the unpowered sensor).
constexpr int PIN_CAM_RESET = 38;  // [CAM_RESET] low = reset (active low); 10 k pull-up to CAM_2V8 + 100 nF
constexpr int PIN_CAM_PWR_EN = 34; // [CAM_PWR_EN] high = 2.8 V + 1.5 V camera regulators on; R10 100 k pull-down = off in sleep

// ---- E-paper: 2.13" 250x122 bare panel (SSD1680), 24-pin 0.5 mm FPC socket J2 + booster on the board (SPI, write-only)
constexpr int PIN_EPD_CLK  = 5;    // [EPD_CLK]
constexpr int PIN_EPD_DIN  = 6;    // [EPD_DIN] (MOSI)
constexpr int PIN_EPD_CS   = 8;    // [EPD_CS]
constexpr int PIN_EPD_DC   = 41;   // [EPD_DC]
constexpr int PIN_EPD_RST  = 42;   // [EPD_RST]
constexpr int PIN_EPD_BUSY = 33;   // [EPD_BUSY] input; high while the panel is refreshing

// ---- Keypad: 49 keys as an 8 x 10 matrix on a TCA8418 (U6, I2C 0x34); ON has its own pin
constexpr int PIN_I2C_SDA    = 1;  // [I2C_SDA] 4.7 k pull-ups to 3.3 V on the board
constexpr int PIN_I2C_SCL    = 2;  // [I2C_SCL]
constexpr int PIN_KEYPAD_INT = 4;  // [KEYPAD_INT] TCA8418 INT (open drain, 10 k pull-up R15, low while a key event
                                   // waits); RTC pin: wakes the chip from deep sleep (EXT1). Clear it before sleeping
                                   // or R15 draws 330 uA all night.
constexpr int PIN_KEY_ON     = 7;  // [KEY_ON] ON pad to GND, R17 100 k pull-up (board/BOM: 100k C25741); RTC pin: wakes the chip from deep sleep (EXT1, low)
constexpr uint8_t TCA8418_I2C_ADDR = 0x34;
// TCA8418 ~RESET [TCA_RESET] has only a 10 k pull-up (R16): it is NOT wired to the ESP32. The firmware
// re-writes every TCA8418 register at start instead of resetting it.
constexpr int PIN_TCA_RESET  = -1;

// ---- Power
constexpr int PIN_VBAT_SENSE = 9;  // [VBAT_SENSE] ADC1_CH8: battery through 1 M / 1 M (+100 nF), reads half the voltage
constexpr int PIN_VBUS_SENSE = 37; // [VBUS_SENSE] high while a cable is on the magnetic connector (10 k / 20 k from VBUS = 3.3 V
                                   // at 5 V). Not an RTC pin: a cable cannot wake deep sleep
constexpr int PIN_CHG_STAT   = 35; // [CHG_STAT] MCP73831 STAT via Schottky D6, pull-up R20 100 k to VBUS_SENSE (stage 11, review S1):
                                   // low = charging, high = done. With no cable R20 has no supply, so the pin reads low:
                                   // only trust it while VBUS_SENSE is high. No internal pull-up/-down (it would back-feed).
constexpr int PIN_BATTERY    = PIN_VBAT_SENSE;  // old name, kept for older notes

// ---- USB (native USB-Serial-JTAG on the magnetic connector): IO19 = D-, IO20 = D+. Fixed by the chip.
constexpr int PIN_USB_DM = 19;     // [USB_DM]
constexpr int PIN_USB_DP = 20;     // [USB_DP]

// ---- Test pads only: IO0 = BOOT (TP1), IO43/IO44 = UART0 TX/RX (TP2/TP3, debug log), EN = TP7, GND = TP4.
//      Unused: IO3, IO45, IO46 (straps, leave unconnected).
//      Force download mode (stage 14; plugging the cable in does NOT reset the chip while the battery is
//      connected, because the battery keeps 3.3 V and EN up):
//        1. back cover off; 2. hold a wire TP1 (BOOT) -> TP4 (GND); 3. tap a second wire TP7 (EN) -> TP4 for ~0.2 s
//        (or: unplug the battery at J4 first, then hold TP1 -> GND while plugging in the cable);
//        4. release TP1, flash over the magnetic USB; 5. tap TP7 -> GND again to run the new firmware.
//      Backup path: the ROM also listens on UART0. 3.3 V USB-UART adapter on TP2 (TX) / TP3 (RX) / TP4 (GND),
//      then the same TP1 + TP7 sequence.
//      NEVER burn the security / USB-disable eFuses (DIS_USB_SERIAL_JTAG, DIS_DOWNLOAD_MODE,
//      ENABLE_SECURITY_DOWNLOAD, secure boot, flash encryption): they are the only way to lock this board out.
constexpr int PIN_BOOT     = 0;
constexpr int PIN_UART_TX  = 43;
constexpr int PIN_UART_RX  = 44;

// ---- Magnetic connector (Adafruit 5358 / MG04254FRA1S1N, 4 pins at 2.50 mm), J3 pin 1 = the piece's "N" end
// (silk "N" and "+" beside pin 1, "-" beside pin 4). Stage 14 order, set by the Adafruit #5412 cable
// (5412_C17238_4P.pdf: its face reads N, GND, D+, D-, VBUS, S; mated face to face the board piece reads
// VBUS, D-, D+, GND from its own N end). Power and ground are the OUTER pins: a piece fitted backwards swaps
// VBUS/GND (D7 clamps, host current-limits) and swaps D+/D-, but never puts 5 V on a data line.
// Meter-check the real cable before gluing (ORDER_WALKTHROUGH.md "Magnet piece").
// constexpr const char* MAG_PIN_ORDER[4] = {"VBUS", "D-", "D+", "GND"};

struct CameraPins {
  const char* name;
  int pwdn, reset, xclk, sda, scl, d0, d1, d2, d3, d4, d5, d6, d7, vsync, href, pclk;
};

constexpr CameraPins kCameraVariants[] = {
    {"Final board (OV5640 AF)", PIN_CAM_PWDN, PIN_CAM_RESET, PIN_CAM_XCLK, PIN_CAM_SIOD, PIN_CAM_SIOC,
     PIN_CAM_D0, PIN_CAM_D1, PIN_CAM_D2, PIN_CAM_D3, PIN_CAM_D4, PIN_CAM_D5, PIN_CAM_D6, PIN_CAM_D7,
     PIN_CAM_VSYNC, PIN_CAM_HREF, PIN_CAM_PCLK},
};

// Every camera GPIO, for powering the camera down: each must end up disabled with no pull
// (a pull or a driven pin back-powers the sensor through its ESD diodes / R18 / R19).
constexpr int kCameraGpios[] = {PIN_CAM_XCLK, PIN_CAM_SIOD,  PIN_CAM_SIOC, PIN_CAM_D0,   PIN_CAM_D1,   PIN_CAM_D2,
                                PIN_CAM_D3,   PIN_CAM_D4,    PIN_CAM_D5,   PIN_CAM_D6,   PIN_CAM_D7,   PIN_CAM_VSYNC,
                                PIN_CAM_HREF, PIN_CAM_PCLK,  PIN_CAM_PWDN, PIN_CAM_RESET};

// ---- Compile-time checks: a typo here fails the build instead of a board.
namespace pins_check {
constexpr int kAll[] = {PIN_CAM_XCLK, PIN_CAM_SIOD, PIN_CAM_SIOC, PIN_CAM_D0, PIN_CAM_D1, PIN_CAM_D2, PIN_CAM_D3,
                        PIN_CAM_D4, PIN_CAM_D5, PIN_CAM_D6, PIN_CAM_D7, PIN_CAM_VSYNC, PIN_CAM_HREF, PIN_CAM_PCLK,
                        PIN_CAM_PWDN, PIN_CAM_RESET, PIN_CAM_PWR_EN, PIN_EPD_CLK, PIN_EPD_DIN, PIN_EPD_CS,
                        PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY, PIN_I2C_SDA, PIN_I2C_SCL, PIN_KEYPAD_INT,
                        PIN_KEY_ON, PIN_VBAT_SENSE, PIN_VBUS_SENSE, PIN_CHG_STAT, PIN_USB_DM, PIN_USB_DP,
                        PIN_BOOT, PIN_UART_TX, PIN_UART_RX};
constexpr int kCount = sizeof(kAll) / sizeof(kAll[0]);
constexpr bool unique() {
  for (int i = 0; i < kCount; ++i)
    for (int j = i + 1; j < kCount; ++j)
      if (kAll[i] == kAll[j]) return false;
  return true;
}
constexpr bool legal() {  // real S3 GPIOs, never the PSRAM pin or the free straps
  for (int i = 0; i < kCount; ++i) {
    const int p = kAll[i];
    if (p < 0 || p > 48 || (p >= 22 && p <= 25) || p == 26 || p == 3 || p == 45 || p == 46) return false;
  }
  return true;
}
static_assert(unique(), "pins_final.h: two signals share one GPIO");
static_assert(legal(), "pins_final.h: a pin is not usable on the ESP32-S3-MINI-1-N4R2");
static_assert(PIN_KEY_ON <= 21 && PIN_KEYPAD_INT <= 21, "deep-sleep wake pins must be RTC GPIOs (IO0..IO21)");
static_assert(PIN_VBAT_SENSE >= 1 && PIN_VBAT_SENSE <= 10, "battery sense must be on ADC1 (IO1..IO10) to work with Wi-Fi");
}  // namespace pins_check
