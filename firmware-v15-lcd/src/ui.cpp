#include "ui.h"

#include <Arduino.h>

#include "calc_engine.h"
#include "fbtext.h"
#include "font.h"
#include "glyphdraw.h"
#include "lcd.h"
#include "text.h"
#include "ui_draw.h"

using calc::Device;
using calc::Framebuffer;
using calc::View;

namespace ui {

// ---------------------------------------------------------------- the two looks

namespace {

// The classic mono calculator LCD: flat grey-green glass, near-black ink (10:1), a slightly
// lighter ink for secondary text (7.3:1). Every "colour" role maps onto the same ink.
constexpr uint16_t kLcdBg = rgb565(0xC7, 0xD1, 0xB5);
constexpr uint16_t kLcdInk = rgb565(0x1E, 0x23, 0x18);
constexpr uint16_t kLcdInk2 = rgb565(0x34, 0x3B, 0x2C);
const Palette kLcd = {
    true,     kLcdBg,   kLcdBg,   kLcdInk, kLcdInk2, kLcdInk, kLcdBg, kLcdInk, kLcdInk, kLcdInk, kLcdInk,
    rgb565(0xB6, 0xC1, 0xA2), rgb565(0xAB, 0xB6, 0x96), rgb565(0x8F, 0x9A, 0x7E)};

// The Apps look: the brand's navy and orange.
const Palette kColour = {false,
                         color::bg,
                         color::bar,
                         color::text,
                         color::dim,
                         color::accent,
                         color::bg,
                         color::result,
                         color::warn,
                         color::error,
                         color::good,
                         rgb565(0x1d, 0x25, 0x34),
                         rgb565(0x28, 0x32, 0x45),
                         rgb565(0x2c, 0x36, 0x48)};

const Palette* g_pal = &kColour;

}  // namespace

const Palette& pal() { return *g_pal; }
void usePalette(bool mono) { g_pal = mono ? &kLcd : &kColour; }

namespace {

constexpr int kRowPitch = kContentH / 6;  // 6 content rows (core rows 1-6), 20 px each
// The calculator (LCD look) is laid out like the Casio fx-ES: a thin indicator row on top
// (S A M STO RCL D/R/G, ▲▼, nothing else), no soft-key bar, the content down to the bottom.
constexpr int kCasioRowH = 18;               // the indicator row (scale-2 glyphs, 14 px)
constexpr int kCalcTop = kCasioRowH + 4;     // 22: the first content pixel
constexpr int kCalcBottom = kH - 4;          // 166: the last content pixel + 1
constexpr int kCalcPitch = (kCalcBottom - kCalcTop) / 6;  // 24 px a core row
constexpr int kExprScale = 3;             // 15 x 21 px for the expression
constexpr int kExprCols = (kW - 2 * kMarginX) / (gd::kGlyphW * kExprScale + 1);  // 18 chars per line at 3x
constexpr int kExprLines = 4;
constexpr uint32_t kBlinkMs = 500;

uint32_t g_shownHash = 0;
fbtext::Grid g_grid;
const Framebuffer* g_fb = nullptr;  // the core picture being shown (for decodeScaled)
bool g_appsLook = false;            // the last screen was an app's (sticky for the shared info pages)

uint32_t fnv1a(const std::string& s, uint32_t h = 2166136261u) {
  for (unsigned char c : s) h = (h ^ c) * 16777619u;
  return h ? h : 1;
}

std::string statusKey(const Status& st) {
  return std::to_string(st.battery) + (st.charging ? "c" : "") + (st.wifiOn ? "w" : "") +
         (st.wifiConnected ? "W" : "") + (st.lowBattery ? "L" : "");
}

// Which look a view gets. The info pages shared by SETUP and the Settings app (Wi-Fi &
// key, Screen look, Exam mode, notices) keep the look of the screen they were opened from.
enum class Look { Lcd, Colour, Same };
Look lookOf(View v) {
  switch (v) {
    case View::Apps: case View::Keyboard: case View::History: case View::Notes: case View::NoteView:
    case View::Settings: case View::Confirm: case View::Geo: case View::Periodic: case View::Link:
    case View::Ai:  // AI Solve (and Type it / History answers) only open from the Apps menu
      return Look::Colour;
    case View::Notice: case View::NetInfo: case View::Look: case View::ExamInfo:
      return Look::Same;
    default:  // Calc, Error, ModeMenu, Setup, SetupFreq, SetupCmplx, Hyp, CmplxMenu, BaseMenu, Clr, ClrConfirm,
              // Stat, Table, Eqn, MatVec, CalcRun, SolveRun, Off (BASE-N is View::Calc: drawCalc() sizes its
              // 16-digit BIN result like any long one; its Dec / Hex / Bin / Oct is the mode tag; MATRIX /
              // VECTOR are View::Calc too, MAT / VCT the mode tag, a matrix result an empty result line)
      return Look::Lcd;
  }
}

// ---- status bar
void batteryIcon(LGFX_Sprite& s, int x, int y, const Status& st, bool onDark = false) {
  // 24 x 12 outline with a tip; the fill shows what's left. Colour look: red under 20 %,
  // orange on a cable. LCD look: all ink (the exam bar: LCD green on ink).
  const Palette& p = pal();
  const uint16_t frame = p.mono ? (onDark ? p.bg : p.text) : p.dim;
  s.drawRect(x, y, 22, 12, frame);
  s.fillRect(x + 22, y + 3, 2, 6, frame);
  if (st.battery < 0) return;
  const int pct = st.battery > 100 ? 100 : st.battery;
  const uint16_t fill = p.mono ? frame : st.charging ? p.accent : pct <= 20 ? p.error : p.good;
  const int w = (18 * pct + 50) / 100;
  if (w > 0) s.fillRect(x + 2, y + 2, w, 8, fill);
  if (st.charging) {  // a small bolt
    const uint16_t bolt = p.mono ? (onDark ? p.text : p.bg) : color::white;
    s.fillRect(x + 9, y + 2, 2, 4, bolt);
    s.fillRect(x + 11, y + 5, 2, 2, bolt);
    s.fillRect(x + 9, y + 7, 2, 3, bolt);
  }
}

void wifiIcon(LGFX_Sprite& s, int x, int y, const Status& st, bool onDark) {
  if (!st.wifiOn && !st.wifiConnected) return;
  const Palette& p = pal();
  const uint16_t c = p.mono ? (onDark ? p.bg : p.text) : st.wifiConnected ? p.accent : p.dim;
  const uint16_t hollow = p.mono ? (onDark ? p.text : p.bg) : p.bar;
  // three bars of growing height
  s.fillRect(x, y + 8, 3, 4, c);
  s.fillRect(x + 4, y + 5, 3, 7, st.wifiConnected ? c : hollow);
  s.fillRect(x + 8, y + 2, 3, 10, st.wifiConnected ? c : hollow);
  if (!st.wifiConnected) {
    s.drawRect(x + 4, y + 5, 3, 7, c);
    s.drawRect(x + 8, y + 2, 3, 10, c);
  }
}

}  // namespace

// The right end of the status bar, laid out right to left from measured widths: the
// battery icon at the edge, its % before it, the Wi-Fi bars before that. Returns the x
// where the cluster starts (anything on the left must end before it). onDark: the exam bar.
int statusCluster(const Status& st, bool onDark) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  constexpr int kBatW = 24, kWifiW = 11, kGap = 5;
  int x = kW - 4 - kBatW;
  batteryIcon(s, x, 5, st, onDark);
  if (st.battery >= 0) {
    const std::string pct = std::to_string(st.battery > 100 ? 100 : st.battery) + "%";
    const int adv = gd::tight(kTextScale);
    uint16_t col = p.dim;
    if (p.mono) col = onDark ? p.bg : p.text;
    else if (onDark) col = color::white;
    else if (st.battery <= 20 && !st.charging) col = p.error;
    gd::textRight(s, x - kGap + 2, 4, pct, kTextScale, col, adv);  // +2: the font's own gap column
    x -= kGap + gd::width(pct, kTextScale, adv) - 2;
  }
  if (st.wifiOn || st.wifiConnected) {
    x -= kGap + 1 + kWifiW;
    wifiIcon(s, x, 5, st, onDark);
  }
  return x;
}

