// The hardware the firmware's drawing code talks to, replaced for the browser build.
// A copy of tools/sim_v15/host_hw.cpp adapted for WebAssembly:
//  - lcd:: (lcd.cpp on the board drives the ST7789 over SPI): a flush copies the real
//    LGFX_Sprite canvas into simhost::frame, which web_main.cpp hands to the page's <canvas>.
//    The backlight and its idle policy behave as lcd.cpp's, minus NVS.
//  - millis(): the browser's monotonic clock (emscripten_get_now). delay() never blocks
//    (one thread, driven by the page's animation frames).
//  - the few Arduino objects the shim headers declare (Serial, ESP, WiFi).
#include <emscripten/emscripten.h>

#include <cstdarg>
#include <cstdio>
#include <random>

#include "Arduino.h"
#include "WiFi.h"
#include "lcd.h"
#include "sim_host.h"

namespace simhost {
uint16_t frame[kW * kH];
uint32_t flushes = 0;

static const double g_t0 = emscripten_get_now();
static uint64_t g_offset = 0;

uint32_t now() {
  const double real = emscripten_get_now() - g_t0;
  return uint32_t(uint64_t(real) + g_offset + 1000);  // never 0 (the firmware uses 0 as "unset")
}

void advance(uint32_t ms) { g_offset += ms; }
void useFakeClock(bool) {}
}  // namespace simhost

// ---- Arduino / ESP-IDF bits (shim/fw/Arduino.h)
uint32_t millis() { return simhost::now(); }
void delay(uint32_t) {}  // never blocks the page (the drawing code doesn't call it)
int digitalRead(int) { return HIGH; }
uint32_t esp_random() {
  static std::mt19937 rng(uint32_t(emscripten_get_now() * 1000.0));
  return rng();
}
HostSerial Serial = {true};
HostEsp ESP;
HostWiFi WiFi;
void HostSerial::printf(const char* fmt, ...) {
  if (quiet) return;
  va_list ap;
  va_start(ap, fmt);
  std::vprintf(fmt, ap);
  va_end(ap);
}

// ---- lcd.h
namespace lcd {
namespace {
LGFX_Sprite g_canvas;
bool g_ok = false;
bool g_powered = true;
bool g_asleep = false;
bool g_lightOff = false;
int g_brightness = 60, g_level = 60;  // lcd.cpp's defaults
constexpr int kDimPercent = 10;      // lcd.cpp: the "dimmed" level
IdlePolicy g_idle;
}  // namespace

bool begin() {
  if (!g_ok) {
    g_canvas.setColorDepth(16);
    g_ok = g_canvas.createSprite(kW, kH) != nullptr;
  }
  g_powered = true;
  return g_ok;
}
bool ok() { return g_ok; }
void powerOff() {
  g_level = 0;
  g_powered = false;
}
bool powered() { return g_powered; }
void sleepPanel() { g_asleep = true; }
void wake() { g_asleep = false; }
bool asleep() { return g_asleep; }
LGFX_Sprite& canvas() { return g_canvas; }

// The canvas holds RGB565 byte-swapped (panel order), as on the board.
void flush() {
  wake();
  const uint16_t* p = static_cast<const uint16_t*>(g_canvas.getBuffer());
  for (int i = 0; i < kW * kH; ++i) simhost::frame[i] = uint16_t((p[i] >> 8) | (p[i] << 8));
  ++simhost::flushes;
}
uint32_t lastFlushMs() { return 22; }  // what the board measures at 40 MHz
void pushRaw(int y, const uint16_t* pixels, int h) {
  wake();
  for (int r = 0; r < h && y + r < kH; ++r)
    for (int x = 0; x < kW; ++x) {
      const uint16_t v = pixels[r * kW + x];
      simhost::frame[(y + r) * kW + x] = uint16_t((v >> 8) | (v << 8));
    }
  ++simhost::flushes;
}
void fill(uint16_t c) {
  for (auto& v : simhost::frame) v = c;
  ++simhost::flushes;
}
void setBrightness(int percent, bool) {
  if (percent < 10) percent = 10;
  if (percent > 100) percent = 100;
  g_brightness = percent;
  setLevel(percent);
}
int brightness() { return g_brightness; }
void setLevel(int percent) {
  if (percent > g_brightness) percent = g_brightness;
  if (percent < 0) percent = 0;
  g_level = percent;
}
int level() { return g_level; }
const IdlePolicy& idlePolicy() { return g_idle; }
void setIdlePolicy(const IdlePolicy& p, bool) { g_idle = p; }
// lcd.cpp's serviceIdle(), unchanged in behaviour.
bool serviceIdle(uint32_t idleMs, bool keepOn) {
  if (!g_powered) return false;
  int want = g_brightness;
  bool off = false, sleepNow = false;
  if (!keepOn) {
    if (g_idle.sleepSeconds > 0 && idleMs >= uint32_t(g_idle.sleepSeconds) * 1000u) sleepNow = off = true;
    else if (g_idle.offSeconds > 0 && idleMs >= uint32_t(g_idle.offSeconds) * 1000u) off = true;
    else if (g_idle.dimSeconds > 0 && idleMs >= uint32_t(g_idle.dimSeconds) * 1000u) want = kDimPercent;
  }
  if (off) want = 0;
  if (sleepNow) sleepPanel();
  else if (g_asleep) wake();
  if (want != g_level) setLevel(want);
  g_lightOff = off;
  return off;
}
bool lightOff() { return g_lightOff; }
}  // namespace lcd

namespace simhost {
int lightLevel() { return lcd::powered() ? lcd::level() : 0; }
bool lightIsOff() { return lcd::lightOff(); }
}  // namespace simhost
