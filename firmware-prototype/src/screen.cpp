#include "screen.h"

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <driver/gpio.h>

#include "pins.h"
#include "viewfinder.h"

using calc::Framebuffer;

namespace {
// Waveshare 2.13" V4 = Good Display GDEY0213B74 (SSD1680), 250x122.
GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> g_epd(
    GxEPD2_213_GDEY0213B74(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));

constexpr int kScale = 2;
constexpr int kFullEvery = 30;  // partial refreshes between full refreshes
constexpr uint32_t kEpdSpiHz = 4000000;
// What's on the glass, kept through deep sleep (RTC memory) so waking up to read a
// key doesn't redraw an unchanged picture. 0 = unknown (cold start).
RTC_DATA_ATTR uint32_t g_shownHash = 0;
RTC_DATA_ATTR int g_partials = kFullEvery;  // force a full refresh first
bool g_wakeRefresh = false;                 // first change after a wake: full refresh

uint32_t fnv1a(const std::string& s) {
  uint32_t h = 2166136261u;
  for (unsigned char c : s) h = (h ^ c) * 16777619u;
  return h ? h : 1;
}

// The 125x61 screen at 2x fills the 250x122 panel exactly. With the "LCD
// dots" look each pixel is drawn as a 2x2 dot missing one corner, a close
// e-paper stand-in for the Casio's dot-matrix gaps.
void draw(const Framebuffer& fb, bool dots) {
  g_epd.fillScreen(GxEPD_WHITE);
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) {
      if (!fb.get(x, y)) continue;
      const int px = x * kScale, py = y * kScale;
      if (dots) {
        g_epd.drawPixel(px, py, GxEPD_BLACK);
        g_epd.drawPixel(px + 1, py, GxEPD_BLACK);
        g_epd.drawPixel(px, py + 1, GxEPD_BLACK);
      } else {
        g_epd.fillRect(px, py, kScale, kScale, GxEPD_BLACK);
      }
    }
}
}  // namespace

void screenBegin(bool wokeFromSleep) {
  SPI.begin(PIN_EPD_CLK, -1, PIN_EPD_DIN, PIN_EPD_CS);
  // BUSY is driven push-pull by the SSD1680, so a weak pull-down changes nothing with a
  // panel attached. Without one (bench, before J2 is fitted) it keeps BUSY from floating
  // high, where every draw would wait the driver's 10 s timeout (review S4). The
  // library's init() re-runs pinMode(INPUT), so the pull is enabled again after it.
  pinMode(PIN_EPD_BUSY, INPUT_PULLDOWN);
  // 2 ms reset pulse and no pull-down on RST: what Waveshare's boards need.
  g_epd.init(0, !wokeFromSleep, 2, false, SPI, SPISettings(kEpdSpiHz, MSBFIRST, SPI_MODE0));
  gpio_pulldown_en(gpio_num_t(PIN_EPD_BUSY));
  g_epd.setRotation(1);  // landscape, 250 x 122
  g_epd.setTextColor(GxEPD_BLACK);
  if (!wokeFromSleep) {
    g_shownHash = 0;
    g_partials = kFullEvery;
  } else {
    // The controller lost its RAM in deep sleep: the next change is drawn in full
    // (a partial update needs the previous picture in the controller).
    g_wakeRefresh = true;
  }
}

void screenShow(const Framebuffer& fb, bool dots) {
  const uint32_t h = fnv1a(fb.toAscii() + (dots ? "d" : "p"));
  if (h == g_shownHash) return;
  g_shownHash = h;
  if (g_partials >= kFullEvery || g_wakeRefresh) {
    g_epd.setFullWindow();
    g_partials = 0;
    g_wakeRefresh = false;
  } else {
    g_epd.setPartialWindow(0, 0, g_epd.width(), g_epd.height());
    ++g_partials;
  }
  g_epd.firstPage();
  do draw(fb, dots);
  while (g_epd.nextPage());
  g_epd.hibernate();  // panel deep sleep: keeps the picture with no power
}

uint32_t screenTestPattern(const Framebuffer& fb) {
  g_epd.setFullWindow();
  const uint32_t t0 = millis();
  g_epd.firstPage();
  do {
    g_epd.fillScreen(GxEPD_WHITE);
    // 10 px checkerboard on the top half (dead lines and stuck pixels show),
    // a border (panel edges and alignment), the framebuffer text underneath.
    for (int y = 0; y < 50; y += 10)
      for (int x = 0; x < g_epd.width(); x += 10)
        if (((x + y) / 10) % 2 == 0) g_epd.fillRect(x, y, 10, 10, GxEPD_BLACK);
    g_epd.drawRect(0, 0, g_epd.width(), g_epd.height(), GxEPD_BLACK);
    for (int y = 30; y < Framebuffer::kHeight; ++y)
      for (int x = 0; x < Framebuffer::kWidth; ++x)
        if (fb.get(x, y)) g_epd.fillRect(x * kScale, y * kScale, kScale, kScale, GxEPD_BLACK);
  } while (g_epd.nextPage());
  const uint32_t ms = millis() - t0;
  g_epd.hibernate();
  g_shownHash = 0;          // whatever comes next is a change
  g_partials = kFullEvery;  // and is drawn in full
  return ms;
}

// ---- camera viewfinder
namespace {
bool g_panelShown = false;  // the viewfinder is on the glass (not the calculator screen)
}

void screenShowPanel(const calc::Panel& p, bool full) {
  if (full) {
    g_epd.setFullWindow();
  } else {
    g_epd.setPartialWindow(0, 0, g_epd.width(), g_epd.height());
  }
  g_epd.firstPage();
  do {
    g_epd.fillScreen(GxEPD_WHITE);
    g_epd.drawBitmap(0, 0, p.bits(), calc::Panel::kWidth, calc::Panel::kHeight, GxEPD_BLACK);
  } while (g_epd.nextPage());
  // No hibernate between frames: waking the controller costs a reset and ~50 ms.
  g_panelShown = true;
}

void screenEndPanel() {
  if (!g_panelShown) return;
  g_panelShown = false;
  g_epd.hibernate();
  g_shownHash = 0;          // the calculator screen is redrawn...
  g_partials = kFullEvery;  // ...in full, which also clears the viewfinder's ghosting
}