void drawStatusBar(const Status& st, const char* leftText) {
  LGFX_Sprite& s = lcd::canvas();
  s.fillRect(0, 0, kW, kStatusH, pal().bar);
  const int right = statusCluster(st, false);
  if (leftText) gd::textFit(s, kMarginX, 4, leftText, kTextScale, pal().text, right - 6 - kMarginX);
}

std::string coreRowText(int row) { return row >= 0 && row < fbtext::kRows ? g_grid.text(row) : std::string(); }

int fitText(int x, int y, const std::string& utf8, int scale, uint16_t color, int maxW) {
  return gd::textFit(lcd::canvas(), x, y, utf8, scale, color, maxW);
}

void appStatusBar(const Status& st, const std::string& title, const std::string& right, bool shift, bool alpha) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  s.fillRect(0, 0, kW, kStatusH, p.bar);
  s.fillRect(0, kStatusH - 2, kW, 2, p.accent);  // the apps' orange rule
  int x = kMarginX;
  // SHIFT / ALPHA: a small badge before the title (the keys they modify still work in apps).
  auto badge = [&](const char* t, uint16_t c) {
    s.fillRoundRect(x - 2, 3, gd::tight(kTextScale) + 3, 15, 3, c);
    gd::text(s, x, 4, t, kTextScale, p.bg, gd::tight(kTextScale));
    x += gd::tight(kTextScale) + 6;
  };
  if (shift) badge("S", p.warn);
  if (alpha) badge("A", p.error);
  const int clusterX = statusCluster(st, false);
  int titleEnd = clusterX - 8;
  if (!right.empty()) {
    const int rw = gd::width(right, kTextScale, gd::tight(kTextScale));
    gd::text(s, clusterX - 10 - rw, 4, right, kTextScale, p.dim, gd::tight(kTextScale));
    titleEnd = clusterX - 18 - rw;
  }
  gd::textFit(s, x, 4, title, kTextScale, p.text, titleEnd - x);
}

