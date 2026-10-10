#include "fbtext.h"

#include "font.h"

namespace fbtext {
namespace {

using calc::Framebuffer;

bool sameRows(const uint8_t* a, const uint8_t* b) {
  for (int i = 0; i < 7; ++i)
    if (a[i] != b[i]) return false;
  return true;
}

// The glyphs core actually draws but kGlyphs[] doesn't hold as such: core/font.cpp swaps
// some kGlyphs entries for hand-drawn replacements (its private kExtra table: √ ≈ ⌟ ▲ ▼
// ◀ ▶ ✓ ⚠) and adds private-use icons (U+E000 padlock, U+E001 Wi-Fi). That table isn't
// exported, so it is read back through core's public findGlyph()/hasGlyph(): whatever
// findGlyph() returns is exactly what Framebuffer::drawText() put on the screen.
struct Drawn {
  uint32_t cp;
  const uint8_t* rows;
};

const std::vector<Drawn>& drawnGlyphs() {
  static std::vector<Drawn> t;
  static bool built = false;
  if (built) return t;
  built = true;
  auto add = [](uint32_t cp) {
    if (!calc::hasGlyph(cp)) return;
    const calc::Glyph* g = calc::findGlyph(cp);
    if (g->cp != cp) return;  // normalised onto another character: that one is matched itself
    for (const Drawn& d : t)
      if (d.cp == cp) return;
    t.push_back({cp, g->rows});
  };
  // kGlyphs entries that findGlyph() replaces (the pointer differs from the table's own).
  for (int i = 0; i < calc::kGlyphCount; ++i)
    if (calc::findGlyph(calc::kGlyphs[i].cp) != &calc::kGlyphs[i]) add(calc::kGlyphs[i].cp);
  // The replacements and icons by code point (some have no kGlyphs entry at all).
  for (uint32_t cp : {0x2248u, 0x221Au, 0x231Fu, 0x25B2u, 0x25BCu, 0x25C0u, 0x25B6u, 0x2713u, 0x26A0u}) add(cp);
  for (uint32_t cp = 0xE000; cp < 0xE020; ++cp) add(cp);
  return t;
}

// The code point whose glyph is exactly `rows`, or 0.
uint32_t match(const uint8_t* rows) {
  bool any = false;
  for (int i = 0; i < 7; ++i) any |= rows[i] != 0;
  if (!any) return ' ';
  for (const Drawn& d : drawnGlyphs())  // first: what core really draws
    if (sameRows(rows, d.rows)) return d.cp;
  for (int i = 0; i < calc::kGlyphCount; ++i)
    if (sameRows(rows, calc::kGlyphs[i].rows)) return calc::kGlyphs[i].cp;
  return 0;
}

// A glyph with a 1-pixel vertical bar added (the insert cursor drawn into a cell's gap
// column, or into column 0): drop the bar and try again.
uint32_t matchWithoutBar(uint8_t* rows) {
  for (uint8_t bar : {uint8_t(0x01), uint8_t(0x10)}) {
    bool full = true;
    for (int i = 0; i < 7; ++i) full &= (rows[i] & bar) != 0;
    if (!full) continue;
    uint8_t t[7];
    for (int i = 0; i < 7; ++i) t[i] = uint8_t(rows[i] & ~bar);
    const uint32_t cp = match(t);
    if (cp) {
      for (int i = 0; i < 7; ++i) rows[i] = t[i];
      return cp;
    }
  }
  return 0;
}

}  // namespace

std::string Grid::text(int row) const {
  std::vector<uint32_t> cps;
  for (int c = 0; c < kCols; ++c) cps.push_back(cell[row][c].raw ? '?' : cell[row][c].cp);
  while (!cps.empty() && cps.back() == ' ') cps.pop_back();
  return calc::encodeUtf8(cps);
}

bool Grid::rowEmpty(int row) const {
  for (int c = 0; c < kCols; ++c)
    if (!cell[row][c].empty()) return false;
  return true;
}

bool Grid::rowInverted(int row) const {
  for (int c = 0; c < kCols; ++c)
    if (cell[row][c].inverted) return true;
  return false;
}

void decode(const Framebuffer& fb, Grid& out) {
  for (int r = 0; r < kRows; ++r) {
    const int y0 = r * 8;
    for (int c = 0; c < kCols; ++c) {
      Cell& cell = out.cell[r][c];
      cell = Cell();
      const int x0 = c * 5;
      for (int j = 0; j < 7; ++j) {
        uint8_t bits = 0;
        for (int i = 0; i < 5; ++i)
          if (fb.get(x0 + i, y0 + j)) bits |= uint8_t(0x10 >> i);
        cell.rows[j] = bits;
      }
      bool gap = false;  // the 1-px row under the cell: underline rule (or the cursor's "_")
      for (int i = 0; i < 5; ++i) gap |= fb.get(x0 + i, y0 + 7);

      uint32_t cp = match(cell.rows);
      if (!cp) cp = matchWithoutBar(cell.rows);
      if (!cp) {
        // Try the inverse: a selected menu item or the exam bar.
        uint8_t inv[7];
        for (int j = 0; j < 7; ++j) inv[j] = uint8_t(~cell.rows[j] & 0x1F);
        cp = match(inv);
        if (!cp) cp = matchWithoutBar(inv);
        if (cp) {
          cell.inverted = true;
          for (int j = 0; j < 7; ++j) cell.rows[j] = inv[j];
        }
      }
      if (!cp) {
        cell.raw = true;
        cell.cp = 0;
      } else {
        cell.cp = cp;
      }
      cell.underline = gap != cell.inverted;
    }
  }
}

std::string decodeScaled(const Framebuffer& fb, int x, int y, int scale) {
  if (scale < 2) return "";
  std::vector<uint32_t> cps;
  const int cw = calc::kCharW * scale;
  for (int cx = x; cx + (calc::kCharW - 1) * scale <= Framebuffer::kWidth; cx += cw) {
    uint8_t rows[7];
    for (int j = 0; j < 7; ++j) {
      uint8_t bits = 0;
      for (int i = 0; i < 5; ++i) {
        // Every font pixel must be a solid scale x scale block, or this isn't scaled text.
        const bool v = fb.get(cx + i * scale, y + j * scale);
        for (int dy = 0; dy < scale; ++dy)
          for (int dx = 0; dx < scale; ++dx)
            if (fb.get(cx + i * scale + dx, y + j * scale + dy) != v) return "";
        if (v) bits |= uint8_t(0x10 >> i);
      }
      rows[j] = bits;
    }
    const uint32_t cp = match(rows);
    if (!cp) return "";
    cps.push_back(cp);
  }
  while (!cps.empty() && cps.back() == ' ') cps.pop_back();
  return calc::encodeUtf8(cps);
}

}  // namespace fbtext
