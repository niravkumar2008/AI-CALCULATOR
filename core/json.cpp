#include "json.h"

#include <cstdlib>

#include "font.h"

namespace calc {

const Json& Json::operator[](const std::string& key) const {
  static const Json kNull;
  if (type_ != Type::Object) return kNull;
  auto it = o_.find(key);
  return it == o_.end() ? kNull : it->second;
}

class JsonParser {
 public:
  explicit JsonParser(const std::string& t) : t_(t) {}

  bool parseDocument(Json& out, std::string& err) {
    skipWs();
    if (!parseValue(out, 0)) {
      err = err_.empty() ? "invalid JSON" : err_;
      return false;
    }
    skipWs();
    if (i_ != t_.size()) {
      err = "unexpected text after JSON at " + std::to_string(i_);
      return false;
    }
    return true;
  }

 private:
  const std::string& t_;
  size_t i_ = 0;
  std::string err_;

  bool fail(const std::string& m) {
    if (err_.empty()) err_ = m + " at " + std::to_string(i_);
    return false;
  }
  void skipWs() {
    while (i_ < t_.size() && (t_[i_] == ' ' || t_[i_] == '\n' || t_[i_] == '\r' || t_[i_] == '\t')) ++i_;
  }
  bool literal(const char* lit) {
    size_t n = 0;
    while (lit[n]) ++n;
    if (t_.compare(i_, n, lit) != 0) return false;
    i_ += n;
    return true;
  }

  bool parseValue(Json& v, int depth) {
    if (depth > Json::kMaxDepth) return fail("nesting too deep");
    skipWs();
    if (i_ >= t_.size()) return fail("unexpected end");
    char c = t_[i_];
    if (c == '{') return parseObject(v, depth);
    if (c == '[') return parseArray(v, depth);
    if (c == '"') {
      v.type_ = Json::Type::String;
      return parseString(v.s_);
    }
    if (literal("true")) { v.type_ = Json::Type::Bool; v.b_ = true; return true; }
    if (literal("false")) { v.type_ = Json::Type::Bool; v.b_ = false; return true; }
    if (literal("null")) { v.type_ = Json::Type::Null; return true; }
    if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(v);
    return fail("unexpected character");
  }

  bool parseNumber(Json& v) {
    size_t start = i_;
    if (t_[i_] == '-') ++i_;
    auto digits = [&]() {
      size_t s = i_;
      while (i_ < t_.size() && t_[i_] >= '0' && t_[i_] <= '9') ++i_;
      return i_ > s;
    };
    if (!digits()) return fail("bad number");
    if (i_ < t_.size() && t_[i_] == '.') {
      ++i_;
      if (!digits()) return fail("bad number");
    }
    if (i_ < t_.size() && (t_[i_] == 'e' || t_[i_] == 'E')) {
      ++i_;
      if (i_ < t_.size() && (t_[i_] == '+' || t_[i_] == '-')) ++i_;
      if (!digits()) return fail("bad number");
    }
    v.type_ = Json::Type::Number;
    v.n_ = std::strtod(t_.substr(start, i_ - start).c_str(), nullptr);
    return true;
  }

  bool hex4(uint32_t& out) {
    if (i_ + 4 > t_.size()) return fail("bad \\u escape");
    out = 0;
    for (int k = 0; k < 4; ++k) {
      char h = t_[i_++];
      out <<= 4;
      if (h >= '0' && h <= '9') out |= h - '0';
      else if (h >= 'a' && h <= 'f') out |= h - 'a' + 10;
      else if (h >= 'A' && h <= 'F') out |= h - 'A' + 10;
      else return fail("bad \\u escape");
    }
    return true;
  }

  // Strings end up on the screen: control characters other than \n (escaped ones: \b \f
  // \u0000 ...) become '?', \t and \r a space, a lone or mismatched surrogate '?', and bytes
  // that are not valid UTF-8 '?' (one per byte), so what is stored is always valid UTF-8.
  static void putCp(std::string& s, uint32_t cp) {
    if (cp >= 0x20 && cp < 0x7F) s += static_cast<char>(cp);  // printable ASCII: as is
    else if (cp == '\n') s += '\n';
    else if (cp == '\t' || cp == '\r') s += ' ';
    else if (cp < 0x20 || cp == 0x7F || (cp >= 0xD800 && cp <= 0xDFFF)) s += '?';
    else s += encodeUtf8({cp});
  }

