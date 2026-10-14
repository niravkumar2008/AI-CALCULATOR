// "Type it": the typed problem drawn as a picture, so it goes to Claude through
// the same photo request as a camera shot (the server and its limits don't change).
// A minimal PNG writer with no library: 8-bit grayscale, zlib "stored"
// (uncompressed) blocks, CRC-32 and Adler-32 written out by hand.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace calc {

// The checksums PNG needs: CRC-32 (ISO 3309, as in zip and PNG chunks) and
// zlib's Adler-32. Exposed for the tests that decode what we write.
uint32_t pngCrc32(const uint8_t* data, size_t n, uint32_t crc = 0);
uint32_t pngAdler32(const uint8_t* data, size_t n);

// A PNG file (bytes) of a w x h 8-bit grayscale image, one byte per pixel,
// rows top to bottom (0 = black, 255 = white). "" for an empty or mismatched image.
std::string encodePngGray(const std::vector<uint8_t>& pixels, int w, int h);

// Draws text black on white with the calculator's 5x7 font scaled up (`scale`
// pixels per font dot), word-wrapped at kTypedCols characters, with margins.
// False (and nothing drawn) when the text is blank: an empty text can't be sent.
constexpr int kTypedCols = 20;
bool renderTextImage(const std::string& utf8, std::vector<uint8_t>& pixels, int& w, int& h, int scale = 3);

// renderTextImage() then encodePngGray(): the bytes the platform sends instead
// of a camera JPEG. "" when the text is blank.
std::string renderTextPng(const std::string& utf8, int scale = 3);

// True when the text has nothing but spaces in it.
bool blankText(const std::string& utf8);

}  // namespace calc
