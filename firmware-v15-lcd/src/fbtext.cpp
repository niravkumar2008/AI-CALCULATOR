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

// The code point whose glyph is exactly `rows`, or 0.
uint32_t match(const uint8_t* rows) {
  bool any = false;
  for (int i = 0; i < 7; ++i) any |= rows[i] != 0;
  if (!any) return ' ';
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

}  // namespace fbtext
