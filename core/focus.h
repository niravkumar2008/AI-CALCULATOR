// Finds the writing in a photo and how sharp it is, from a small grayscale
// copy (the firmware decodes each JPEG at 1/4 size: 400x300 for UXGA).
//
// Sharpness: the Laplacian (how much each pixel differs from its 4
// neighbours) is large on crisp ink edges and small when focus or a shaky
// hand smears them. Measured only where the writing is, so a large blank
// page doesn't drag a sharp photo's score down.
//
// Where the writing is: 8x8 blocks with far more edge energy than the page
// around them. Their bounding box becomes a full-resolution close-up for
// Claude, so small characters get more pixels than in the whole photo.
#pragma once
#include <cstdint>

namespace calc {

struct Focus {
  double score = 0;   // higher = sharper writing
  bool found = false; // false: no writing-like detail (blank page, lens cap)
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;  // writing box in the small image, x1/y1 exclusive
  double noise = 0;   // the page's typical (median) edge energy: grain and sensor noise
};

// g: w*h grayscale bytes, row by row.
Focus analyseFocus(const uint8_t* g, int w, int h);

// Lighting of a photo, from the same small gray copy.
struct Exposure {
  int dark = 0;        // 5th percentile brightness inside the writing box: the ink
  int paper = 0;       // 90th percentile brightness of the whole photo: the paper
  double clipped = 0;  // share of pixels at 250+ (washed-out white, detail lost)
};
// f: where the writing is (ink is a tiny share of a whole page, so it is
// measured inside the box); without writing, `dark` is the whole photo's.
Exposure measureExposure(const uint8_t* g, int w, int h, const Focus& f);

// How good a photo is for reading: sharp, clean ink edges (writing edge
// energy over the page's noise, so a noisy high-gain photo doesn't win just by
// being grainy), bright-but-not-washed-out paper. Higher is better; 0 when no
// writing was found. Used to pick camera settings for the light.
double scanQuality(const Focus& f, const Exposure& e);

struct Crop {
  bool use = false;  // false: the writing fills most of the photo; no close-up needed
  int x = 0, y = 0, w = 0, h = 0;  // in full-resolution pixels, multiples of 8
};

// The close-up to send: the writing box scaled up to the full photo, padded,
// at least 640x480, at most maxPixels.
Crop detailCrop(const Focus& f, int smallW, int smallH, int fullW, int fullH, long maxPixels = 1150000);

}  // namespace calc
