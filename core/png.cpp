#include "png.h"

#include "font.h"
#include "text.h"

namespace calc {

uint32_t pngCrc32(const uint8_t* data, size_t n, uint32_t crc) {
  // Bit by bit (no 1 KB table): a typed problem's PNG is ~100 KB, a few ms on the ESP32.
  crc = ~crc;
  for (size_t i = 0; i < n; ++i) {
    crc ^= data[i];
    for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

uint32_t pngAdler32(const uint8_t* data, size_t n) {
  uint32_t a = 1, b = 0;
  for (size_t i = 0; i < n; ++i) {
    a = (a + data[i]) % 65521u;
    b = (b + a) % 65521u;
  }
  return (b << 16) | a;
}

namespace {
void putBE32(std::string& o, uint32_t v) {
  for (int i = 3; i >= 0; --i) o += static_cast<char>((v >> (8 * i)) & 0xFF);
}

// length, type, data, CRC of type + data
void chunk(std::string& out, const char* type, const std::string& data) {
  putBE32(out, static_cast<uint32_t>(data.size()));
  const std::string body = std::string(type, 4) + data;
  out += body;
  putBE32(out, pngCrc32(reinterpret_cast<const uint8_t*>(body.data()), body.size()));
}
}  // namespace

std::string encodePngGray(const std::vector<uint8_t>& pixels, int w, int h) {
  if (w <= 0 || h <= 0 || pixels.size() != static_cast<size_t>(w) * static_cast<size_t>(h)) return "";
  // The scanlines: a filter byte (0 = none) before each row.
  std::string raw;
  raw.reserve(static_cast<size_t>(w + 1) * static_cast<size_t>(h));
  for (int y = 0; y < h; ++y) {
    raw += '\0';
    raw.append(reinterpret_cast<const char*>(&pixels[static_cast<size_t>(y) * w]), static_cast<size_t>(w));
  }
  // zlib stream: header (deflate, 32 KB window, no dictionary; 0x7801 % 31 == 0),
  // stored blocks of at most 65535 bytes, Adler-32 of the raw data.
  std::string z;
  z += '\x78';
  z += '\x01';
  size_t pos = 0;
  do {
    const size_t n = raw.size() - pos < 65535 ? raw.size() - pos : 65535;
    const bool last = pos + n == raw.size();
    z += static_cast<char>(last ? 1 : 0);  // BFINAL, BTYPE 00 = stored
    z += static_cast<char>(n & 0xFF);
    z += static_cast<char>((n >> 8) & 0xFF);
    z += static_cast<char>(~n & 0xFF);
    z += static_cast<char>((~n >> 8) & 0xFF);
    z.append(raw, pos, n);
    pos += n;
  } while (pos < raw.size());
  putBE32(z, pngAdler32(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()));

  std::string ihdr;
  putBE32(ihdr, static_cast<uint32_t>(w));
  putBE32(ihdr, static_cast<uint32_t>(h));
  ihdr += '\x08';  // bit depth
  ihdr += '\x00';  // colour type 0: grayscale
  ihdr += '\x00';  // compression: deflate
  ihdr += '\x00';  // filter method 0
  ihdr += '\x00';  // no interlace

  std::string out("\x89PNG\r\n\x1a\n", 8);
  chunk(out, "IHDR", ihdr);
  chunk(out, "IDAT", z);
  chunk(out, "IEND", "");
  return out;
}

bool blankText(const std::string& utf8) {
  for (uint32_t cp : decodeUtf8(utf8))
    if (normalizeCodepoint(cp) != ' ' && cp != '\n' && cp != '\r') return false;
  return true;
}

bool renderTextImage(const std::string& utf8, std::vector<uint8_t>& pixels, int& w, int& h, int scale) {
  if (blankText(utf8)) return false;
  if (scale < 1) scale = 1;
  const std::vector<std::string> lines = wrapText(utf8, kTypedCols);
  const int margin = 8 * scale, cell = kCharW * scale, pitch = 10 * scale;  // 3 dots between lines
  w = 2 * margin + kTypedCols * cell - scale;                              // the last gap column isn't needed
  h = 2 * margin + static_cast<int>(lines.size()) * pitch - 3 * scale;
  pixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 255);
  for (size_t l = 0; l < lines.size(); ++l) {
    int x = margin;
    const int y = margin + static_cast<int>(l) * pitch;
    for (uint32_t cp : decodeUtf8(lines[l])) {
      const Glyph* g = findGlyph(cp);
      for (int r = 0; r < kCharH; ++r)
        for (int c = 0; c < kCharW; ++c)
          if (g->rows[r] & (1 << (4 - c)))
            for (int dy = 0; dy < scale; ++dy)
              for (int dx = 0; dx < scale; ++dx) {
                const int px = x + c * scale + dx, py = y + r * scale + dy;
                if (px < w && py < h) pixels[static_cast<size_t>(py) * w + px] = 0;
              }
      x += cell;
    }
  }
  return true;
}

std::string renderTextPng(const std::string& utf8, int scale) {
  std::vector<uint8_t> px;
  int w = 0, h = 0;
  if (!renderTextImage(utf8, px, w, h, scale)) return "";
  return encodePngGray(px, w, h);
}

}  // namespace calc