void scrollBar(int y, int h, int first, int visible, int total) {
  if (total <= visible || total <= 0) return;
  LGFX_Sprite& s = lcd::canvas();
  const int x = kW - 5;
  s.fillRoundRect(x, y, 3, h, 1, pal().panel2);
  int th = h * visible / total;
  if (th < 8) th = 8;
  const int ty = y + (h - th) * first / (total - visible);
  s.fillRoundRect(x, ty, 3, th, 1, pal().accent);
}

void toastPill(const std::string& text) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  const int tw = gd::width(text, kTextScale);
  int w = tw + 30;
  if (w > kW - 16) w = kW - 16;
  // LCD look: no soft-key bar under it, so it sits at the bottom of the glass.
  const int h = 24, x = (kW - w) / 2, y = (p.mono ? kCalcBottom : kContentBottom) - h - 6;
  const uint16_t fill = p.mono ? p.text : rgb565(0x2e, 0x38, 0x4c);
  s.fillRoundRect(x + 1, y + 2, w, h, h / 2, p.mono ? p.dim : color::black);  // a little shadow
  s.fillRoundRect(x, y, w, h, h / 2, fill);
  if (!p.mono) s.drawRoundRect(x, y, w, h, h / 2, p.accent);
  s.fillCircle(x + 12, y + h / 2, 3, p.mono ? p.bg : p.accent);
  gd::textFit(s, x + 20, y + 5, text, kTextScale, p.mono ? p.bg : color::white, w - 26);
}

// The soft-key bar: [key] what, ... Measured first: the normal spacing if everything fits,
// else the tight advance and smaller gaps; a key that still would not fit whole is left
// out (never cut in half at the edge). LCD look: the key names as dark outlined labels.
void softKeys(const Keys& keys) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  const int y = kH - kSoftH;
  s.fillRect(0, y, kW, kSoftH, p.bar);
  if (p.mono) s.fillRect(0, y, kW, 1, p.rule);
  const int avail = kW - 2 * kMarginX;
  int adv = gd::advance(kTextScale), gap = 12;
  auto groupW = [&](const std::pair<std::string, std::string>& k) {
    return gd::width(k.first, kTextScale, adv) + 6 + 3 + gd::width(k.second, kTextScale, adv);
  };
  int total = -gap;
  for (const auto& k : keys) total += groupW(k) + gap;
  if (total > avail) {
    adv = gd::tight(kTextScale);
    gap = 8;
  }
  int x = kMarginX;
  for (const auto& k : keys) {
    if (x + groupW(k) > kW - kMarginX + 2) break;  // +2: the last glyph's blank gap column
    const int kw = gd::width(k.first, kTextScale, adv) + 6 - (adv < gd::advance(kTextScale) ? 2 : 0);
    if (p.mono) {
      s.drawRoundRect(x, y + 3, kw, 15, 3, p.text);
      gd::text(s, x + 3, y + 4, k.first, kTextScale, p.text, adv);
    } else {
      s.fillRoundRect(x, y + 3, kw, 14, 3, p.dim);
      gd::text(s, x + 3, y + 3, k.first, kTextScale, color::black, adv);
    }
    x += kw + 3;
    x = gd::text(s, x, y + (p.mono ? 4 : 3), k.second, kTextScale, p.mono ? p.text : p.dim, adv) + gap;
  }
}

