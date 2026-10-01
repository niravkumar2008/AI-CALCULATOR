#include "focus.h"

#include <algorithm>
#include <vector>

namespace calc {

namespace {
constexpr int kBlock = 8;
}

Focus analyseFocus(const uint8_t* g, int w, int h) {
  Focus f;
  const int bw = w / kBlock, bh = h / kBlock;
  if (!g || bw < 3 || bh < 3) return f;

  // Mean squared Laplacian per block (border pixels skipped).
  std::vector<double> e(bw * bh, 0.0);
  for (int by = 0; by < bh; ++by)
    for (int bx = 0; bx < bw; ++bx) {
      double sum = 0;
      int n = 0;
      for (int y = by * kBlock; y < (by + 1) * kBlock; ++y) {
        if (y == 0 || y == h - 1) continue;
        for (int x = bx * kBlock; x < (bx + 1) * kBlock; ++x) {
          if (x == 0 || x == w - 1) continue;
          const int p = y * w + x;
          const int lap = 4 * g[p] - g[p - 1] - g[p + 1] - g[p - w] - g[p + w];
          sum += double(lap) * lap;
          ++n;
        }
      }
      e[by * bw + bx] = n ? sum / n : 0;
    }

  // Writing = blocks well above the page's typical (median) texture and above
  // sensor noise. Lone blocks are specks, not writing.
  std::vector<double> sorted(e);
  std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
  const double threshold = std::max(40.0, 4.0 * sorted[sorted.size() / 2]);
  f.noise = sorted[sorted.size() / 2];
  std::vector<char> ink(bw * bh, 0);
  for (int i = 0; i < bw * bh; ++i) ink[i] = e[i] > threshold;

  int x0 = bw, y0 = bh, x1 = -1, y1 = -1, count = 0;
  double total = 0;
  for (int by = 0; by < bh; ++by)
    for (int bx = 0; bx < bw; ++bx) {
      if (!ink[by * bw + bx]) continue;
      bool neighbour = false;
      for (int dy = -1; dy <= 1 && !neighbour; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
          const int nx = bx + dx, ny = by + dy;
          if ((dx || dy) && nx >= 0 && ny >= 0 && nx < bw && ny < bh && ink[ny * bw + nx]) {
            neighbour = true;
            break;
          }
        }
      if (!neighbour) continue;
      x0 = std::min(x0, bx);
      y0 = std::min(y0, by);
      x1 = std::max(x1, bx);
      y1 = std::max(y1, by);
      total += e[by * bw + bx];
      ++count;
    }

  if (count == 0) {  // nothing writing-like: score the whole frame
    double all = 0;
    for (double v : e) all += v;
    f.score = all / e.size();
    return f;
  }
  f.found = true;
  f.score = total / count;
  f.x0 = x0 * kBlock;
  f.y0 = y0 * kBlock;
  f.x1 = (x1 + 1) * kBlock;
  f.y1 = (y1 + 1) * kBlock;
  return f;
}

Exposure measureExposure(const uint8_t* g, int w, int h, const Focus& f) {
  Exposure e;
  if (!g || w <= 0 || h <= 0) return e;
  // Percentile of a histogram.
  auto percentile = [](const long* hist, long n, double p) {
    const long want = long(p * n);
    long seen = 0;
    for (int v = 0; v < 256; ++v)
      if ((seen += hist[v]) > want) return v;
    return 255;
  };
  long all[256] = {}, box[256] = {};
  long inBox = 0;
  const bool useBox = f.found && f.x1 > f.x0 && f.y1 > f.y0;
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      const uint8_t v = g[y * w + x];
      ++all[v];
      if (useBox && x >= f.x0 && x < f.x1 && y >= f.y0 && y < f.y1) {
        ++box[v];
        ++inBox;
      }
    }
  const long n = long(w) * h;
  e.paper = percentile(all, n, 0.90);
  e.dark = inBox ? percentile(box, inBox, 0.05) : percentile(all, n, 0.05);
  long bright = 0;
  for (int v = 250; v < 256; ++v) bright += all[v];
  e.clipped = double(bright) / n;
  return e;
}

double scanQuality(const Focus& f, const Exposure& e) {
  if (!f.found) return 0;
  double q = f.score / (f.noise + 10.0);  // clean edges, not grain
  // Paper should be bright: below ~190 the ink and paper start to merge.
  if (e.paper < 190) q *= std::max(0.0, e.paper / 190.0);
  // Washed-out white loses pencil strokes: heavy penalty past 1%.
  if (e.clipped > 0.01) q *= std::max(0.0, 1.0 - (e.clipped - 0.01) * 10.0);
  // Ink should stay clearly darker than paper.
  const int gap = e.paper - e.dark;
  if (gap < 80) q *= std::max(0.0, gap / 80.0);
  return q;
}

Crop detailCrop(const Focus& f, int smallW, int smallH, int fullW, int fullH, long maxPixels) {
  Crop c;
  if (!f.found || smallW <= 0 || smallH <= 0 || fullW < 640 || fullH < 480) return c;
  const double sx = double(fullW) / smallW, sy = double(fullH) / smallH;
  // Pad by 4% of the frame so characters on the box edge aren't clipped.
  const int padX = fullW / 25, padY = fullH / 25;
  int x0 = int(f.x0 * sx) - padX, y0 = int(f.y0 * sy) - padY;
  int x1 = int(f.x1 * sx) + padX, y1 = int(f.y1 * sy) + padY;
  // At least 640x480, grown around the centre.
  auto grow = [](int& a, int& b, int minLen, int limit) {
    const int need = minLen - (b - a);
    if (need > 0) {
      a -= need / 2;
      b += need - need / 2;
    }
    if (a < 0) { b -= a; a = 0; }
    if (b > limit) { a -= b - limit; b = limit; }
    a = std::max(a, 0);
  };
  grow(x0, x1, 640, fullW);
  grow(y0, y1, 480, fullH);
  // Multiples of 8: what the JPEG encoder works in.
  x0 &= ~7;
  y0 &= ~7;
  int w = (x1 - x0) & ~7, h = (y1 - y0) & ~7;
  if (x0 + w > fullW) w = (fullW - x0) & ~7;
  if (y0 + h > fullH) h = (fullH - y0) & ~7;
  // A close-up only helps when it is clearly smaller than the whole photo.
  if (long(w) * h > maxPixels || long(w) * h * 10 > long(fullW) * fullH * 6) return c;
  c.use = true;
  c.x = x0;
  c.y = y0;
  c.w = w;
  c.h = h;
  return c;
}

}  // namespace calc
