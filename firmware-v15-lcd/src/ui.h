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

// Layout (pixels): status bar on top, soft-key bar at the bottom, content between.
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
// The next show*() redraws whatever it is given (after the viewfinder owned the panel).
void invalidate();

// Colours shared with the viewfinder overlay and the self-test.
namespace color {
constexpr uint16_t bg = 0x0861;       // near black (#101418)
constexpr uint16_t bar = 0x1986;      // status / soft-key bar (#1c2430)
constexpr uint16_t text = 0xEF7D;     // main text (#e8eef4)
constexpr uint16_t dim = 0x8D30;      // secondary text (#8fa3b8)
constexpr uint16_t accent = 0x3E1F;   // highlight (#39c3ff)
constexpr uint16_t result = 0x7FD3;   // results (#7cfc9a)
constexpr uint16_t warn = 0xFE69;     // amber (#ffcc4d)
constexpr uint16_t error = 0xB105;    // red (#b0202a)
constexpr uint16_t good = 0x2E88;     // green (#2ad144)
constexpr uint16_t black = 0x0000;
constexpr uint16_t white = 0xFFFF;
}  // namespace color

// Draws the status bar on the canvas (used by the viewfinder too). `decodedRow0` may be
// null (no core screen: the firmware's own pages).
void drawStatusBar(const Status& st, const char* leftText);

}  // namespace ui
