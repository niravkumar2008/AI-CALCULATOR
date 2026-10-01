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
    if (depth > 64) return fail("nesting too deep");
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

  bool parseString(std::string& s) {
    ++i_;  // opening quote
    s.clear();
    while (i_ < t_.size()) {
      char c = t_[i_++];
      if (c == '"') return true;
      if (static_cast<unsigned char>(c) < 0x20) return fail("control character in string");
      if (c != '\\') { s += c; continue; }
      if (i_ >= t_.size()) break;
      char e = t_[i_++];
      switch (e) {
        case '"': s += '"'; break;
        case '\\': s += '\\'; break;
        case '/': s += '/'; break;
        case 'b': s += '\b'; break;
        case 'f': s += '\f'; break;
        case 'n': s += '\n'; break;
        case 'r': s += '\r'; break;
        case 't': s += '\t'; break;
        case 'u': {
          uint32_t cp;
          if (!hex4(cp)) return false;
          if (cp >= 0xD800 && cp <= 0xDBFF) {  // surrogate pair
            uint32_t lo;
            if (i_ + 2 <= t_.size() && t_[i_] == '\\' && t_[i_ + 1] == 'u') {
              i_ += 2;
              if (!hex4(lo)) return false;
              cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
            } else {
              cp = '?';
            }
          }
          s += encodeUtf8({cp});
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