namespace {

std::string clockText(uint32_t ms) {  // h:mm, as core's status row shows it
  const uint32_t min = ms / 60000;
  char b[16];
  snprintf(b, sizeof b, "%u:%02u", unsigned(min / 60), unsigned(min % 60));
  return b;
}

// A small battery for the Casio row (13 x 8 + tip), drawn only when the battery is low or a
// cable is in; a little lightning bolt before it while charging.
void casioBattery(LGFX_Sprite& s, int x, int y, const Status& st, uint16_t ink) {
  s.drawRect(x, y, 13, 8, ink);
  s.fillRect(x + 13, y + 2, 2, 4, ink);
  const int pct = st.battery > 100 ? 100 : st.battery;
  const int w = (9 * pct + 50) / 100;
  if (w > 0) s.fillRect(x + 2, y + 2, w, 4, ink);
  if (st.charging) {  // the bolt, 2 px wide, left of the battery
    const int bx = x - 8;
    for (int d = 0; d < 2; ++d) {
      s.drawLine(bx + 4 + d, y - 1, bx + 1 + d, y + 4, ink);
      s.drawLine(bx + 1 + d, y + 4, bx + 4 + d, y + 4, ink);
      s.drawLine(bx + 4 + d, y + 4, bx + 1 + d, y + 9, ink);
    }
  }
}

// The calculator's indicator row, like the Casio fx-115ES / fx-300ES: only what the core's
// status row holds (S, A, M, STO / RCL, D / R / G, cells 0-11, in their columns), small and
// in the LCD ink on the glass itself (no bar, no rule), the ▲▼ scroll indicators on the
// right. No battery %, no Wi-Fi, no "AI", no title: the only extras are a small battery
// when it is low (<= 20 %) or a cable is in. Exam mode keeps its unmistakable inverted bar
// (LCD-green on ink) with the padlock and the clock, so a teacher sees it across a room.
void casioStatus(const Device& dev, const Status& st, bool& up, bool& down) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  const bool exam = dev.examActive();
  const uint16_t ink = exam ? p.bg : p.text;
  if (exam) s.fillRect(0, 0, kW, kCasioRowH, p.text);
  up = g_grid.cell[0][23].cp == 0x25B2;
  down = g_grid.cell[0][24].cp == 0x25BC;
  constexpr int y = 2;
  const int adv = gd::tight(kTextScale);
  // STAT / TABLE / CMPLX / BASE-N's Dec..Oct / MAT / VCT: the core puts the mode's indicator in cells 11-15 (after D/R/G), as the
  // Casio shows STAT on every screen of the mode; the exam clock then moves right of it.
  const bool modeTag = g_grid.cell[0][11].cp != ' ';
  const int cells = modeTag ? 16 : 12;
  for (int c = 0; c < cells; ++c) {
    const fbtext::Cell& cell = g_grid.cell[0][c];
    const int cx = kMarginX + c * (adv + 1);
    if (cell.raw) gd::bits(s, cx, y, cell.rows, kTextScale, ink);
    else if (cell.cp != ' ') gd::glyph(s, cx, y, calc::findGlyph(cell.cp), kTextScale, ink);
  }
  // The right end, right to left: the battery (low / charging only), ▼, ▲, the exam clock.
  int x = kW - kMarginX;
  if (st.battery >= 0 && (st.charging || st.battery <= 20)) {
    x -= 15;
    casioBattery(s, x, 5, st, ink);
    x -= st.charging ? 14 : 6;
  }
  if (down) {
    x -= adv;
    gd::glyph(s, x, y, calc::findGlyph(0x25BC), kTextScale, ink);
    x -= 2;
  }
  if (up) {
    x -= adv;
    gd::glyph(s, x, y, calc::findGlyph(0x25B2), kTextScale, ink);
    x -= 2;
  }
  if (exam) {
    // With STAT / TABLE showing: the padlock and the clock only (the bar itself says exam).
    const std::string tail = std::string("\xEE\x80\x80") + (modeTag ? "" : "EXAM ") + clockText(dev.examElapsedMs());
    const int lo = kMarginX + (modeTag ? 17 : 11) * (adv + 1);  // after the D/R/G cell (or the mode's)
    int tx = x - 6 - gd::width(tail, kTextScale, adv);
    if (tx < lo) tx = lo;
    gd::text(s, tx, y, tail, kTextScale, ink, adv);
  }
}

void scrollArrows(bool up, bool down) {
  LGFX_Sprite& s = lcd::canvas();
  if (up) gd::glyph(s, kW - 14, kContentY + 2, calc::findGlyph(0x25B2), kTextScale, pal().accent);
  if (down) gd::glyph(s, kW - 14, kContentY + kContentH - 16, calc::findGlyph(0x25BC), kTextScale, pal().accent);
}

// Colour for symbols in decoded text: the verified tick green, the warning amber (colour look).
uint16_t symbolColor(uint32_t cp, uint16_t textColor) {
  if (pal().mono) return textColor;
  if (cp == 0x2713) return pal().good;
  if (cp == 0x26A0) return pal().warn;
  return textColor;
}

// A header row: the core underlines the whole title, so every character in it has the rule
// under it. A few underlined cells are not a header: they are the top edge of a selection
// mark on the row below (SETUP's 4:Norm1), which reaches one pixel up into this row's gap.
bool rowUnderlined(int row) {
  int chars = 0;
  for (int c = 0; c < fbtext::kCols; ++c) {
    const fbtext::Cell& cell = g_grid.cell[row][c];
    if (cell.cp == ' ' || cell.inverted) continue;
    if (!cell.underline) return false;
    ++chars;
  }
  return chars > 0;
}

// One decoded core row typeset at 2x: inverted cells on an accent block, underlined rows
// as headers.
void drawGridRow(int row, int y, uint16_t textColor) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  const int adv = gd::kGlyphW * kTextScale + 1;
  const bool underline = rowUnderlined(row);
  for (int c = 0; c < fbtext::kCols; ++c) {
    const fbtext::Cell& cell = g_grid.cell[row][c];
    const int x = kMarginX + c * adv;
    if (cell.inverted) s.fillRect(x - 1, y - 2, adv + 1, gd::kGlyphH * kTextScale + 4, p.accent);
    const uint16_t col = cell.inverted ? p.onAccent : textColor;
    if (cell.raw) gd::bits(s, x, y, cell.rows, kTextScale, col);
    else if (cell.cp != ' ')
      gd::glyph(s, x, y, calc::findGlyph(cell.cp), kTextScale, cell.inverted ? col : symbolColor(cell.cp, col));
  }
  if (underline) {
    const int w = gd::width(g_grid.text(row), kTextScale);
    s.fillRect(kMarginX, y + gd::kGlyphH * kTextScale + 1, w, 1, p.accent);
  }
}

bool rowHasRaw(int row) {
  for (int c = 0; c < fbtext::kCols; ++c)
    if (g_grid.cell[row][c].raw) return true;
  return false;
}

