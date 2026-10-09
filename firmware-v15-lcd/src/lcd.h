// The v15 board's screen: a 1.9" 170x320 IPS LCD (ST7789V3, 4-wire SPI, write-only) used in
// landscape as 320 x 170, with its high-side power switch (Q4, LCD_PWR_N) and PWM backlight
// (Q5, IO4). Replaces the v14 e-paper's screen.cpp: there is no partial / full refresh,
// no ghosting and no BUSY line; a frame is drawn into a 320x170 RGB565 canvas in PSRAM
// and pushed over SPI (~22 ms at 40 MHz).
//
// Power sequence (hardware/stage15_lcd.md): every LCD GPIO low -> LCD_PWR_N low (panel
// powered) -> 10 ms -> reset pulse -> init -> backlight up. Off is the reverse: backlight 0 ->
// SLPIN -> every LCD GPIO low -> LCD_PWR_N high. A high SPI line into an unpowered panel
// would back-feed it through its ESD diodes, so the order matters.
#pragma once
#include <stdint.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace lcd {

constexpr int kW = 320;
constexpr int kH = 170;

// Powers the panel, initialises it and allocates the canvas. False = no PSRAM for the
// canvas (the panel itself cannot report anything back: SDO is not wired).
bool begin();
bool ok();
// Backlight 0, SLPIN, every LCD pin low, LCD_PWR_N high: the panel draws < 2 uA. Call
// before deep sleep. begin() powers it up again.
void powerOff();
bool powered();
// Panel sleep with the power kept (idle for a few minutes): ~20 uA for the panel, and it
// wakes in ~120 ms instead of a full init. wake() is called by any draw.
void sleepPanel();
void wake();
bool asleep();

// The frame being drawn. 320 x 170, 16-bit, byte-swapped (panel order), in PSRAM.
LGFX_Sprite& canvas();
// Writes the canvas to the panel (wakes it if asleep).
void flush();
uint32_t lastFlushMs();  // how long the last flush took (self-test)
// Pushes a raw 320 x h RGB565 frame (byte-swapped, panel order) straight to the panel at
// row y: the live camera preview path, no canvas copy.
void pushRaw(int y, const uint16_t* pixels, int h);
// Fills the whole panel with one colour (self-test, blanking).
void fill(uint16_t color565);

// ---- backlight (LEDC PWM on PIN_LCD_BL)
// The user's brightness, 10..100 %, kept in NVS. setLevel() is what the light does now
// (dimmed / off while idle); it never exceeds brightness().
void setBrightness(int percent, bool save);
int brightness();
void setLevel(int percent);
int level();
// Idle policy (NVS): dim after dimSeconds, light off after offSeconds, panel to sleep
// after sleepSeconds without a key. 0 = never.
struct IdlePolicy {
  int dimSeconds = 20;
  int offSeconds = 60;
  int sleepSeconds = 120;
};
const IdlePolicy& idlePolicy();
void setIdlePolicy(const IdlePolicy& p, bool save);
// Applies the policy for `idleMs` since the last key; `keepOn` (viewfinder, update
// progress) holds full brightness. Returns true if the light is off (the next key should
// only wake the screen).
bool serviceIdle(uint32_t idleMs, bool keepOn);
bool lightOff();

// ---- colours (RGB888 -> the panel's 565)
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return uint16_t(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

}  // namespace lcd
