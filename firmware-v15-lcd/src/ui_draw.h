// Internal to the LCD UI (ui.cpp, ui_apps.cpp, ui_geo.cpp, ui_periodic.cpp): the two
// looks, shared drawing helpers and the per-app screens.
//
// Two looks (owner decision, docs/MODE4_APPS_SPEC.md):
//  * LCD: the calculator (COMP, MODE, SETUP, errors, exam mode) looks like a classic mono
//    calculator LCD: flat grey-green background, near-black ink, no colour accents.
//  * Colour: the Apps menu and every app (AI Solve, Type it, History, Notes, Geography,
//    Periodic Table, Settings): navy #10141c, brand orange #e07a4f, white / grey text.
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "device.h"
#include "glyphdraw.h"
#include "lcd.h"
#include "ui.h"

namespace ui {

namespace gd = glyphdraw;

struct Palette {
  bool mono;
  uint16_t bg;       // the screen
  uint16_t bar;      // status / soft-key bars
  uint16_t text;     // main text
  uint16_t dim;      // secondary text
  uint16_t accent;   // highlights, selection
  uint16_t onAccent; // text on an accent fill
  uint16_t result;   // calculator results
  uint16_t warn, error, good;
  uint16_t panel;    // cards, tiles, unselected keys
  uint16_t panel2;   // a second, lighter panel shade
  uint16_t rule;     // thin separators
};

const Palette& pal();
void usePalette(bool mono);

constexpr int kW = lcd::kW, kH = lcd::kH;
constexpr int kMarginX = 10;
constexpr int kTextScale = 2;
constexpr int kContentBottom = kH - kSoftH - 2;  // last content row + 1 (148)

using Keys = std::vector<std::pair<std::string, std::string>>;

// ---- shared pieces (ui.cpp)
// The soft-key bar: [key] what, ... (measured; a group that would not fit is left out).
void softKeys(const Keys& keys);
// The right end of the status bar (battery, %, Wi-Fi). Returns the x where it starts.
int statusCluster(const Status& st, bool onDark);
// The status bar of an app screen: its title on the left, SHIFT / ALPHA badges, the
// battery and Wi-Fi cluster on the right, and an optional `right` text before it.
void appStatusBar(const Status& st, const std::string& title, const std::string& right, bool shift, bool alpha);
// A thin scroll bar on the right edge: `total` rows, `visible` shown from `first`.
void scrollBar(int y, int h, int first, int visible, int total);
// A one-line pop-up near the bottom (Device::toast()): a rounded pill.
void toastPill(const std::string& text);
// Text that must fit in maxW px at `scale`: normal, tight, or cut with "..".
int fitText(int x, int y, const std::string& utf8, int scale, uint16_t color, int maxW);
// The text of one row of the core's own (mono) render of this screen, as decoded.
std::string coreRowText(int row);

// A list of rows (History, Notes, Settings, the apps' lists): 5 rounded rows of 22 px from
// kListY, the selected one filled orange. `right` is drawn right-aligned in the row.
struct ListRow {
  std::string left, right;
  uint16_t leftColor = 0, rightColor = 0;  // 0 = the palette's text / dim
  int swatch = -1;                         // >= 0: a small colour chip before the text (RGB565)
};
constexpr int kListY = kContentY + 2;
constexpr int kListPitch = 24;
void drawList(const std::vector<ListRow>& rows, int selected, int top, int visible = 5);

// ---- the apps (ui_apps.cpp, ui_geo.cpp, ui_periodic.cpp). Each draws the content and
// the soft-key bar; the status bar is drawn by the caller with appTitle().
struct Frame {
  const calc::Device& dev;
  const Status& st;
  uint32_t nowMs;
  bool shift, alpha;
};
std::string appTitle(const calc::Device& dev, std::string& right);  // status-bar title (+ right text)
void drawAppsMenu(const Frame& f);
void drawKeyboard(const Frame& f);
void drawHistory(const Frame& f);
void drawNotes(const Frame& f);
void drawNoteView(const Frame& f);
void drawSettings(const Frame& f);
void drawConfirm(const Frame& f);
void drawLink(const Frame& f);
void drawGeo(const Frame& f);
void drawPeriodic(const Frame& f);

// Vector icons for the Apps carousel, drawn into a size x size box centred on (cx, cy).
void appIcon(int app, int cx, int cy, int size);

}  // namespace ui
