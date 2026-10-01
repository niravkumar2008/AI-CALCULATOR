// 5x7 bitmap font and UTF-8 helpers for the calculator's dot-matrix screen.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace calc {

constexpr int kCharW = 5;  // cell width in pixels (glyph + 1px gap)
constexpr int kCharH = 7;  // cell height in pixels

struct Glyph {
  uint32_t cp;
  uint8_t rows[7];  // low 5 bits per row, bit 4 = leftmost pixel
};

extern const Glyph kGlyphs[];
extern const int kGlyphCount;

// Returns the glyph for a code point after normalisation; never null
// (unknown characters render as '?').
const Glyph* findGlyph(uint32_t cp);

// True if the code point has its own glyph (no '?' fallback needed).
bool hasGlyph(uint32_t cp);

// Maps look-alike characters Claude may send (curly quotes, dashes,
// non-breaking spaces...) onto characters the font has.
uint32_t normalizeCodepoint(uint32_t cp);

std::vector<uint32_t> decodeUtf8(const std::string& s);
std::string encodeUtf8(const std::vector<uint32_t>& cps);

}  // namespace calc
