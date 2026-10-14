// The simulated board's "hardware" as the rest of the simulator sees it: the LCD's last
// frame (what lcd::flush() / lcd::pushRaw() sent, host_hw.cpp) and the clock behind
// millis(). No LovyanGFX and no windows.h here, so any file may include it.
#pragma once
#include <cstdint>

namespace simhost {

constexpr int kW = 320, kH = 170;
// The last picture the panel got, RGB565 in normal (not panel) byte order.
extern uint16_t frame[kW * kH];
extern uint32_t flushes;  // frames pushed so far (the window repaints when it moves)

// The clock behind millis(): real time since start plus whatever advance() added. With
// the fake clock (--selftest) only advance() moves it.
uint32_t now();
void advance(uint32_t ms);
void useFakeClock(bool on);

// The backlight as lcd.cpp's idle policy left it: 0 (off) .. 100 %.
int lightLevel();
// Light off (by the idle policy). The firmware then ignores the next key except to wake.
bool lightIsOff();

}  // namespace simhost
