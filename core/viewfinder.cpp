#include "viewfinder.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "font.h"

namespace calc {

// ---------------------------------------------------------------- Panel

void Panel::clear() { std::memset(px_, 0, sizeof px_); }

void Panel::set(int x, int y, bool on) {
  if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return;
  const uint8_t bit = uint8_t(0x80 >> (x & 7));
  if (on) px_[y][x >> 3] |= bit;
  else px_[y][x >> 3] &= uint8_t(~bit);
}

bool Panel::get(int x, int y) const {
  return x >= 0 && y >= 0 && x < kWidth && y < kHeight && (px_[y][x >> 3] & (0x80 >> (x & 7)));
}

void Panel::invert(int x, int y) {
  if (x >= 0 && y >= 0 && x < kWidth && y < kHeight) px_[y][x >> 3] ^= uint8_t(0x80 >> (x & 7));
}

void Panel::fillRect(int x, int y, int w, int h, bool on) {
  for (int j = y; j < y + h; ++j)
    for (int i = x; i < x + w; ++i) set(i, j, on);
}

void Panel::drawText(int x, int y, const std::string& utf8) {
  for (uint32_t cp : decodeUtf8(utf8)) {
    if (x + kCharW > kWidth + 1) break;
    const Glyph* g = findGlyph(cp);
    for (int r = 0; r < kCharH; ++r)
      for (int c = 0; c < kCharW; ++c)
        if (g->rows[r] & (1 << (4 - c))) set(x + c, y + r, true);
    x += kCharW;
  }
}

int Panel::diff(const Panel& o) const {
  int n = 0;
  for (int y = 0; y < kHeight; ++y)
    for (int b = 0; b < kStride; ++b)
      for (uint8_t d = px_[y][b] ^ o.px_[y][b]; d; d &= uint8_t(d - 1)) ++n;
  return n;
}

std::string Panel::toAscii() const {
  std::string s;
  s.reserve((kWidth + 1) * kHeight);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) s += get(x, y) ? '#' : '.';
    s += '\n';
  }
  return s;
}

// ---------------------------------------------------------------- image maths

namespace {
// 4x4 Bayer matrix: an ordered dither is stable from frame to frame (Floyd-Steinberg
// "boils"), so partial refreshes only change where the picture really changed.
const uint8_t kBayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

void percentiles(const uint8_t* g, int w, int h, int& lo, int& hi) {
  int hist[256] = {};
  for (int y = 0; y < h; y += 2)
    for (int x = 0; x < w; x += 2) ++hist[g[y * w + x]];
  const int n = ((h + 1) / 2) * ((w + 1) / 2);
  int acc = 0;
  lo = 0;
  hi = 255;
  for (int v = 0; v < 256; ++v)
    if ((acc += hist[v]) >= n * 2 / 100) {
      lo = v;
      break;
    }
  acc = 0;
  for (int v = 255; v >= 0; --v)
    if ((acc += hist[v]) >= n * 2 / 100) {
      hi = v;
      break;
    }
  if (hi - lo < 32) {  // flat scene: don't blow sensor noise up into a pattern
    const int mid = (hi + lo) / 2;
    lo = std::max(0, mid - 16);
    hi = std::min(255, mid + 16);
  }
}
}  // namespace

double focusMeasure(const uint8_t* g, int w, int h, int lo, int hi) {
  const int x0 = w / 4, x1 = w * 3 / 4, y0 = h / 4, y1 = h * 3 / 4;
  double sum = 0, sum2 = 0;
  int n = 0;
  for (int y = std::max(1, y0); y < std::min(h - 1, y1); ++y)
    for (int x = std::max(1, x0); x < std::min(w - 1, x1); ++x) {
      const int c = g[y * w + x];
      const double lap = 4.0 * c - g[y * w + x - 1] - g[y * w + x + 1] - g[(y - 1) * w + x] - g[(y + 1) * w + x];
      sum += lap;
      sum2 += lap * lap;
      ++n;
    }
  if (n == 0) return 0;
  const double mean = sum / n, var = sum2 / n - mean * mean;
  const double contrast = std::max(32, hi - lo);
  return var / (contrast * contrast) * 1000.0;
}

// ---------------------------------------------------------------- Viewfinder

void Viewfinder::start(uint32_t nowMs) {
  running_ = true;
  startMs_ = lastKeyMs_ = nowMs;
  frames_ = 0;
  sinceFull_ = fullEvery;  // the first frame is drawn in full
  flipped_ = 0;
  haveShown_ = false;
  info_ = FrameInfo();
  prev_.clear();
  image_.clear();
}

uint32_t Viewfinder::secondsLeft(uint32_t nowMs) const {
  const uint32_t used = nowMs - lastKeyMs_;
  return used >= kTimeoutMs ? 0 : (kTimeoutMs - used + 999) / 1000;
}

