#include "font.h"

namespace calc {

namespace {
// Hand-drawn replacements for glyphs that read badly at 4 px wide:
// the font's ≈ looks like '%' and its √ like '/'.
const Glyph kExtra[] = {
    {0x2248, {0x00, 0x0C, 0x13, 0x00, 0x0C, 0x13, 0x00}},  // ≈
    {0x221A, {0x03, 0x02, 0x02, 0x12, 0x0A, 0x04, 0x00}},  // √
    {0x231F, {0x00, 0x00, 0x02, 0x02, 0x02, 0x1E, 0x00}},  // ⌟ fraction bar, as on the Casio
    {0x25B2, {0x00, 0x04, 0x0E, 0x1F, 0x00, 0x00, 0x00}},  // ▲
    {0x25BC, {0x00, 0x00, 0x1F, 0x0E, 0x04, 0x00, 0x00}},  // ▼
    {0x25C0, {0x02, 0x06, 0x0E, 0x06, 0x02, 0x00, 0x00}},  // ◀
    {0x25B6, {0x08, 0x0C, 0x0E, 0x0C, 0x08, 0x00, 0x00}},  // ▶
    {0xE000, {0x0C, 0x12, 0x1E, 0x1E, 0x16, 0x1E, 0x00}},  // padlock (exam mode)
    {0xE001, {0x0E, 0x11, 0x04, 0x0A, 0x00, 0x04, 0x00}},  // Wi-Fi
};

const Glyph* lookup(uint32_t cp) {
  for (const Glyph& g : kExtra)
    if (g.cp == cp) return &g;
  for (int i = 0; i < kGlyphCount; ++i)
    if (kGlyphs[i].cp == cp) return &kGlyphs[i];
  return nullptr;
}
}  // namespace

uint32_t normalizeCodepoint(uint32_t cp) {
  switch (cp) {
    case 0x00A0: case 0x2009: case 0x202F: case 0x2002: case 0x2003:
    case '\t': return ' ';
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014:
      return '-';
    case 0x2018: case 0x2019: return '\'';
    case 0x201C: case 0x201D: return '"';
    case 0x2217: case 0x22C5: return 0x00B7;  // asterisk op / dot op -> middle dot
    case 0x21D2: return 0x2192;               // => -> arrow
    case 0x2126: return 0x03A9;               // ohm sign -> omega
    case 0x212B: return 0x00C5;               // angstrom sign -> A ring
    case 0x2103: return 0x00B0;               // degree celsius sign -> degree (C follows)
    default: return cp;
  }
}

bool hasGlyph(uint32_t cp) { return lookup(normalizeCodepoint(cp)) != nullptr; }

const Glyph* findGlyph(uint32_t cp) {
  const Glyph* g = lookup(normalizeCodepoint(cp));
  if (g) return g;
  static const Glyph* q = lookup('?');
  return q;
}

std::vector<uint32_t> decodeUtf8(const std::string& s) {
  std::vector<uint32_t> out;
  size_t i = 0;
  while (i < s.size()) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    uint32_t cp;
    int extra;
    if (c < 0x80) { cp = c; extra = 0; }
    else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
    else { out.push_back('?'); ++i; continue; }
    if (i + extra >= s.size()) {  // truncated multi-byte sequence
      out.push_back('?');
      break;
    }
    bool ok = true;
    for (int k = 1; k <= extra; ++k) {
      unsigned char cc = static_cast<unsigned char>(s[i + k]);
      if ((cc & 0xC0) != 0x80) { ok = false; break; }
      cp = (cp << 6) | (cc & 0x3F);
    }
    if (!ok) { out.push_back('?'); ++i; continue; }
    out.push_back(cp);
    i += extra + 1;
  }
  return out;
}

std::string encodeUtf8(const std::vector<uint32_t>& cps) {
  std::string s;
  for (uint32_t cp : cps) {
    if (cp < 0x80) s += static_cast<char>(cp);
    else if (cp < 0x800) {
      s += static_cast<char>(0xC0 | (cp >> 6));
      s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      s += static_cast<char>(0xE0 | (cp >> 12));
      s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      s += static_cast<char>(0xF0 | (cp >> 18));
      s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      s += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }
  return s;
}

}  // namespace calc