// top / pitch: the colour look's rows between its two bars, or the LCD look's taller rows
// down to the bottom of the glass (no soft-key bar there).
void drawGenericRows(uint16_t textColor, int top = kContentY, int pitch = kRowPitch) {
  LGFX_Sprite& s = lcd::canvas();
  for (int r = 1; r < fbtext::kRows; ++r) {
    const int y = top + (r - 1) * pitch + 3;
    // Text the core drew at 2x (the HOLD STILL countdown, drawTextPx one pixel below the
    // row) spans this row and the next: read it at 2x and show it as large amber digits.
    if (g_fb && r + 1 < fbtext::kRows && rowHasRaw(r)) {
      std::string big;
      for (int dy : {1, 0}) {
        big = fbtext::decodeScaled(*g_fb, 0, r * 8 + dy, 2);
        if (!big.empty()) break;
      }
      if (!big.empty()) {
        // "3 s": the number large, its unit smaller on the same baseline.
        std::string unit;
        const size_t sp = big.rfind(' ');
        if (sp != std::string::npos && sp > 0) {
          unit = big.substr(sp + 1);
          big.resize(sp);
        }
        const int x = gd::text(s, kMarginX, y, big, 5, pal().warn);
        if (!unit.empty()) gd::text(s, x + 4, y + gd::kGlyphH * (5 - 3), unit, 3, pal().warn);
        ++r;  // the next row holds the bottom half
        continue;
      }
    }
    const bool header = r == 1 && rowUnderlined(1);
    drawGridRow(r, y, header ? pal().accent : textColor);
  }
}

// ---- the calculator screen: a large expression with the cursor, the result underneath
int cursorCellOf(const Device& dev) {
  int n = 0;
  const auto& e = dev.expression();
  for (int i = 0; i < dev.cursor() && i < int(e.size()); ++i) n += calc::textLength(calc::tokText(e[i]));
  return n;
}

void drawCalc(const Device& dev, uint32_t nowMs) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  const bool editing = !dev.showingResult();
  // The expression as the core laid it out (rows 1-4, windowed around the cursor), re-flowed
  // to the wider glyphs. Rows are full 25 characters when wrapped, so joining them restores
  // the text; the last row's trailing spaces are dropped.
  std::vector<uint32_t> cps;
  int lastRow = 0;
  for (int r = 1; r <= 4; ++r)
    if (!g_grid.rowEmpty(r)) lastRow = r;
  for (int r = 1; r <= lastRow; ++r) {
    std::string t = g_grid.text(r);
    std::vector<uint32_t> row = calc::decodeUtf8(t);
    if (r < lastRow) row.resize(fbtext::kCols, ' ');
    cps.insert(cps.end(), row.begin(), row.end());
  }
  // Where the cursor falls in that text (the core's window starts at `first` rows).
  int cursorIdx = -1;
  if (editing) {
    const int cell = cursorCellOf(dev);
    const int line = cell / fbtext::kCols;
    const int first = line > 3 ? line - 3 : 0;
    cursorIdx = cell - first * fbtext::kCols;
  }
  const int adv = gd::kGlyphW * kExprScale + 1;
  const int lineH = gd::kGlyphH * kExprScale + 3;
  const int top = kCalcTop + 2;  // right under the indicator row, as on the Casio
  const uint16_t exprColor = editing ? p.text : p.dim;
  const int n = int(cps.size());
  const int lines = n / kExprCols + 1;
  const int cursorLine = cursorIdx >= 0 ? cursorIdx / kExprCols : 0;
  int firstLine = cursorLine - (kExprLines - 1);
  if (firstLine < 0) firstLine = 0;
  const int maxLines = editing ? kExprLines : 2;  // with a result shown, two lines of expression fit above it
  if (!editing && lines > maxLines) firstLine = 0;
  for (int l = firstLine; l < lines && l < firstLine + maxLines; ++l) {
    const int y = top + (l - firstLine) * lineH;
    for (int i = l * kExprCols; i < n && i < (l + 1) * kExprCols; ++i) {
      if (cps[i] == ' ') continue;
      gd::glyph(s, kMarginX + (i - l * kExprCols) * adv, y, calc::findGlyph(cps[i]), kExprScale, exprColor);
    }
  }
  if (!editing && lines > maxLines)
    gd::text(s, kW - kMarginX - gd::width("...", kExprScale), top + (maxLines - 1) * lineH, "...", kExprScale, p.dim);
  // The cursor: our own, blinking (the core's steady bar is dropped by the decoder).
  if (cursorIdx >= 0 && ((nowMs / kBlinkMs) % 2 == 0)) {
    const int col = cursorIdx % kExprCols, row = cursorIdx / kExprCols - firstLine;
    const int x = kMarginX + col * adv - 2, y = top + row * lineH;
    s.fillRect(x < 0 ? 0 : x, y - 1, 2, gd::kGlyphH * kExprScale + 2, p.accent);
  }
  // The result, right-aligned, as large as fits, on the bottom line of the glass (Casio).
  if (!editing) {
    const std::string r = dev.resultText();
    const int len = calc::textLength(r);
    // CMPLX: a value too long for one row shows one half; "R<=>I" (the Casio's indicator, here
    // left of the value on its bottom line, which gets narrower) says SHIFT = shows the other half.
    const char* const kReIm = "R\xE2\x87\x94I";
    const bool split = dev.complexSplit();
    const int room = kW - 2 * kMarginX - (split ? gd::width(kReIm, kTextScale) + 8 : 0);
    int scale = 4;
    if (len * gd::advance(4) > room) scale = 3;
    if (len * gd::advance(3) > room) scale = 2;
    const int y = kCalcBottom - gd::kGlyphH * scale - 2;
    gd::textRight(s, kW - kMarginX, y, r, scale, p.result);
    if (split) gd::text(s, kMarginX, kCalcBottom - gd::kGlyphH * kTextScale - 2, kReIm, kTextScale, p.accent);
  } else if (!g_grid.rowEmpty(6)) {
    // A note on the bottom row ("Scanned: check, press =", "! UNCLEAR SCAN: CHECK IT").
    const bool warn = g_grid.rowInverted(6);
    const int y = kCalcBottom - gd::kGlyphH * kTextScale - 4;
    if (warn) s.fillRect(0, y - 3, kW, gd::kGlyphH * kTextScale + 6, p.warn);
    gd::text(s, kMarginX, y, g_grid.text(6), kTextScale, warn ? p.onAccent : p.accent);
  }
}

