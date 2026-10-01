#include "enhance.h"

#include <algorithm>
#include <cmath>

namespace calc {

namespace {
constexpr int kBlock = 32;  // lighting is estimated on 32x32-pixel blocks

inline uint8_t clampByte(int v) { return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v); }
}  // namespace

void enhanceWriting(std::vector<uint8_t>& img, int& w, int& h, long maxPixels, void (*yield)()) {
  if (w < 8 || h < 8 || img.size() < size_t(w) * h) return;
  auto pause = [&](int y) {
    if (yield && y % 32 == 31) yield();
  };

  // 1. Paper brightness per block: its 90th percentile (ink is the dark minority).
  const int bw = (w + kBlock - 1) / kBlock, bh = (h + kBlock - 1) / kBlock;
  std::vector<float> paper(bw * bh);
  for (int by = 0; by < bh; ++by) {
    for (int bx = 0; bx < bw; ++bx) {
      int hist[64] = {}, n = 0;
      for (int y = by * kBlock; y < std::min(h, (by + 1) * kBlock); ++y)
        for (int x = bx * kBlock; x < std::min(w, (bx + 1) * kBlock); ++x, ++n) ++hist[img[y * w + x] >> 2];
      int seen = 0, v = 63;
      for (int i = 0; i < 64; ++i)
        if ((seen += hist[i]) >= n * 9 / 10) {
          v = i;
          break;
        }
      paper[by * bw + bx] = std::max(16, v * 4 + 2);
    }
    pause(by * kBlock + kBlock - 1);
  }
  // Smooth the block map so block edges never show.
  for (int pass = 0; pass < 2; ++pass) {
    std::vector<float> s(paper.size());
    for (int by = 0; by < bh; ++by)
      for (int bx = 0; bx < bw; ++bx) {
        // Edges mirror the inside, so a lighting gradient isn't bent at the border.
        float sum = 0;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx) {
            int x = bx + dx, y = by + dy;
            if (x < 0 || x >= bw) x = bx - dx;
            if (y < 0 || y >= bh) y = by - dy;
            x = std::min(std::max(x, 0), bw - 1);
            y = std::min(std::max(y, 0), bh - 1);
            sum += paper[y * bw + x];
          }
        s[by * bw + bx] = sum / 9;
      }
    paper.swap(s);
  }

  // 2a. Divide by the local paper brightness (bilinear between block centres).
  int hist[256] = {};
  for (int y = 0; y < h; ++y) {
    const float fy = std::min(std::max((y + 0.5f) / kBlock - 0.5f, 0.0f), float(bh - 1));
    const int y0 = int(fy), y1 = std::min(y0 + 1, bh - 1);
    const float ty = fy - y0;
    for (int x = 0; x < w; ++x) {
      const float fx = std::min(std::max((x + 0.5f) / kBlock - 0.5f, 0.0f), float(bw - 1));
      const int x0 = int(fx), x1 = std::min(x0 + 1, bw - 1);
      const float tx = fx - x0;
      const float bg = (paper[y0 * bw + x0] * (1 - tx) + paper[y0 * bw + x1] * tx) * (1 - ty) +
                       (paper[y1 * bw + x0] * (1 - tx) + paper[y1 * bw + x1] * tx) * ty;
      uint8_t& p = img[y * w + x];
      p = clampByte(int(p * 230.0f / bg + 0.5f));
      ++hist[p];
    }
    pause(y);
  }
  // 2b. Stretch: 1st percentile (ink) -> 0, 90th (paper) -> 255, by at most
  // 2x so faint lighting left-overs in the paper don't turn into blotches.
  const long n = long(w) * h;
  auto percentile = [&](double q) {
    long seen = 0;
    for (int v = 0; v < 256; ++v)
      if ((seen += hist[v]) > long(q * n)) return v;
    return 255;
  };
  const int hi = percentile(0.90), lo = std::min(percentile(0.01), hi - 128);
  uint8_t lut[256];
  for (int v = 0; v < 256; ++v) lut[v] = clampByte((v - lo) * 255 / (hi - lo));
  for (auto& p : img) p = lut[p];

  // 3. Resize to about maxPixels (only when it changes the size by 15%+).
  double scale = std::sqrt(double(maxPixels) / n);
  scale = std::min(scale, 2.5);  // enlarging further only invents blur
  if (scale > 1.15 || scale < 0.87) {
    const int nw = std::max(8, int(w * scale)), nh = std::max(8, int(h * scale));
    std::vector<uint8_t> out(size_t(nw) * nh);
    for (int y = 0; y < nh; ++y) {
      const float fy = std::min(std::max((y + 0.5f) * h / nh - 0.5f, 0.0f), float(h - 1));
      const int y0 = int(fy), y1 = std::min(y0 + 1, h - 1);
      const float ty = fy - y0;
      for (int x = 0; x < nw; ++x) {
        const float fx = std::min(std::max((x + 0.5f) * w / nw - 0.5f, 0.0f), float(w - 1));
        const int x0 = int(fx), x1 = std::min(x0 + 1, w - 1);
        const float tx = fx - x0;
        const float v = (img[y0 * w + x0] * (1 - tx) + img[y0 * w + x1] * tx) * (1 - ty) +
                        (img[y1 * w + x0] * (1 - tx) + img[y1 * w + x1] * tx) * ty;
        out[size_t(y) * nw + x] = uint8_t(v + 0.5f);
      }
      pause(y);
    }
    img.swap(out);
    w = nw;
    h = nh;
  }

  // 4. Unsharp mask: v + 0.8 * (v - 3x3 mean), using the original rows.
  std::vector<uint8_t> above(img.begin(), img.begin() + w), cur(w);
  for (int y = 1; y < h - 1; ++y) {
    std::copy(img.begin() + size_t(y) * w, img.begin() + size_t(y + 1) * w, cur.begin());
    const uint8_t* below = &img[size_t(y + 1) * w];
    for (int x = 1; x < w - 1; ++x) {
      const int sum = above[x - 1] + above[x] + above[x + 1] + cur[x - 1] + cur[x] + cur[x + 1] +
                      below[x - 1] + below[x] + below[x + 1];
      img[size_t(y) * w + x] = clampByte(cur[x] + (cur[x] * 9 - sum) * 8 / 90);
    }
    above.swap(cur);
    pause(y);
  }
}

}  // namespace calc
