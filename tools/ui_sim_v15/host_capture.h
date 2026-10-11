// What the "panel" received: every lcd::flush() / lcd::pushRaw() lands in one 320x170
// RGB565 frame here, which the harness writes out as a PPM (make_screens.py -> PNG).
#pragma once
#include <cstdint>
#include <string>

namespace host {
constexpr int kW = 320, kH = 170;
// The last frame the panel got, RGB565 in normal (not panel) byte order.
extern uint16_t frame[kW * kH];
extern int flushes;            // frames pushed so far
extern std::string outDir;     // where save() writes
// Writes the last frame as <outDir>/<name>.ppm (RGB888, 565 expanded by bit replication).
void save(const std::string& name);
// Saves every frame pushed from now on as <prefix>_NN (self-test run). "" = off.
void autoSave(const std::string& prefix);
// The fake clock behind millis() / delay().
extern uint32_t nowMs;
}  // namespace host
