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

// A real radical sign for √ at 2x and up (the 5x7 one is a blocky hook when blown up): a
// short tick, the down-stroke, the long up-stroke and the bar, which runs on into the gap
// column so it meets the top of the radicand.
inline void radical(LGFX_Sprite& s, int x, int y, int scale, uint16_t color) {
  const float r = scale * 0.45f;
  auto P = [&](float u, float v, int& px, int& py) {
    px = x + int(u * scale + 0.5f);
    py = y + int(v * scale + 0.5f);
  };
  const float pts[][2] = {{0.1f, 4.3f}, {0.9f, 3.9f}, {1.9f, 6.3f}, {3.3f, 0.45f}, {5.0f, 0.45f}};
  for (int i = 0; i + 1 < 5; ++i) {
    int x0, y0, x1, y1;
    P(pts[i][0], pts[i][1], x0, y0);
    P(pts[i + 1][0], pts[i + 1][1], x1, y1);
    s.drawWideLine(x0, y0, x1, y1, r, color);
  }
}

// One glyph at (x, y), each font pixel drawn `scale` x `scale` LCD pixels.
inline void glyph(LGFX_Sprite& s, int x, int y, const calc::Glyph* g, int scale, uint16_t color) {
  if (g->cp == 0x221A && scale >= 2) {
    radical(s, x, y, scale, color);
    return;
  }
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

// The tight advance: the font's own 5-px cell (its 1-px gap) x scale, no extra air. For
// bars and long lines that would not fit at advance().
inline int tight(int scale) { return kGlyphW * scale; }

// `adv` < 0: advance(scale).
inline int width(const std::string& utf8, int scale, int adv = -1) {
  return int(calc::decodeUtf8(utf8).size()) * (adv < 0 ? advance(scale) : adv);
}

// UTF-8 text from (x, y). Returns the x after the last character.
inline int text(LGFX_Sprite& s, int x, int y, const std::string& utf8, int scale, uint16_t color, int adv = -1) {
  if (adv < 0) adv = advance(scale);
  for (uint32_t cp : calc::decodeUtf8(utf8)) {
    glyph(s, x, y, calc::findGlyph(cp), scale, color);
    x += adv;
  }
  return x;
}

inline void textCentered(LGFX_Sprite& s, int cx, int y, const std::string& utf8, int scale, uint16_t color,
                         int adv = -1) {
  text(s, cx - width(utf8, scale, adv) / 2, y, utf8, scale, color, adv);
}

inline void textRight(LGFX_Sprite& s, int xRight, int y, const std::string& utf8, int scale, uint16_t color,
                      int adv = -1) {
  text(s, xRight - width(utf8, scale, adv), y, utf8, scale, color, adv);
}

// A line that must stay inside `maxW` px: the normal advance if it fits, else the tight
// one, else (still too long) cut at a character with a trailing "..". Returns the x after it.
inline int textFit(LGFX_Sprite& s, int x, int y, const std::string& utf8, int scale, uint16_t color, int maxW) {
  if (width(utf8, scale) <= maxW) return text(s, x, y, utf8, scale, color);
  const int adv = tight(scale);
  std::vector<uint32_t> cps = calc::decodeUtf8(utf8);
  if (int(cps.size()) * adv > maxW) {
    const int keep = maxW / adv - 2;
    cps.resize(keep > 0 ? keep : 0);
    cps.push_back('.');
    cps.push_back('.');
  }
  return text(s, x, y, calc::encodeUtf8(cps), scale, color, adv);
}

}  // namespace glyphdraw
