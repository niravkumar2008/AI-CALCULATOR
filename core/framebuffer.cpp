#include "framebuffer.h"

#include "font.h"

namespace calc {

void Framebuffer::clear() {
  for (auto& row : px_)
    for (auto& p : row) p = 0;
  for (auto& i : icons_) i = false;
}

void Framebuffer::set(int x, int y, bool on) {
  if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return;
  px_[y][x] = on ? 1 : 0;
}

bool Framebuffer::get(int x, int y) const {
  if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return false;
  return px_[y][x] != 0;
}

void Framebuffer::fillRect(int x, int y, int w, int h, bool on) {
  for (int yy = y; yy < y + h; ++yy)
    for (int xx = x; xx < x + w; ++xx) set(xx, yy, on);
}

void Framebuffer::invertRect(int x, int y, int w, int h) {
  for (int yy = y; yy < y + h; ++yy)
    for (int xx = x; xx < x + w; ++xx) set(xx, yy, !get(xx, yy));
}

void Framebuffer::drawText(int col, int row, const std::string& utf8, bool underline) {
  const int y0 = row * 8;
  if (underline) fillRect(0, y0 + kCharH, kWidth, 1, true);
  drawTextPx(col * kCharW, y0, utf8, 1);
}

void Framebuffer::drawTextPx(int x, int y, const std::string& utf8, int scale) {
  if (scale < 1) scale = 1;
  const int cw = kCharW * scale;
  for (uint32_t cp : decodeUtf8(utf8)) {
    if (x + cw > kWidth + scale) break;  // the last glyph may drop its blank gap column
    const Glyph* g = findGlyph(cp);
    for (int r = 0; r < kCharH; ++r)
      for (int c = 0; c < kCharW; ++c)
        if (g->rows[r] & (1 << (4 - c))) fillRect(x + c * scale, y + r * scale, scale, scale, true);
    x += cw;
  }
}

std::string Framebuffer::toAscii() const {
  std::string s = "icons:";
  s += icons_[0] ? " UP" : " --";
  s += icons_[1] ? " DN" : " --";
  s += '\n';
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) s += px_[y][x] ? '#' : '.';
    s += '\n';
  }
  return s;
}

}  // namespace calc
