// Periodic Table (Apps 6) in colour: the 18 x 7 table plus the two f-block rows as small
// tiles coloured by category, the cursor tile enlarged, an info box in the table's gap
// (rows 1-3, groups 3-12), a footer "26 Fe Iron 55.845"; the detail page, the list, the
// search, the property graph and a colour key. State comes from Device::periodic().
//
// The ten category colours differ in lightness as well as hue (so neighbouring categories
// stay apart for most colour-vision deficiencies), every tile carries its symbol, and the
// category is named in the info box, the detail page and the key (0).
#include <cstdio>

#include "periodic.h"
#include "text.h"
#include "ui_draw.h"

using calc::Device;
using calc::PeriodicApp;

namespace ui {

namespace {

LGFX_Sprite& S() { return lcd::canvas(); }

const uint16_t kCat[] = {
    rgb565(0xF4, 0x84, 0x5F),  // alkali metals: coral
    rgb565(0xF9, 0xC7, 0x4F),  // alkaline earth metals: yellow
    rgb565(0x5F, 0xA8, 0xD3),  // transition metals: blue
    rgb565(0x90, 0xBE, 0x6D),  // post-transition metals: green
    rgb565(0x43, 0xAA, 0x8B),  // metalloids: teal
    rgb565(0xE8, 0xF0, 0xA8),  // reactive nonmetals: pale lime
    rgb565(0xB3, 0x88, 0xEB),  // noble gases: violet
    rgb565(0xF7, 0xA1, 0xC4),  // lanthanides: pink
    rgb565(0xC9, 0xA2, 0x7E),  // actinides: tan
    rgb565(0xA0, 0xA8, 0xB4),  // unknown: grey
};
const char* const kCatShort[] = {"Alkali metal", "Alkaline earth", "Transition", "Post-trans.", "Metalloid",
                                 "Nonmetal",     "Noble gas",      "Lanthanide", "Actinide",        "Unknown"};
const uint16_t kTileInk = rgb565(0x10, 0x14, 0x1c);
const uint16_t kTileDim = rgb565(0x24, 0x2c, 0x3a);
const uint16_t kTileDimInk = rgb565(0x5a, 0x66, 0x78);

uint16_t catColor(int z) { return kCat[int(calc::element(z).category)]; }

// Table geometry: 17 px columns, 11 px rows, the f-block rows 4 px lower.
constexpr int kPitchX = 17, kPitchY = 11, kTileW = 16, kTileH = 10;
constexpr int kTableX = (kW - 18 * kPitchX) / 2 + 1;
constexpr int kTableY = kContentY + 1;
constexpr int kFootY = kTableY + 9 * kPitchY + 4 + 1;  // 130
int cellX(int col) { return kTableX + col * kPitchX; }
int cellY(int row) { return kTableY + row * kPitchY + (row >= 7 ? 4 : 0); }

void footer(int z, const std::string& right, uint16_t rightColor) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const calc::Element& e = calc::element(z);
  s.fillRect(0, kFootY, kW, kContentBottom - kFootY, p.bar);
  s.fillRoundRect(6, kFootY + 4, 8, 11, 2, catColor(z));
  char left[64];
  std::snprintf(left, sizeof left, "%d %s %s", e.z, e.symbol, e.name);
  const int rw = gd::width(right, kTextScale);
  gd::textRight(s, kW - 8, kFootY + 3, right, kTextScale, rightColor);
  fitText(20, kFootY + 3, left, kTextScale, p.text, kW - 20 - rw - 16);
}

std::string massText(int z) {
  char m[24];
  calc::formatMass(z, m, sizeof m);
  return m;
}

void drawTable(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const PeriodicApp& a = f.dev.periodic();
  const bool filtering = a.filter() != calc::Filter::All;
  for (int z = 1; z <= calc::kElementCount; ++z) {
    const calc::GridPos g = calc::gridPos(z);
    const int x = cellX(g.col), y = cellY(g.row);
    const bool on = !filtering || a.highlighted(z);
    s.fillRect(x, y, kTileW, kTileH, on ? catColor(z) : kTileDim);
    const char* sym = calc::element(z).symbol;
    const int w = gd::width(sym, 1, 5) - 1;
    gd::text(s, x + (kTileW - w) / 2, y + 2, sym, 1, on ? kTileInk : kTileDimInk, 5);
  }

  // The info box in the gap.
  const int z = a.cursor();
  const calc::Element& e = calc::element(z);
  const int bx = cellX(2), by = kTableY, bw = 10 * kPitchX - 1, bh = 3 * kPitchY - 1;
  s.fillRoundRect(bx, by, bw, bh, 4, p.panel);
  s.fillRoundRect(bx + 2, by + 2, 30, bh - 4, 3, catColor(z));
  gd::text(s, bx + 4, by + 3, std::to_string(z), 1, kTileInk);
  gd::textCentered(s, bx + 17 + 1, by + 13, e.symbol, kTextScale, kTileInk, 10);
  fitText(bx + 37, by + 2, e.name, kTextScale, p.text, bw - 40);
  const std::string second = filtering ? calc::filterName(a.filter()) : calc::categoryName(e.category);
  const uint16_t secondColor = filtering ? p.accent : catColor(z);
  fitText(bx + 37, by + 20, second, 1, secondColor, bw - 40);
  // The cursor: the tile a little larger, framed white with a dark keyline (the big
  // version is in the info box).
  const calc::GridPos g = calc::gridPos(z);
  const int mw = kTileW + 6, mh = kTileH + 6;
  int mx = cellX(g.col) - 3, my = cellY(g.row) - 3;
  if (mx < 0) mx = 0;
  if (mx + mw > kW) mx = kW - mw;
  if (my < kContentY) my = kContentY;
  const bool lit = !filtering || a.highlighted(z);
  s.drawRect(mx - 1, my - 1, mw + 2, mh + 2, p.bg);
  s.fillRect(mx, my, mw, mh, color::white);
  s.fillRect(mx + 2, my + 2, mw - 4, mh - 4, lit ? catColor(z) : kTileDim);
  const int sw = gd::width(e.symbol, 1, 5) - 1;
  gd::text(s, mx + (mw - sw) / 2, my + (mh - 7) / 2, e.symbol, 1, lit ? kTileInk : color::white, 5);
  footer(z, massText(z), p.dim);
  softKeys({{"=", "Info"}, {"4", "Filter"}, {"5", "Graph"}, {"0", "Key"}});
}

void drawDetail(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const PeriodicApp& a = f.dev.periodic();
  const int z = a.cursor();
  const calc::Element& e = calc::element(z);
  // A big tile, the name and the category.
  const int tx = kMarginX, ty = kContentY + 2, ts = 34;
  s.fillRoundRect(tx, ty, ts, ts, 4, catColor(z));
  gd::text(s, tx + 3, ty + 3, std::to_string(z), 1, kTileInk);
  gd::textCentered(s, tx + ts / 2 + 1, ty + 15, e.symbol, kTextScale, kTileInk, 10);
  const int nx = tx + ts + 10, nw = kW - nx - 26;
  if (gd::width(e.name, 3) <= nw) gd::text(s, nx, ty - 1, e.name, 3, p.text);
  else fitText(nx, ty + 2, e.name, kTextScale, p.text, nw);
  fitText(nx, ty + 22, calc::categoryName(e.category), kTextScale, catColor(z), nw);
  gd::text(s, kW - 32, ty + 1, "\xE2\x97\x80\xE2\x96\xB6", 1, p.dim);
  // The property lines, the core's wrapping (25 columns), five at a time.
  const int lines = calc::detailLine(z, -1, nullptr, 0);
  const int top = a.detailScroll(), visible = 5, y0 = ty + ts + 4, pitch = 17;
  char line[96];
  for (int r = 0; r < visible && top + r < lines; ++r) {
    calc::detailLine(z, top + r, line, sizeof line);
    const bool cont = line[0] == ' ';
    gd::text(s, kMarginX, y0 + r * pitch, line, kTextScale, cont ? p.dim : p.text);
  }
  scrollBar(y0, visible * pitch - 3, top, visible, lines);
  softKeys({{"\xE2\x97\x80\xE2\x96\xB6", "Element"}, {"\xE2\x96\xB2\xE2\x96\xBC", "Scroll"}, {"AC", "Back"}});
}

void drawElementList(const Frame& f) {
  const PeriodicApp& a = f.dev.periodic();
  std::vector<ListRow> rows(calc::kElementCount);
  const int top = a.listTop();
  for (int i = top; i < top + 5 && i < calc::kElementCount; ++i) {
    const int z = a.listAt(i);
    const calc::Element& e = calc::element(z);
    char left[48];
    std::snprintf(left, sizeof left, "%3d %-2s %s", e.z, e.symbol, e.name);
    rows[size_t(i)].left = left;
    rows[size_t(i)].right = massText(z);
    rows[size_t(i)].swatch = catColor(z);
  }
  drawList(rows, a.listPos(), top);
  softKeys({{"\xE2\x96\xB2\xE2\x96\xBC", "Move"}, {"2", "Sort"}, {"=", "Info"}, {"AC", "Table"}});
}

void drawSearch(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const PeriodicApp& a = f.dev.periodic();
  s.fillRoundRect(6, kContentY + 2, kW - 12, 22, 5, p.panel);
  const bool number = a.searchNumber() > 0;
  int x = gd::text(s, 14, kContentY + 6, number ? "Z:" : "Symbol:", kTextScale, p.dim);
  x = gd::text(s, x + 8, kContentY + 6, number ? std::to_string(a.searchNumber()) : std::string(a.searchLetters()),
               kTextScale, p.text);
  s.fillRect(x + 1, kContentY + 5, 2, 15, p.accent);
  const int z = a.searchMatch();
  if (z) {
    const calc::Element& e = calc::element(z);
    const std::string t = std::string(e.symbol) + " " + e.name;
    const int w = gd::width(t, kTextScale, gd::tight(kTextScale));
    s.fillRoundRect(kW - 14 - w - 16, kContentY + 6, 10, 14, 2, catColor(z));
    gd::text(s, kW - 14 - w, kContentY + 6, t, kTextScale, p.accent, gd::tight(kTextScale));
  } else if (a.searchLetters()[0]) {
    gd::textRight(s, kW - 14, kContentY + 6, "no match", kTextScale, p.dim);
  }
  // A-Z + OK, 9 to a row.
  const int pitchX = 34, pitchY = 23, w = 32, h = 21;
  const int x0 = (kW - PeriodicApp::kSearchCols * pitchX + 2) / 2, y0 = kContentY + 29;
  for (int i = 0; i < PeriodicApp::kSearchCells; ++i) {
    const int cx = x0 + (i % PeriodicApp::kSearchCols) * pitchX, cy = y0 + (i / PeriodicApp::kSearchCols) * pitchY;
    const bool sel = i == a.searchCell(), ok = i == 26;
    s.fillRoundRect(cx, cy, w, h, 4, sel ? p.accent : ok ? rgb565(0x4d, 0x2c, 0x1f) : p.panel2);
    if (ok && !sel) s.drawRoundRect(cx, cy, w, h, 4, p.accent);
    gd::textCentered(s, cx + w / 2 + 1, cy + 4, ok ? "OK" : std::string(1, char('A' + i)), kTextScale,
                     sel ? p.onAccent : ok ? p.accent : p.text);
  }
  gd::text(s, 14, kContentY + 101, "Digits type a number (1-118).", kTextScale, p.dim, gd::tight(kTextScale));
  softKeys({{"=", "Pick"}, {"DEL", "Undo"}, {"AC", "Back"}});
}

std::string axisText(float v) {
  char b[16];
  if (v >= 100.0f) std::snprintf(b, sizeof b, "%.0f", v);
  else if (v >= 0.01f || v == 0.0f) std::snprintf(b, sizeof b, "%.3g", v);
  else std::snprintf(b, sizeof b, "%.0e", v);
  return b;
}

void drawGraph(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const PeriodicApp& a = f.dev.periodic();
  const calc::GraphProp prop = a.graphProp();
  float lo, hi;
  calc::graphRange(prop, lo, hi);
  const std::string loT = axisText(lo), hiT = axisText(hi);
  const int labelW = std::max(gd::width(loT, 1, 6), gd::width(hiT, 1, 6));
  const int px0 = 8 + labelW + 4, px1 = kW - 8, py0 = kContentY + 6, py1 = kFootY - 12;
  auto X = [&](int z) { return px0 + (z - 1) * (px1 - px0) / (calc::kElementCount - 1); };
  auto Y = [&](float v) { return py1 - int((v - lo) / (hi - lo) * (py1 - py0) + 0.5f); };
  // Axes, ticks and labels.
  s.drawFastVLine(px0 - 2, py0, py1 - py0 + 3, p.dim);
  s.drawFastHLine(px0 - 2, py1 + 2, px1 - px0 + 3, p.dim);
  for (int k = 0; k <= 4; ++k) s.drawFastHLine(px0, py0 + k * (py1 - py0) / 4, px1 - px0, p.rule);
  gd::textRight(s, px0 - 4, py0 - 3, hiT, 1, p.dim, 6);
  gd::textRight(s, px0 - 4, py1 - 3, loT, 1, p.dim, 6);
  for (int z : {1, 20, 40, 60, 80, 100, 118}) {
    s.drawFastVLine(X(z), py1 + 2, 3, p.dim);
    gd::textCentered(s, X(z) + 1, py1 + 6, std::to_string(z), 1, p.dim, 6);
  }
  // The line through the values, then the points in their category colours.
  int lx = -1, ly = 0;
  for (int z = 1; z <= calc::kElementCount; ++z) {
    float v;
    if (!calc::propertyValue(z, prop, v)) {
      lx = -1;
      continue;
    }
    const int x = X(z), y = Y(v);
    if (lx >= 0) s.drawLine(lx, ly, x, y, p.panel2);
    lx = x, ly = y;
  }
  for (int z = 1; z <= calc::kElementCount; ++z) {
    float v;
    if (calc::propertyValue(z, prop, v)) s.fillRect(X(z) - 1, Y(v) - 1, 2, 2, catColor(z));
  }
  // The cursor: a dashed line and a ring on its point.
  const int z = a.cursor();
  const int cx = X(z);
  for (int y = py0; y <= py1; y += 4) s.drawFastVLine(cx, y, 2, p.accent);
  float v;
  const bool has = calc::propertyValue(z, prop, v);
  if (has) {
    s.drawCircle(cx, Y(v), 4, color::white);
    s.fillCircle(cx, Y(v), 2, p.accent);
  }
  char val[32];
  if (has) std::snprintf(val, sizeof val, "%.6g", v);
  else std::snprintf(val, sizeof val, "no value");
  footer(z, val, has ? p.accent : p.dim);
  softKeys({{"\xE2\x97\x80\xE2\x96\xB6", "Element"}, {"\xE2\x96\xB2\xE2\x96\xBC", "Property"}, {"AC", "Table"}});
}

void drawKey(const Frame& f) {
  (void)f;
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  for (int i = 0; i < 10; ++i) {
    const int col = i / 5, row = i % 5;
    const int x = 8 + col * 168, y = kContentY + 4 + row * 20;
    s.fillRoundRect(x, y + 1, 14, 13, 3, kCat[i]);
    gd::text(s, x + 20, y, kCatShort[i], kTextScale, p.text, gd::tight(kTextScale));
  }
  gd::text(s, 10, kContentY + 106, "1 List  3 Find  4 Filter  5 Graph", kTextScale, p.dim, gd::tight(kTextScale) - 1);
  softKeys({{"any key", "Back"}});
}

}  // namespace

