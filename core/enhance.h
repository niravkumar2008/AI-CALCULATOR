// Turns a gray photo of writing into a clean "scan" for Claude to read:
//  1. evens out the lighting (a shadow across the page, a bright lamp spot)
//     by dividing each pixel by the paper brightness around it,
//  2. stretches contrast so ink goes near black and paper white,
//  3. resizes to about maxPixels (enlarging a small close-up, shrinking a
//     whole frame) with bilinear interpolation,
//  4. sharpens stroke edges (unsharp mask).
// Works in place on one buffer plus a second for the resize, so a full
// 1600x1200 frame needs about 3 MB.
#pragma once
#include <cstdint>
#include <vector>

namespace calc {

// img: w*h gray bytes, replaced by the result; w/h updated. yield (optional)
// is called every few dozen rows so a long run doesn't starve other tasks.
void enhanceWriting(std::vector<uint8_t>& img, int& w, int& h, long maxPixels = 1150000,
                    void (*yield)() = nullptr);

}  // namespace calc
