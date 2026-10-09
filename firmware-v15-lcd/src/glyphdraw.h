// Draws text with the calculator's own 5x7 font (core/font.h: every maths symbol the
// calculator uses: ÷ × √ π ² ⁻¹ ∫ Σ ▲ ▼ ...) scaled up on an LGFX sprite. One font for the
// whole UI, so an expression looks the same on the LCD, the e-paper and the simulators.
#pragma once
#include <cstdint>
#include <string>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "font.h"

namespace glyphdraw {

constexpr int kGlyphW = 5;  // the font cell (4 px glyph + 1 px gap)
constexpr int kGlyphH = 7;

// One glyph at (x, y), each font pixel drawn `scale` x `scale` LCD pixels.
inline void glyph(LGFX_Sprite& s, int x, int y, const calc::Glyph* g, int scale, uint16_t color) {
  for (int r = 0; r < kGlyphH; ++r) {
    const uint8_t bits = g->rows[r];
    if (!bits) continue;
    for (int c = 0; c < kGlyphW; ++c)
      if (bits & (0x10 >> c)) s.fillRect(x + c * scale, y + r * scale, scale, scale, color);
  }
}

// Raw cell pixels (a battery icon the core drew, or anything the decoder didn't recognise).
inline void bits(LGFX_Sprite& s, int x, int y, const uint8_t rows[7], int scale, uint16_t color) {
  for (int r = 0; r < kGlyphH; ++r)
    for (int c = 0; c < kGlyphW; ++c)
      if (rows[r] & (0x10 >> c)) s.fillRect(x + c * scale, y + r * scale, scale, scale, color);
}

// The advance per character at a scale: the 5-px cell plus a little extra air, which the
// 1-bit font lacks (its glyphs are 4 px with a 1 px gap).
inline int advance(int scale) { return kGlyphW * scale + (scale >= 3 ? scale / 2 : scale / 2); }

inline int width(const std::string& utf8, int scale) {
  return int(calc::decodeUtf8(utf8).size()) * advance(scale);
}

// UTF-8 text from (x, y). Returns the x after the last character.
inline int text(LGFX_Sprite& s, int x, int y, const std::string& utf8, int scale, uint16_t color) {
  for (uint32_t cp : calc::decodeUtf8(utf8)) {
    glyph(s, x, y, calc::findGlyph(cp), scale, color);
    x += advance(scale);
  }
  return x;
}

inline void textCentered(LGFX_Sprite& s, int cx, int y, const std::string& utf8, int scale, uint16_t color) {
  text(s, cx - width(utf8, scale) / 2, y, utf8, scale, color);
}

inline void textRight(LGFX_Sprite& s, int xRight, int y, const std::string& utf8, int scale, uint16_t color) {
  text(s, xRight - width(utf8, scale), y, utf8, scale, color);
}

}  // namespace glyphdraw
