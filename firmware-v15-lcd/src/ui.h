// The colour UI for the 320 x 170 LCD. The calculator's logic (core/Device) still renders
// its 1-bit 125x61 picture; this module reads that picture back as text (fbtext.h) and
// re-typesets every screen larger and in colour: a status bar (mode, SHIFT/ALPHA, angle
// unit, Wi-Fi, battery), the calculator with a large expression and result, the menus,
// SETUP, the AI answer pages, notices, and the firmware's own screens (setup by phone,
// updates, self-test). core/ does not know which display it is on.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "device.h"
#include "framebuffer.h"

namespace ui {

struct Status {
  int battery = -1;          // 0-100, -1 unknown
  bool charging = false;
  bool wifiOn = false;       // radio on (joining)
  bool wifiConnected = false;
  bool lowBattery = false;   // AI lock-out
};

// Layout (pixels) of the Apps look: status bar on top, soft-key bar at the bottom, content
// between. The calculator (LCD look) has its own Casio layout in ui.cpp: a thin indicator
// row and no soft-key bar (owner, 2026-10-11).
constexpr int kStatusH = 22;
constexpr int kSoftH = 20;
constexpr int kContentY = kStatusH + 2;
constexpr int kContentH = 170 - kStatusH - kSoftH - 4;

void begin();
// Draws the Device's screen (its render in `fb`) if it changed since the last call, or
// to blink the cursor. flush = true forces a redraw.
void showDevice(const calc::Device& dev, const calc::Framebuffer& fb, const Status& st, uint32_t nowMs,
                bool force = false);
// The firmware's own text screens: lines[0] is the title. Redrawn only when the text changes.
void showLines(const std::vector<std::string>& lines, const Status& st);
// A progress screen (firmware update): title, two text lines, 0-100 %.
void showProgress(const std::string& title, const std::string& line1, const std::string& line2, int percent,
                  const Status& st);
// The Proofline loading screen: logo, "AI CALCULATOR", a 0-100 % bar and the step being
// done ("" = none). setup() calls it on a cold start; showDevice() shows it whenever the
// core's splash is up (Device::splashShowing(), e.g. for a moment after ON).
void showSplash(int percent, const std::string& step);
// The next show*() redraws whatever it is given (after the viewfinder owned the panel).
void invalidate();

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return uint16_t(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// The brand colours (the Apps look: navy and orange), shared with the viewfinder overlay
// and the self-test. The calculator itself is drawn in the classic mono "LCD" look
// (ui.cpp: Palette); full colour is only inside the Apps menu (docs/MODE4_APPS_SPEC.md).
namespace color {
constexpr uint16_t bg = rgb565(0x10, 0x14, 0x1c);      // navy (#10141c)
constexpr uint16_t bar = rgb565(0x1a, 0x21, 0x30);     // status / soft-key bar (#1a2130)
constexpr uint16_t text = rgb565(0xee, 0xf2, 0xf6);    // main text (#eef2f6)
constexpr uint16_t dim = rgb565(0x93, 0xa1, 0xb3);     // secondary text (#93a1b3), 7:1 on bg
constexpr uint16_t accent = rgb565(0xe0, 0x7a, 0x4f);  // brand orange (#e07a4f)
constexpr uint16_t result = rgb565(0xff, 0xff, 0xff);  // results
constexpr uint16_t warn = rgb565(0xf5, 0xb7, 0x4f);    // amber (#f5b74f)
constexpr uint16_t error = rgb565(0xd6, 0x3a, 0x3f);   // red (#d63a3f)
constexpr uint16_t good = rgb565(0x4c, 0xc3, 0x8a);    // green (#4cc38a)
constexpr uint16_t black = 0x0000;
constexpr uint16_t white = 0xFFFF;
}  // namespace color

// Draws the status bar on the canvas (used by the viewfinder too). `decodedRow0` may be
// null (no core screen: the firmware's own pages).
void drawStatusBar(const Status& st, const char* leftText);

}  // namespace ui