// ---- STAT, TABLE, EQN, CALC and SOLVE: the module's rows (editor columns, the table, the menus,
// "X?" prompts, "X=" / "L−R=" lines), re-typeset like every calculator screen. A value the core
// drew in 2x digits at the bottom (a STAT result, CALC's result, a prompt's current value) is
// not on the character grid: it is drawn here from its text, large and right-aligned, as
// drawCalc() draws the COMP result.
const calc::numeric::RunnerBase* runnerOf(const Device& dev) {
  if (dev.view() == View::CalcRun) return &dev.calcRunner();
  if (dev.view() == View::SolveRun) return &dev.solveRunner();
  return nullptr;
}

std::string bigValueOf(const Device& dev) {
  if (const calc::numeric::RunnerBase* r = runnerOf(dev)) return r->screen().big;
  const calc::StatApp& st = dev.stat();
  if (dev.view() == View::Stat && st.screen() == calc::StatApp::Screen::Calc && st.showingResult())
    return st.resultText();
  // EQN's "X=" value (a+bi too): large when it fits the glass at 2x (27 characters); a longer
  // one stays on the grid, where the core put it on one or two rows.
  const calc::EqnApp& eq = dev.eqn();
  if (dev.view() == View::Eqn && eq.screen() == calc::EqnApp::Screen::Result) {
    const std::string t = eq.shownText();
    if (calc::textLength(t) * gd::advance(2) <= kW - 2 * kMarginX) return t;
  }
  return "";
}

void drawModeScreen(const Device& dev, uint32_t nowMs, int top, int pitch) {
  LGFX_Sprite& s = lcd::canvas();
  const Palette& p = pal();
  const std::string big = bigValueOf(dev);
  const int lastRow = big.empty() ? 6 : 4;  // rows 5-6 hold the 2x value's pixels
  for (int r = 1; r <= lastRow; ++r) drawGridRow(r, top + (r - 1) * pitch + 3, p.text);
  // A CALC / SOLVE prompt's insert cursor (the decoder drops the core's 1-px bar): blinking.
  if (const calc::numeric::RunnerBase* rn = runnerOf(dev)) {
    const calc::numeric::Screen sc = rn->screen();
    if (sc.cursorRow >= 1 && sc.cursorRow <= 6 && (nowMs / kBlinkMs) % 2 == 0) {
      const int adv = gd::kGlyphW * kTextScale + 1;
      const int x = kMarginX + sc.cursorCol * adv - 2, y = top + (sc.cursorRow - 1) * pitch + 3;
      s.fillRect(x < 0 ? 0 : x, y - 1, 2, gd::kGlyphH * kTextScale + 2, p.accent);
    }
  }
  if (!big.empty()) {
    const int len = calc::textLength(big);
    int scale = 4;
    if (len * gd::advance(4) > kW - 2 * kMarginX) scale = 3;
    if (len * gd::advance(3) > kW - 2 * kMarginX) scale = 2;
    gd::textRight(s, kW - kMarginX, kCalcBottom - gd::kGlyphH * scale - 2, big, scale, p.result);
  }
}

void drawOff(const Status& st) {
  LGFX_Sprite& s = lcd::canvas();
  if (g_grid.rowEmpty(3) && g_grid.rowEmpty(4)) {
    s.fillSprite(color::black);  // off: blank (the backlight is off too)
    return;
  }
  // "Charging 64%" / "ON: use it meanwhile" from the core, centred, with the battery icon,
  // in the calculator's LCD look.
  s.fillSprite(pal().bg);
  batteryIcon(s, kW / 2 - 12, 40, st);
  auto trimmed = [](std::string t) {  // the core centres with leading spaces: centre the text itself
    t.erase(0, t.find_first_not_of(' ') == std::string::npos ? t.size() : t.find_first_not_of(' '));
    return t;
  };
  gd::textCentered(s, kW / 2, 70, trimmed(g_grid.text(3)), kExprScale, pal().text);
  gd::textCentered(s, kW / 2, 110, trimmed(g_grid.text(4)), kTextScale, pal().dim);
}

bool shiftShown() { return g_grid.cell[0][0].cp == 'S'; }
bool alphaShown() { return g_grid.cell[0][1].cp == 'A'; }