double Viewfinder::fps(uint32_t nowMs) const {
  const uint32_t ms = nowMs - startMs_;
  return ms ? frames_ * 1000.0 / ms : 0;
}

const FrameInfo& Viewfinder::feed(const uint8_t* g, int w, int h, bool afFocused, uint32_t nowMs) {
  (void)nowMs;
  ++frames_;
  FrameInfo fi;
  percentiles(g, w, h, fi.lo, fi.hi);
  fi.sharpness = focusMeasure(g, w, h, fi.lo, fi.hi);

  // Motion: mean change of a 32x24 thumbnail from the previous frame.
  std::vector<uint8_t> small(32 * 24);
  for (int y = 0; y < 24; ++y)
    for (int x = 0; x < 32; ++x) small[y * 32 + x] = g[(y * h / 24) * w + (x * w / 32)];
  if (prev_.size() == small.size()) {
    long d = 0;
    for (size_t i = 0; i < small.size(); ++i) d += std::abs(int(small[i]) - int(prev_[i]));
    fi.motion = double(d) / small.size();
  }
  const bool first = prev_.empty();
  prev_.swap(small);

  if (first) fi.state = Focus3::Starting;
  else if (fi.motion > kMotionMax) fi.state = Focus3::Moving;
  else if (!afFocused || fi.sharpness < kSharpMin) fi.state = Focus3::Focusing;
  else fi.state = Focus3::Sharp;

  // Text: 8x8 blocks with many strong edges; the box around the dense ones.
  const int bs = 8, bw = w / bs, bh = h / bs;
  const int edge = std::max(24, (fi.hi - fi.lo) / 3);
  int minX = bw, minY = bh, maxX = -1, maxY = -1, blocks = 0;
  for (int by = 0; by < bh; ++by)
    for (int bx = 0; bx < bw; ++bx) {
      int ex = 0, ey = 0;  // edges across x and across y: writing has both, a ruled line only one
      for (int y = by * bs; y < by * bs + bs && y + 1 < h; ++y)
        for (int x = bx * bs; x < bx * bs + bs && x + 1 < w; ++x) {
          const int c = g[y * w + x];
          ex += std::abs(c - g[y * w + x + 1]) > edge;
          ey += std::abs(c - g[(y + 1) * w + x]) > edge;
        }
      if (ex >= 3 && ey >= 3 && (ex + ey) * 100 >= bs * bs * 10) {  // 10 % of the block is edges, both ways
        ++blocks;
        minX = std::min(minX, bx);
        minY = std::min(minY, by);
        maxX = std::max(maxX, bx);
        maxY = std::max(maxY, by);
      }
    }
  if (blocks >= 3 && blocks < bw * bh * 3 / 4) {  // some writing, but not the whole frame of texture
    fi.textFound = true;
    fi.tx = minX * bs;
    fi.ty = minY * bs;
    fi.tw = (maxX - minX + 1) * bs;
    fi.th = (maxY - minY + 1) * bs;
  }

  // The whole frame, stretched, at the size it is drawn (nearest neighbour is
  // enough: the panel has fewer pixels than the frame in both directions).
  image_.assign(size_t(kImgW) * kImgH, 255);
  const int range = std::max(1, fi.hi - fi.lo);
  for (int y = 0; y < kImgH; ++y) {
    const int sy = y * h / kImgH;
    for (int x = 0; x < kImgW; ++x) {
      const int v = (int(g[sy * w + x * w / kImgW]) - fi.lo) * 255 / range;
      image_[size_t(y) * kImgW + x] = uint8_t(std::clamp(v, 0, 255));
    }
  }
  frameW_ = w;
  frameH_ = h;
  info_ = fi;
  return info_;
}

namespace {
void hLine(Panel& p, int x0, int x1, int y, bool inv) {
  for (int x = x0; x <= x1; ++x) inv ? p.invert(x, y) : p.set(x, y, true);
}
void vLine(Panel& p, int x, int y0, int y1, bool inv) {
  for (int y = y0; y <= y1; ++y) inv ? p.invert(x, y) : p.set(x, y, true);
}
// Corner brackets with a white edge so they show on any picture.
void bracket(Panel& p, int x, int y, int dx, int dy) {
  const int len = 14;
  for (int k = 0; k < 2; ++k) {  // white halo, then black line
    const bool on = k == 1;
    for (int i = -1; i <= len; ++i)
      for (int t = -1; t <= 2; ++t) {
        if (k == 1 && (t < 0 || t > 1 || i < 0 || i >= len)) continue;
        p.set(x + dx * i, y + dy * t, on);
        p.set(x + dx * t, y + dy * i, on);
      }
  }
}
}  // namespace

