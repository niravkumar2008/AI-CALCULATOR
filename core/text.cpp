#include "text.h"

#include "font.h"

namespace calc {

int textLength(const std::string& utf8) {
  return static_cast<int>(decodeUtf8(utf8).size());
}

std::string utf8Prefix(const std::string& utf8, size_t maxBytes) {
  if (utf8.size() <= maxBytes) return utf8;
  size_t cut = maxBytes;
  // Back over continuation bytes (10xxxxxx) to the start of the character the cut is in
  // (at most 3: a longer run is broken text anyway, and is cut where it is).
  for (int i = 0; i < 3 && cut > 0 && (static_cast<unsigned char>(utf8[cut]) & 0xC0) == 0x80; ++i) --cut;
  if ((static_cast<unsigned char>(utf8[cut]) & 0xC0) == 0x80) cut = maxBytes;
  return utf8.substr(0, cut);
}

namespace {
void wrapParagraph(const std::vector<uint32_t>& cps, int width,
                   std::vector<std::string>& out) {
  // Split into words on spaces.
  std::vector<std::vector<uint32_t>> words;
  std::vector<uint32_t> cur;
  for (uint32_t cp : cps) {
    if (normalizeCodepoint(cp) == ' ') {
      if (!cur.empty()) words.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(cp);
    }
  }
  if (!cur.empty()) words.push_back(cur);
  if (words.empty()) {
    out.push_back("");
    return;
  }

  std::vector<uint32_t> line;
  auto flush = [&]() {
    out.push_back(encodeUtf8(line));
    line.clear();
  };
  for (auto word : words) {
    // Hard-break words that can never fit on one line.
    while (static_cast<int>(word.size()) > width) {
      if (!line.empty()) flush();
      line.assign(word.begin(), word.begin() + width);
      flush();
      word.erase(word.begin(), word.begin() + width);
    }
    if (word.empty()) continue;
    const int needed = static_cast<int>(line.size() + (line.empty() ? 0 : 1) + word.size());
    if (needed > width) flush();
    if (!line.empty()) line.push_back(' ');
    line.insert(line.end(), word.begin(), word.end());
  }
  if (!line.empty()) flush();
}
}  // namespace

std::vector<std::string> wrapText(const std::string& utf8, int width) {
  std::vector<std::string> out;
  if (width < 1) width = 1;
  std::vector<uint32_t> para;
  for (uint32_t cp : decodeUtf8(utf8)) {
    if (cp == '\r') continue;
    if (cp == '\n') {
      wrapParagraph(para, width, out);
      para.clear();
    } else {
      para.push_back(cp);
    }
  }
  wrapParagraph(para, width, out);
  return out;
}

}  // namespace calc