std::string periodicTitle(const Device& dev, std::string& right) {
  const PeriodicApp& a = dev.periodic();
  switch (a.screen()) {
    case PeriodicApp::Screen::Detail: right = std::to_string(a.cursor()) + "/118"; return "ELEMENT";
    case PeriodicApp::Screen::List: {
      static const char* const kSort[] = {"LIST BY NUMBER", "LIST BY NAME", "LIST BY SYMBOL"};
      return kSort[int(a.sort())];
    }
    case PeriodicApp::Screen::Search: return "FIND ELEMENT";
    case PeriodicApp::Screen::Graph: right = "\xE2\x96\xB2\xE2\x96\xBC"; return calc::graphPropName(a.graphProp());
    case PeriodicApp::Screen::Help: return "COLOUR KEY";
    default: return "PERIODIC TABLE";
  }
}

void drawPeriodic(const Frame& f) {
  switch (f.dev.periodic().screen()) {
    case PeriodicApp::Screen::Table: return drawTable(f);
    case PeriodicApp::Screen::Detail: return drawDetail(f);
    case PeriodicApp::Screen::List: return drawElementList(f);
    case PeriodicApp::Screen::Search: return drawSearch(f);
    case PeriodicApp::Screen::Graph: return drawGraph(f);
    case PeriodicApp::Screen::Help: return drawKey(f);
  }
}

}  // namespace ui