  // A raw (unescaped) UTF-8 sequence starting with lead byte c at i_ - 1: appended if valid
  // (no overlongs, surrogates or code points past U+10FFFF), else '?' for the lead byte.
  void rawUtf8(std::string& s, unsigned char c) {
    int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : -1;
    if (c >= 0xF5 || c == 0xC0 || c == 0xC1) n = -1;
    if (n < 0 || i_ + static_cast<size_t>(n) > t_.size()) {
      s += '?';
      return;
    }
    uint32_t cp = c & (0x3F >> n);
    for (int k = 0; k < n; ++k) {
      const unsigned char b = static_cast<unsigned char>(t_[i_ + static_cast<size_t>(k)]);
      if ((b & 0xC0) != 0x80) {
        s += '?';
        return;
      }
      cp = (cp << 6) | (b & 0x3F);
    }
    static const uint32_t kMin[4] = {0, 0x80, 0x800, 0x10000};
    if (cp < kMin[n] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
      s += '?';
      return;
    }
    s.append(t_, i_ - 1, static_cast<size_t>(n) + 1);
    i_ += static_cast<size_t>(n);
  }

  bool parseString(std::string& s) {
    ++i_;  // opening quote
    s.clear();
    while (i_ < t_.size()) {
      char c = t_[i_++];
      if (c == '"') return true;
      const unsigned char u = static_cast<unsigned char>(c);
      if (u < 0x20) return fail("control character in string");
      if (u >= 0x80) {
        rawUtf8(s, u);
        continue;
      }
      if (c != '\\') { putCp(s, u); continue; }
      if (i_ >= t_.size()) break;
      char e = t_[i_++];
      switch (e) {
        case '"': s += '"'; break;
        case '\\': s += '\\'; break;
        case '/': s += '/'; break;
        case 'b': putCp(s, '\b'); break;
        case 'f': putCp(s, '\f'); break;
        case 'n': putCp(s, '\n'); break;
        case 'r': putCp(s, '\r'); break;
        case 't': putCp(s, '\t'); break;
        case 'u': {
          uint32_t cp;
          if (!hex4(cp)) return false;
          if (cp >= 0xD800 && cp <= 0xDBFF) {  // a high surrogate: a low one must follow
            uint32_t lo = 0;
            if (i_ + 2 <= t_.size() && t_[i_] == '\\' && t_[i_ + 1] == 'u') {
              const size_t back = i_;
              i_ += 2;
              if (!hex4(lo)) return false;
              if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
              } else {
                cp = '?';
                i_ = back;  // not a pair: the second \u is read on its own
              }
            } else {
              cp = '?';
            }
          }
          putCp(s, cp);  // a lone low surrogate (DC00-DFFF) is '?' too
          break;
        }
        default: return fail("bad escape");
      }
    }
    return fail("unterminated string");
  }

  bool parseArray(Json& v, int depth) {
    ++i_;
    v.type_ = Json::Type::Array;
    skipWs();
    if (i_ < t_.size() && t_[i_] == ']') { ++i_; return true; }
    while (true) {
      Json item;
      if (!parseValue(item, depth + 1)) return false;
      v.a_.push_back(std::move(item));
      skipWs();
      if (i_ >= t_.size()) return fail("unterminated array");
      if (t_[i_] == ',') { ++i_; continue; }
      if (t_[i_] == ']') { ++i_; return true; }
      return fail("expected , or ]");
    }
  }

  bool parseObject(Json& v, int depth) {
    ++i_;
    v.type_ = Json::Type::Object;
    skipWs();
    if (i_ < t_.size() && t_[i_] == '}') { ++i_; return true; }
    while (true) {
      skipWs();
      if (i_ >= t_.size() || t_[i_] != '"') return fail("expected key");
      std::string key;
      if (!parseString(key)) return false;
      skipWs();
      if (i_ >= t_.size() || t_[i_] != ':') return fail("expected :");
      ++i_;
      Json val;
      if (!parseValue(val, depth + 1)) return false;
      v.o_[key] = std::move(val);
      skipWs();
      if (i_ >= t_.size()) return fail("unterminated object");
      if (t_[i_] == ',') { ++i_; continue; }
      if (t_[i_] == '}') { ++i_; return true; }
      return fail("expected , or }");
    }
  }
};

bool Json::parse(const std::string& text, Json& out, std::string& err) {
  JsonParser p(text);
  out = Json();
  return p.parseDocument(out, err);
}

}  // namespace calc
