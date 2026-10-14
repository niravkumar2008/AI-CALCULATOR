// The simulated board's "hardware" as the rest of the browser build sees it (same as
// tools/sim_v15/sim_host.h): the LCD's last frame and the clock behind millis().
#pragma once
#include <cstdint>

namespace simhost {

constexpr int kW = 320, kH = 170;
// The last picture the panel got, RGB565 in normal (not panel) byte order.
extern uint16_t frame[kW * kH];
extern uint32_t flushes;  // frames pushed so far (the page repaints when it moves)

// The clock behind millis(): the browser's clock since start plus whatever advance() added.
uint32_t now();
void advance(uint32_t ms);
void useFakeClock(bool on);  // no-op in the browser

// The backlight as lcd.cpp's idle policy left it: 0 (off) .. 100 %.
int lightLevel();
bool lightIsOff();

}  // namespace simhost