// The AI Solve pages (photo, typed, or a History answer): the core's text, typeset.
void aiSoftKeys(const Device& dev) {
  if (dev.ai().screen() == calc::Screen::Ready)
    // Effort (▲▼) and Tutor (1) are on the page itself; the bar holds the three main keys.
    softKeys({{"=", "Scan"}, {"\xE2\x96\xB6", "Camera"}, {"AC", "Back"}});
  else if (dev.ai().screen() == calc::Screen::Result)
    softKeys(dev.ai().readOnly() ? Keys{{"\xE2\x96\xB2\xE2\x96\xBC", "Scroll"}, {"AC", "Back"}}
                                 : Keys{{"\xE2\x96\xB2\xE2\x96\xBC", "Scroll"}, {"=", "Again"}, {"AC", "Back"}});
  else
    softKeys({{"AC", "Cancel"}});
}

}  // namespace

void begin() { g_shownHash = 0; }

void invalidate() { g_shownHash = 0; }

void showDevice(const Device& dev, const Framebuffer& fb, const Status& st, uint32_t nowMs, bool force) {
  if (!lcd::ok()) return;
  // Which look: decided every call (also while the light is off) so the shared info pages
  // know where they were opened from.
  const Look look = dev.isOff() ? Look::Lcd : lookOf(dev.view());
  if (look != Look::Same) g_appsLook = look == Look::Colour;
  // Review F-5: nothing is drawn while the light is off. A flush would wake a sleeping panel
  // (SLPOUT + 120 ms) just for the cursor blink. The hash is kept, so the first loop with the
  // light back on draws the current picture.
  if (!force && lcd::lightOff()) return;
  if (!dev.isOff() && dev.splashShowing()) return showSplash(dev.splashPercent(), dev.splashStep());
  const View view = dev.view();
  const calc::numeric::RunnerBase* runner = dev.isOff() ? nullptr : runnerOf(dev);
  const bool editing = (view == View::Calc && !dev.showingResult() && !dev.isOff()) || view == View::Keyboard ||
                       (runner && runner->screen().cursorRow > 0);  // a CALC / SOLVE prompt's cursor blinks
  const uint32_t blink = editing ? (nowMs / kBlinkMs) % 2 : 0;
  // fb.hash(): no 7.7 KB toAscii() string every loop (review F-15). The core's mono render
  // holds every visible state (lists, the map, the table cursor, toasts), so its hash is enough.
  const uint32_t h = fnv1a(statusKey(st) + char('0' + blink) + char('A' + int(view)) + (g_appsLook ? "C" : "L") +
                               (dev.dotMatrix() ? "d" : "") + std::to_string(dev.cursor()),
                           fb.hash());
  if (!force && h == g_shownHash) return;
  g_shownHash = h;
  fbtext::decode(fb, g_grid);
  g_fb = &fb;
  usePalette(!g_appsLook);
  gd::lcdDots() = !g_appsLook && dev.dotMatrix();  // SETUP > Screen look > LCD dots
  LGFX_Sprite& s = lcd::canvas();
  if (dev.isOff()) {
    drawOff(st);
    gd::lcdDots() = false;
    lcd::flush();
    return;
  }
  s.fillSprite(pal().bg);
  const Frame f{dev, st, nowMs, shiftShown(), alphaShown()};
  bool up = false, down = false;
  bool custom = true;  // an app screen drawn from the Device's state (not the decoded text)
  switch (view) {
    case View::Apps: drawAppsMenu(f); break;
    case View::Keyboard: drawKeyboard(f); break;
    case View::History: drawHistory(f); break;
    case View::Notes: drawNotes(f); break;
    case View::NoteView: drawNoteView(f); break;
    case View::Settings: drawSettings(f); break;
    case View::Confirm: drawConfirm(f); break;
    case View::Link: drawLink(f); break;
    case View::Geo: drawGeo(f); break;
    case View::Periodic: drawPeriodic(f); break;
    default: custom = false; break;
  }
  if (custom) {
    std::string right;
    const std::string title = appTitle(dev, right);
    appStatusBar(st, title, right, f.shift, f.alpha);
  } else {
    if (g_appsLook) {
      // AI Solve and the shared info pages inside Apps: an app title, not the calculator's
      // indicators (D/R/G means nothing here).
      up = g_grid.cell[0][23].cp == 0x25B2;
      down = g_grid.cell[0][24].cp == 0x25BC;
      const char* title = "SETTINGS";
      if (view == View::Ai) title = dev.ai().readOnly() ? "HISTORY" : dev.ai().typed() ? "TYPE IT" : "AI SOLVE";
      appStatusBar(st, title, "", f.shift, f.alpha);
    } else {
      casioStatus(dev, st, up, down);  // ▲▼ are drawn in the indicator row
    }
    // Calculator screens (LCD look) are exactly the Casio's: no soft-key bar, the content
    // runs to the bottom of the glass. Error pages carry their own "[AC] :Cancel" and
    // "[◀][▶]:Goto" lines from the core render, as on the Casio.
    const bool calcLook = !g_appsLook;
    const int top = calcLook ? kCalcTop : kContentY;
    const int pitch = calcLook ? kCalcPitch : kRowPitch;
    switch (view) {
      case View::Calc:
        drawCalc(dev, nowMs);
        break;
      case View::Stat:
      case View::Table:
      case View::Eqn:
      case View::MatVec:  // MATRIX / VECTOR's Dim lists, editors, MatAns screen and menu (core rows)
      case View::CalcRun:
      case View::SolveRun:
        drawModeScreen(dev, nowMs, top, pitch);
        break;
      case View::Ai:
        drawGenericRows(pal().text, top, pitch);
        aiSoftKeys(dev);
        break;
      case View::Error:
        drawGenericRows(pal().warn, top, pitch);
        break;
      default:
        drawGenericRows(pal().text, top, pitch);
        if (!calcLook) softKeys({{"AC", "Back"}});  // an info page opened from the Settings app
        break;
    }
    if (!calcLook) scrollArrows(up, down);
  }
  if (dev.toastVisible()) toastPill(dev.toastText());
  gd::lcdDots() = false;
  lcd::flush();
}