void Viewfinder::renderStarting(Panel& out) const {
  out.clear();
  out.fillRect(0, 0, kImgW, kImgH, false);
  for (int x = 0; x < kImgW; ++x) out.set(x, 0, true), out.set(x, kImgH - 1, true);
  for (int y = 0; y < kImgH; ++y) out.set(0, y, true), out.set(kImgW - 1, y, true);
  out.drawText(30, 57, "Starting camera...");
  out.drawText(kSideX, 0, "AI CAMERA");
  out.drawText(kSideX, 96, "=:photo");
  out.drawText(kSideX, 104, "AC:back");
}

void Viewfinder::render(Panel& out, uint32_t nowMs, const std::string& extra) const {
  out.clear();
  if (image_.empty()) return renderStarting(out);
  for (int y = 0; y < kImgH; ++y)
    for (int x = 0; x < kImgW; ++x)
      out.set(x, y, image_[size_t(y) * kImgW + x] <= kBayer[y & 3][x & 3] * 16 + 8);

  // Overlay: brackets at the corners of what is sent, a crosshair in the middle.
  bracket(out, 2, 2, 1, 1);
  bracket(out, kImgW - 3, 2, -1, 1);
  bracket(out, 2, kImgH - 3, 1, -1);
  bracket(out, kImgW - 3, kImgH - 3, -1, -1);
  const int cx = kImgW / 2, cy = kImgH / 2;
  hLine(out, cx - 8, cx - 3, cy, true);
  hLine(out, cx + 3, cx + 8, cy, true);
  vLine(out, cx, cy - 8, cy - 3, true);
  vLine(out, cx, cy + 3, cy + 8, true);

  // The text it found: a dashed box (frame -> panel coordinates).
  if (info_.textFound && frameW_ > 0) {
    const int x0 = info_.tx * kImgW / frameW_, x1 = (info_.tx + info_.tw) * kImgW / frameW_ - 1;
    const int y0 = info_.ty * kImgH / frameH_, y1 = (info_.ty + info_.th) * kImgH / frameH_ - 1;
    for (int x = x0; x <= x1; ++x)
      if ((x / 3) % 2 == 0) out.invert(x, y0), out.invert(x, y1);
    for (int y = y0 + 1; y < y1; ++y)
      if ((y / 3) % 2 == 0) out.invert(x0, y), out.invert(x1, y);
  }

  // Side panel.
  const int sx = kSideX;
  out.drawText(sx, 0, "AI CAMERA");
  for (int x = sx; x < Panel::kWidth; ++x) out.set(x, 8, true);
  switch (info_.state) {
    case Focus3::Starting: out.drawText(sx, 14, "Starting..."); break;
    case Focus3::Moving:
      out.drawText(sx, 14, "HOLD STILL");
      out.drawText(sx, 23, "moving");
      break;
    case Focus3::Focusing:
      out.drawText(sx, 14, "Focusing...");
      out.drawText(sx, 23, "hold still");
      break;
    case Focus3::Sharp: {
      out.fillRect(sx, 12, Panel::kWidth - sx, 11, true);  // reverse video: easy to see
      Panel text;
      text.drawText(sx + 2, 14, "✓ SHARP");
      for (int y = 14; y < 21; ++y)
        for (int x = sx; x < Panel::kWidth; ++x)
          if (text.get(x, y)) out.set(x, y, false);
      out.drawText(sx, 25, "press =");
      break;
    }
  }
  // Sharpness bar: 0 .. 2x the threshold.
  const int barW = Panel::kWidth - sx - 1;
  const int fill = int(std::min(1.0, info_.sharpness / (2 * kSharpMin)) * barW);
  for (int x = sx; x <= sx + barW; ++x) out.set(x, 36, true), out.set(x, 41, true);
  out.set(sx, 37, true), out.set(sx, 38, true), out.set(sx, 39, true), out.set(sx, 40, true);
  out.set(sx + barW, 37, true), out.set(sx + barW, 38, true), out.set(sx + barW, 39, true), out.set(sx + barW, 40, true);
  out.fillRect(sx + 1, 37, fill, 4, true);
  const int mark = sx + barW / 2;  // the "sharp" threshold
  out.set(mark, 34, true), out.set(mark, 35, true), out.set(mark, 42, true), out.set(mark, 43, true);
  out.drawText(sx, 46, "focus");
  if (info_.textFound) out.drawText(sx, 58, "text found");

  if (!extra.empty()) out.drawText(sx, 80, extra);
  out.drawText(sx, 96, "=:photo");
  out.drawText(sx, 104, "AC:back");
  out.drawText(sx, 114, "off in " + std::to_string(secondsLeft(nowMs)) + " s");
}

bool Viewfinder::takeFullRefresh(const Panel& now) {
  if (haveShown_) flipped_ += now.diff(shown_);
  shown_ = now;
  haveShown_ = true;
  if (++sinceFull_ > fullEvery || flipped_ > ghostBudget) {
    sinceFull_ = 0;
    flipped_ = 0;
    return true;
  }
  return false;
}

}  // namespace calc
