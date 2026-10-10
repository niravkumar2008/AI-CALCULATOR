// Reads the calculator's 1-bit 125x61 Framebuffer (what core/ renders for the e-paper)
// back into text: 7 rows x 25 character cells, each matched against the core's own 5x7
// font, with its attributes (inverted = a menu mark / the exam bar, underline = a
// section header). Cells that are not a glyph (the battery outline, a cursor bar drawn
// into a glyph's gap column) keep their raw pixels.
//
// Why: core/ draws every screen (menus, SETUP, AI answers, notices) into that small
// buffer and stays display-agnostic. The LCD UI (ui.cpp) wants to typeset those same
// screens larger and in colour, so it decodes the text instead of blowing the 1-bit
// picture up. Nothing in core/ changes for the v15 board.
#pragma once
#include <cstdint>
#include <string>

#include "framebuffer.h"

namespace fbtext {

constexpr int kRows = calc::Framebuffer::kRows;  // 7 (row 0 = status bar)
constexpr int kCols = calc::Framebuffer::kCols;  // 25

struct Cell {
  uint32_t cp = ' ';      // the character (space for an empty cell, 0 when raw)
  uint8_t rows[7] = {};   // the cell's pixels (low 5 bits, bit 4 = leftmost)
  bool raw = false;       // no glyph matched: draw rows[] as pixels
  bool inverted = false;  // drawn light-on-dark
  bool underline = false; // a rule in the gap row under the cell
  bool empty() const { return cp == ' ' && !raw && !inverted && !underline; }
};

struct Grid {
  Cell cell[kRows][kCols];
  // The text of one row, UTF-8, trailing spaces removed (raw cells become '?').
  std::string text(int row) const;
  bool rowEmpty(int row) const;
  // True if any cell of the row is inverted (the exam bar, a selected menu item).
  bool rowInverted(int row) const;
};

void decode(const calc::Framebuffer& fb, Grid& out);

}  // namespace fbtext
