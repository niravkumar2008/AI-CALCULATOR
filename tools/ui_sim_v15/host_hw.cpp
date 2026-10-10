// The hardware the UI code talks to, replaced for the PC: the lcd:: module (lcd.cpp on
// the board drives the ST7789 over SPI; here a flush copies the real LGFX_Sprite canvas
// into host::frame), a fake clock, Serial, ESP and WiFi.
#include <cstdio>
#include <vector>

#include "Arduino.h"
#include "WiFi.h"
#include "host_capture.h"
#include "lcd.h"

namespace host {
uint16_t frame[kW * kH];
int flushes = 0;
std::string outDir = ".";
uint32_t nowMs = 0;
static std::string g_auto;

void save(const std::string& name) {
  const std::string path = outDir + "/" + name + ".ppm";
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) {
    std::fprintf(stderr, "can't write %s\n", path.c_str());
    return;
  }
  std::fprintf(f, "P6\n%d %d\n255\n", kW, kH);
  std::vector<uint8_t> rgb(size_t(kW) * kH * 3);
  for (int i = 0; i < kW * kH; ++i) {
    const uint16_t v = frame[i];
    const int r = (v >> 11) & 0x1F, g = (v >> 5) & 0x3F, b = v & 0x1F;
    rgb[i * 3 + 0] = uint8_t((r << 3) | (r >> 2));
    rgb[i * 3 + 1] = uint8_t((g << 2) | (g >> 4));
    rgb[i * 3 + 2] = uint8_t((b << 3) | (b >> 2));
  }
  std::fwrite(rgb.data(), 1, rgb.size(), f);
  std::fclose(f);
  std::printf("  %s\n", name.c_str());
}

void autoSave(const std::string& prefix) { g_auto = prefix; }

static void pushed() {
  ++flushes;
  if (!g_auto.empty()) {
    char n[16];
    std::snprintf(n, sizeof n, "_%02d", flushes);
    save(g_auto + n);
  }
}
}  // namespace host

// ---- Arduino / ESP-IDF bits
uint32_t millis() { return host::nowMs; }
void delay(uint32_t ms) { host::nowMs += ms; }
int digitalRead(int) { return HIGH; }
uint32_t esp_random() { return 0x3F7A5C21u; }
HostSerial Serial;
HostEsp ESP;
HostWiFi WiFi;
void HostSerial::printf(const char* fmt, ...) {
  if (quiet) return;
  va_list ap;
  va_start(ap, fmt);
  std::vprintf(fmt, ap);
  va_end(ap);
}

// ---- lcd.h, as far as the drawing code uses it
namespace lcd {
namespace {
LGFX_Sprite g_canvas;
bool g_ok = false;
int g_brightness = 60, g_level = 60;
IdlePolicy g_policy;
}  // namespace

bool begin() {
  if (!g_ok) {
    g_canvas.setColorDepth(16);
    g_ok = g_canvas.createSprite(kW, kH) != nullptr;
  }
  return g_ok;
}
bool ok() { return g_ok; }
void powerOff() {}
bool powered() { return g_ok; }
void sleepPanel() {}
void wake() {}
bool asleep() { return false; }
LGFX_Sprite& canvas() { return g_canvas; }

// The canvas holds RGB565 byte-swapped (panel order), as on the board.
void flush() {
  const uint16_t* p = static_cast<const uint16_t*>(g_canvas.getBuffer());
  for (int i = 0; i < kW * kH; ++i) host::frame[i] = uint16_t((p[i] >> 8) | (p[i] << 8));
  host::pushed();
}
uint32_t lastFlushMs() { return 22; }  // what the board measures at 40 MHz
void pushRaw(int y, const uint16_t* pixels, int h) {
  for (int r = 0; r < h && y + r < kH; ++r)
    for (int x = 0; x < kW; ++x) {
      const uint16_t v = pixels[r * kW + x];
      host::frame[(y + r) * kW + x] = uint16_t((v >> 8) | (v << 8));
    }
  host::pushed();
}
void fill(uint16_t c) {
  for (auto& v : host::frame) v = c;
  host::pushed();
}
void setBrightness(int percent, bool) { g_brightness = g_level = percent; }
int brightness() { return g_brightness; }
void setLevel(int percent) { g_level = percent; }
int level() { return g_level; }
const IdlePolicy& idlePolicy() { return g_policy; }
void setIdlePolicy(const IdlePolicy& p, bool) { g_policy = p; }
bool serviceIdle(uint32_t, bool) { return false; }
bool lightOff() { return false; }
}  // namespace lcd
