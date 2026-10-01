// 1-bit pixel buffer for the calculator screen: the Waveshare 2.13" e-paper
// (250x122) drawn at 2x, so 125x61 logical pixels = 25 columns x 7 text rows.
// Row 0 is the status bar (S, A, M, D/R/G, AI, arrows); rows 1-6 are content.
#pragma once
#include <cstdint>
#include <string>

namespace calc {

// Scroll arrows (drawn in the status bar by the device layer).
enum class Icon : uint8_t { Up, Down, Count };

class Framebuffer {
 public:
  static constexpr int kWidth = 125;
  static constexpr int kHeight = 61;
  static constexpr int kCols = kWidth / 5;  // 25 characters per line
  static constexpr int kRows = 7;           // 7 text rows, 8px pitch (row 0 = status)

  void clear();
  void set(int x, int y, bool on);
  bool get(int x, int y) const;
  void fillRect(int x, int y, int w, int h, bool on);
  void invertRect(int x, int y, int w, int h);

  // Draws UTF-8 text starting at a character column/row. Characters past the
  // right edge are clipped. underline=true draws a rule in the 1px gap under
  // the line (used for section headers).
  void drawText(int col, int row, const std::string& utf8, bool underline = false);
  // Same, at any pixel position and 1x or 2x size.
  void drawTextPx(int x, int y, const std::string& utf8, int scale = 1);

  void setIcon(Icon i, bool on) { icons_[static_cast<int>(i)] = on; }
  bool icon(Icon i) const { return icons_[static_cast<int>(i)]; }

  // '#' for lit pixels, '.' for unlit; one line per pixel row, prefixed by an
  // icon line. Used by snapshot tests and to skip identical e-paper refreshes.
  std::string toAscii() const;
  const uint8_t* pixels() const { return &px_[0][0]; }  // kHeight rows of kWidth bytes (0/1)

 private:
  uint8_t px_[kHeight][kWidth] = {};
  bool icons_[static_cast<int>(Icon::Count)] = {};
};

}  // namespace calc
