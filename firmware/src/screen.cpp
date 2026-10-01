#include "screen.h"

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>

#include "pins.h"

using calc::Framebuffer;

namespace {
// Waveshare 2.13" V4 = Good Display GDEY0213B74 (SSD1680), 250x122.
GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> g_epd(
    GxEPD2_213_GDEY0213B74(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));

constexpr int kScale = 2;
constexpr int kFullEvery = 30;  // partial refreshes between full refreshes
std::string g_shown;            // what's on the glass now (Framebuffer::toAscii)
int g_partials = kFullEvery;    // force a full refresh first

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

void screenBegin() {
  SPI.begin(PIN_EPD_CLK, -1, PIN_EPD_DIN, PIN_EPD_CS);
  // 2 ms reset pulse and no pull-down on RST: what Waveshare's boards need.
  g_epd.init(0, true, 2, false, SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  g_epd.setRotation(1);  // landscape, 250 x 122
  g_epd.setTextColor(GxEPD_BLACK);
}

void screenShow(const Framebuffer& fb, bool dots) {
  std::string now = fb.toAscii() + (dots ? "d" : "p");
  if (now == g_shown) return;
  g_shown = now;
  if (g_partials >= kFullEvery) {
    g_epd.setFullWindow();
    g_partials = 0;
  } else {
    g_epd.setPartialWindow(0, 0, g_epd.width(), g_epd.height());
    ++g_partials;
  }
  g_epd.firstPage();
  do draw(fb, dots);
  while (g_epd.nextPage());
  g_epd.hibernate();  // panel keeps the picture with no power
}
