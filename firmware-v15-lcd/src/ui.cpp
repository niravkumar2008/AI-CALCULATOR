#include "ui.h"

#include <Arduino.h>

#include "calc_engine.h"
#include "fbtext.h"
#include "font.h"
#include "glyphdraw.h"
#include "lcd.h"
#include "text.h"

using calc::Device;
using calc::Framebuffer;
using calc::View;
namespace gd = glyphdraw;

namespace ui {
namespace {

constexpr int kW = lcd::kW, kH = lcd::kH;
constexpr int kMarginX = 10;
constexpr int kRowPitch = kContentH / 6;  // 6 content rows (core rows 1-6), 20 px each
constexpr int kTextScale = 2;             // 10 x 14 px glyphs for menus and answers
constexpr int kExprScale = 3;             // 15 x 21 px for the expression
constexpr int kExprCols = (kW - 2 * kMarginX) / (gd::kGlyphW * kExprScale + 1);  // 18 chars per line at 3x
constexpr int kExprLines = 4;
constexpr uint32_t kBlinkMs = 500;

uint32_t g_shownHash = 0;
fbtext::Grid g_grid;

uint32_t fnv1a(const std::string& s, uint32_t h = 2166136261u) {
  for (unsigned char c : s) h = (h ^ c) * 16777619u;
  return h ? h : 1;
}

std::string statusKey(const Status& st) {
  return std::to_string(st.battery) + (st.charging ? "c" : "") + (st.wifiOn ? "w" : "") +
         (st.wifiConnected ? "W" : "") + (st.lowBattery ? "L" : "");
}

// ---- status bar
void batteryIcon(LGFX_Sprite& s, int x, int y, const Status& st) {
  // 24 x 12 outline with a tip; the fill shows what's left; red under 20 %, accent on a cable.
  const uint16_t frame = color::dim;
  s.drawRect(x, y, 22, 12, frame);
  s.fillRect(x + 22, y + 3, 2, 6, frame);
  if (st.battery < 0) return;
  const int pct = st.battery > 100 ? 100 : st.battery;
  const uint16_t fill = st.charging ? color::accent : pct <= 20 ? color::error : color::good;
  const int w = (18 * pct + 50) / 100;
  if (w > 0) s.fillRect(x + 2, y + 2, w, 8, fill);
  if (st.charging) {  // a small bolt
    s.fillRect(x + 9, y + 2, 2, 4, color::white);
    s.fillRect(x + 11, y + 5, 2, 2, color::white);
    s.fillRect(x + 9, y + 7, 2, 3, color::white);
  }
}

void wifiIcon(LGFX_Sprite& s, int x, int y, const Status& st) {
  if (!st.wifiOn && !st.wifiConnected) return;
  const uint16_t c = st.wifiConnected ? color::accent : color::dim;
  // three bars of growing height
  s.fillRect(x, y + 8, 3, 4, c);
  s.fillRect(x + 4, y + 5, 3, 7, st.wifiConnected ? c : color::bar);
  s.fillRect(x + 8, y + 2, 3, 10, st.wifiConnected ? c : color::bar);
  if (!st.wifiConnected) {
    s.drawRect(x + 4, y + 5, 3, 7, c);
    s.drawRect(x + 8, y + 2, 3, 10, c);
  }
}

}  // namespace

void drawStatusBar(const Status& st, const char* leftText) {
  LGFX_Sprite& s = lcd::canvas();
  s.fillRect(0, 0, kW, kStatusH, color::bar);
  if (leftText) gd::text(s, kMarginX, 4, leftText, kTextScale, color::text);
  wifiIcon(s, 250, 5, st);
  batteryIcon(s, 292, 5, st);
  if (st.battery >= 0) gd::textRight(s, 288, 4, std::to_string(st.battery > 100 ? 100 : st.battery) + "%",
                                     kTextScale, st.battery <= 20 && !st.charging ? color::error : color::dim);
}

namespace {

// The core's status row (SHIFT, ALPHA, M, STO/RCL, D/R/G, EXAM clock, AI) typeset into the
// bar; its own battery/Wi-Fi glyphs and scroll arrows are replaced by ours.
void statusFromGrid(const Status& st, bool& up, bool& down) {
  LGFX_Sprite& s = lcd::canvas();
  const bool exam = g_grid.rowInverted(0) && g_grid.text(0).find("EXAM") != std::string::npos;
  s.fillRect(0, 0, kW, kStatusH, exam ? color::error : color::bar);
  int x = kMarginX;
  up = g_grid.cell[0][23].cp == 0x25B2;
  down = g_grid.cell[0][24].cp == 0x25BC;
  for (int c = 0; c < 21; ++c) {  // cells 21-22 hold the core's battery outline, 23-24 the arrows
    const fbtext::Cell& cell = g_grid.cell[0][c];
    const int cx = kMarginX + c * (gd::kGlyphW * kTextScale + 1);
    if (cell.raw) {
      gd::bits(s, cx, 4, cell.rows, kTextScale, color::text);
      continue;
    }
    if (cell.cp == ' ') continue;
    if (cell.cp == 0xE001) continue;  // the Wi-Fi glyph: ours is on the right
    uint16_t col = color::text;
    if (c == 0 && cell.cp == 'S') col = color::warn;    // SHIFT
    if (c == 1 && cell.cp == 'A') col = color::error;   // ALPHA
    if (c == 9) col = color::accent;                    // D / R / G
    if (c >= 19 && cell.cp != ' ') col = color::accent; // AI
    gd::glyph(s, cx, 4, calc::findGlyph(cell.cp), kTextScale, exam ? color::white : col);
    x = cx;
  }
  (void)x;
  wifiIcon(s, 250, 5, st);
  batteryIcon(s, 292, 5, st);
  if (st.battery >= 0) gd::textRight(s, 288, 4, std::to_string(st.battery > 100 ? 100 : st.battery) + "%",
                                     kTextScale, st.battery <= 20 && !st.charging ? color::error : color::dim);
}

void scrollArrows(bool up, bool down) {
  LGFX_Sprite& s = lcd::canvas();
  if (up) gd::glyph(s, kW - 14, kContentY + 2, calc::findGlyph(0x25B2), kTextScale, color::accent);
  if (down) gd::glyph(s, kW - 14, kContentY + kContentH - 16, calc::findGlyph(0x25BC), kTextScale, color::accent);
}

void softKeys(const std::vector<std::pair<std::string, std::string>>& keys) {
  LGFX_Sprite& s = lcd::canvas();
  const int y = kH - kSoftH;
  s.fillRect(0, y, kW, kSoftH, color::bar);
  int x = kMarginX;
  for (const auto& k : keys) {
    const int kw = gd::width(k.first, kTextScale) + 6;
    s.fillRoundRect(x, y + 3, kw, 14, 3, color::dim);
    gd::text(s, x + 3, y + 3, k.first, kTextScale, color::black);
    x += kw + 4;
    x = gd::text(s, x, y + 3, k.second, kTextScale, color::dim) + 12;
  }
}

// One decoded core row typeset at 2x: inverted cells on an accent block, underlined rows
// as headers.
void drawGridRow(int row, int y, uint16_t textColor) {
  LGFX_Sprite& s = lcd::canvas();
  const int adv = gd::kGlyphW * kTextScale + 1;
  bool underline = false;
  for (int c = 0; c < fbtext::kCols; ++c) {
    const fbtext::Cell& cell = g_grid.cell[row][c];
    const int x = kMarginX + c * adv;
    underline |= cell.underline && cell.cp != ' ';
    if (cell.inverted) s.fillRect(x - 1, y - 2, adv + 1, gd::kGlyphH * kTextScale + 4, color::accent);
    const uint16_t col = cell.inverted ? color::black : textColor;
    if (cell.raw) gd::bits(s, x, y, cell.rows, kTextScale, col);
    else if (cell.cp != ' ') gd::glyph(s, x, y, calc::findGlyph(cell.cp), kTextScale, col);
  }
  if (underline) {
    const int w = gd::width(g_grid.text(row), kTextScale);
    s.fillRect(kMarginX, y + gd::kGlyphH * kTextScale + 1, w, 1, color::accent);
  }
}

void drawGenericRows(uint16_t textColor = color::text) {
  LGFX_Sprite& s = lcd::canvas();
  for (int r = 1; r < fbtext::kRows; ++r) {
    const int y = kContentY + (r - 1) * kRowPitch + 3;
    const bool header = r == 1 && [&] {
      for (int c = 0; c < fbtext::kCols; ++c)
        if (g_grid.cell[1][c].underline && g_grid.cell[1][c].cp != ' ') return true;
      return false;
    }();
    drawGridRow(r, y, header ? color::accent : textColor);
  }
  (void)s;
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
  const int top = kContentY + 2;
  const uint16_t exprColor = editing ? color::text : color::dim;
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
    gd::text(s, kW - kMarginX - gd::width("...", kExprScale), top + (maxLines - 1) * lineH, "...", kExprScale, color::dim);
  // The cursor: our own, blinking (the core's steady bar is dropped by the decoder).
  if (cursorIdx >= 0 && ((nowMs / kBlinkMs) % 2 == 0)) {
    const int col = cursorIdx % kExprCols, row = cursorIdx / kExprCols - firstLine;
    const int x = kMarginX + col * adv - 2, y = top + row * lineH;
    s.fillRect(x < 0 ? 0 : x, y - 1, 2, gd::kGlyphH * kExprScale + 2, color::accent);
  }
  // The result, right-aligned, as large as fits.
  if (!editing) {
    const std::string r = dev.resultText();
    const int len = calc::textLength(r);
    int scale = 4;
    if (len * gd::advance(4) > kW - 2 * kMarginX) scale = 3;
    if (len * gd::advance(3) > kW - 2 * kMarginX) scale = 2;
    const int y = kContentY + kContentH - gd::kGlyphH * scale - 4;
    gd::textRight(s, kW - kMarginX, y, r, scale, color::result);
  } else if (!g_grid.rowEmpty(6)) {
    // A note on the bottom row ("Scanned: check, press =", "! UNCLEAR SCAN: CHECK IT").
    const bool warn = g_grid.rowInverted(6);
    const int y = kContentY + kContentH - gd::kGlyphH * kTextScale - 6;
    if (warn) s.fillRect(0, y - 3, kW, gd::kGlyphH * kTextScale + 6, color::warn);
    gd::text(s, kMarginX, y, g_grid.text(6), kTextScale, warn ? color::black : color::accent);
  }
}

void drawOff(const Status& st) {
  LGFX_Sprite& s = lcd::canvas();
  s.fillSprite(color::black);
  if (g_grid.rowEmpty(3) && g_grid.rowEmpty(4)) return;
  // "Charging 64%" / "ON: use it meanwhile" from the core, centred, with the battery icon.
  batteryIcon(s, kW / 2 - 12, 40, st);
  gd::textCentered(s, kW / 2, 70, g_grid.text(3), kExprScale, color::text);
  gd::textCentered(s, kW / 2, 110, g_grid.text(4), kTextScale, color::dim);
}

bool shiftShown() { return g_grid.cell[0][0].cp == 'S'; }

}  // namespace

void begin() { g_shownHash = 0; }

void invalidate() { g_shownHash = 0; }

void showDevice(const Device& dev, const Framebuffer& fb, const Status& st, uint32_t nowMs, bool force) {
  if (!lcd::ok()) return;
  const bool editing = dev.view() == View::Calc && !dev.showingResult() && !dev.isOff();
  const uint32_t blink = editing ? (nowMs / kBlinkMs) % 2 : 0;
  const uint32_t h = fnv1a(fb.toAscii() + statusKey(st) + char('0' + blink) + char('A' + int(dev.view())) +
                           std::to_string(dev.cursor()));
  if (!force && h == g_shownHash) return;
  g_shownHash = h;
  fbtext::decode(fb, g_grid);
  LGFX_Sprite& s = lcd::canvas();
  if (dev.isOff()) {
    drawOff(st);
    lcd::flush();
    return;
  }
  s.fillSprite(color::bg);
  bool up = false, down = false;
  statusFromGrid(st, up, down);
  switch (dev.view()) {
    case View::Calc:
      drawCalc(dev, nowMs);
      if (shiftShown()) softKeys({{"MODE", "Setup"}, {"\xE2\x96\xB2\xE2\x96\xBC", "Light"}, {"AC", "Off"}});
      else softKeys({{"MODE", "Menu"}, {"MODE 4", "AI solve"}, {"SHIFT", "2nd"}});
      break;
    case View::Ai:
      drawGenericRows();
      if (dev.ai().screen() == calc::Screen::Ready)
        softKeys({{"=", "Scan"}, {"\xE2\x96\xB6", "Camera"}, {"\xE2\x96\xB2\xE2\x96\xBC", "Effort"}, {"1", "Tutor"}, {"AC", "Back"}});
      else if (dev.ai().screen() == calc::Screen::Result)
        softKeys({{"\xE2\x96\xB2\xE2\x96\xBC", "Scroll"}, {"=", "Again"}, {"AC", "Back"}});
      else
        softKeys({{"AC", "Cancel"}});
      break;
    case View::Error:
      drawGenericRows(color::warn);
      softKeys({{"AC", "Clear"}, {"\xE2\x97\x80\xE2\x96\xB6", "Go to"}});
      break;
    default:
      drawGenericRows();
      softKeys({{"AC", "Back"}});
      break;
  }
  scrollArrows(up, down);
  lcd::flush();
}

void showLines(const std::vector<std::string>& lines, const Status& st) {
  if (!lcd::ok()) return;
  std::string key = "L" + statusKey(st);
  for (const auto& l : lines) key += l + "\n";
  const uint32_t h = fnv1a(key);
  if (h == g_shownHash) return;
  g_shownHash = h;
  LGFX_Sprite& s = lcd::canvas();
  s.fillSprite(color::bg);
  drawStatusBar(st, lines.empty() ? "" : lines[0].c_str());
  for (size_t i = 1; i < lines.size() && i < 7; ++i) {
    const int y = kContentY + int(i - 1) * kRowPitch + 3;
    const std::string& t = lines[i];
    const bool hint = t.rfind("AC", 0) == 0 || t.find("Next:") == 0;
    gd::text(s, kMarginX, y, t, kTextScale, hint ? color::accent : color::text);
  }
  s.fillRect(0, kH - kSoftH, kW, kSoftH, color::bar);
  lcd::flush();
}

void showProgress(const std::string& title, const std::string& line1, const std::string& line2, int percent,
                  const Status& st) {
  if (!lcd::ok()) return;
  const uint32_t h = fnv1a("P" + title + line1 + line2 + std::to_string(percent) + statusKey(st));
  if (h == g_shownHash) return;
  g_shownHash = h;
  LGFX_Sprite& s = lcd::canvas();
  s.fillSprite(color::bg);
  drawStatusBar(st, title.c_str());
  gd::text(s, kMarginX, kContentY + 6, line1, kTextScale, color::text);
  gd::text(s, kMarginX, kContentY + 30, line2, kTextScale, color::dim);
  const int bx = kMarginX, by = kContentY + 62, bw = kW - 2 * kMarginX, bh = 18;
  s.drawRoundRect(bx, by, bw, bh, 4, color::dim);
  if (percent > 0) s.fillRoundRect(bx + 2, by + 2, (bw - 4) * (percent > 100 ? 100 : percent) / 100, bh - 4, 3, color::accent);
  gd::textCentered(s, kW / 2, by + bh + 8, std::to_string(percent) + "%", kTextScale, color::text);
  s.fillRect(0, kH - kSoftH, kW, kSoftH, color::bar);
  gd::text(s, kMarginX, kH - kSoftH + 3, "Don't turn it off.", kTextScale, color::warn);
  lcd::flush();
}

}  // namespace ui