void showLines(const std::vector<std::string>& lines, const Status& st) {
  if (!lcd::ok()) return;
  std::string key = "L" + statusKey(st);
  for (const auto& l : lines) key += l + "\n";
  const uint32_t h = fnv1a(key);
  if (h == g_shownHash) return;
  g_shownHash = h;
  usePalette(false);  // the firmware's own pages (setup by phone, updates): the brand look
  LGFX_Sprite& s = lcd::canvas();
  s.fillSprite(pal().bg);
  drawStatusBar(st, lines.empty() ? "" : lines[0].c_str());
  for (size_t i = 1; i < lines.size() && i < 7; ++i) {
    const int y = kContentY + int(i - 1) * kRowPitch + 3;
    const std::string& t = lines[i];
    const bool hint = t.rfind("AC", 0) == 0 || t.find("Next:") == 0;
    gd::textFit(s, kMarginX, y, t, kTextScale, hint ? pal().accent : pal().text, kW - 2 * kMarginX);
  }
  s.fillRect(0, kH - kSoftH, kW, kSoftH, pal().bar);
  lcd::flush();
}

void showProgress(const std::string& title, const std::string& line1, const std::string& line2, int percent,
                  const Status& st) {
  if (!lcd::ok()) return;
  const uint32_t h = fnv1a("P" + title + line1 + line2 + std::to_string(percent) + statusKey(st));
  if (h == g_shownHash) return;
  g_shownHash = h;
  usePalette(false);
  LGFX_Sprite& s = lcd::canvas();
  s.fillSprite(pal().bg);
  drawStatusBar(st, title.c_str());
  gd::textFit(s, kMarginX, kContentY + 6, line1, kTextScale, pal().text, kW - 2 * kMarginX);
  gd::textFit(s, kMarginX, kContentY + 30, line2, kTextScale, pal().dim, kW - 2 * kMarginX);
  const int bx = kMarginX, by = kContentY + 62, bw = kW - 2 * kMarginX, bh = 18;
  s.drawRoundRect(bx, by, bw, bh, 4, pal().dim);
  if (percent > 0)
    s.fillRoundRect(bx + 2, by + 2, (bw - 4) * (percent > 100 ? 100 : percent) / 100, bh - 4, 3, pal().accent);
  gd::textCentered(s, kW / 2, by + bh + 8, std::to_string(percent) + "%", kTextScale, pal().text);
  s.fillRect(0, kH - kSoftH, kW, kSoftH, pal().bar);
  gd::text(s, kMarginX, kH - kSoftH + 3, "Don't turn it off.", kTextScale, pal().warn);
  lcd::flush();
}

void showSplash(int percent, const std::string& step) {
  if (!lcd::ok()) return;
  percent = percent < 0 ? 0 : percent > 100 ? 100 : percent;
  const uint32_t h = fnv1a("S" + std::to_string(percent) + step);
  if (h == g_shownHash) return;
  g_shownHash = h;
  usePalette(false);  // the brand's navy and orange, like the other firmware screens
  gd::lcdDots() = false;
  LGFX_Sprite& s = lcd::canvas();
  s.fillSprite(pal().bg);
  // Tick mark + "Proofline" at 4x, centred as one group.
  constexpr int kNameScale = 4, kTickW = 26, kGap = 10, kNameY = 30;
  const int nameW = gd::width("Proofline", kNameScale);
  const int x0 = (kW - (kTickW + kGap + nameW)) / 2;
  const int ty = kNameY + 6;
  s.drawWideLine(x0 + 2, ty + 12, x0 + 9, ty + 19, 2.5f, pal().accent);
  s.drawWideLine(x0 + 9, ty + 19, x0 + kTickW - 2, ty + 1, 2.5f, pal().accent);
  gd::text(s, x0 + kTickW + kGap, kNameY, "Proofline", kNameScale, pal().result);
  gd::textCentered(s, kW / 2, kNameY + 40, "AI CALCULATOR", kTextScale, pal().dim);
  // The bar.
  const int bx = 40, by = 104, bw = kW - 80, bh = 12;
  s.drawRoundRect(bx, by, bw, bh, 6, pal().dim);
  if (percent > 0) s.fillRoundRect(bx + 2, by + 2, (bw - 4) * percent / 100, bh - 4, 4, pal().accent);
  if (!step.empty()) gd::textCentered(s, kW / 2, by + bh + 14, step, kTextScale, pal().dim);
  lcd::flush();
}

}  // namespace ui
