// Minimal JSON reader (no dependencies) for parsing Claude's replies.
#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace calc {

class Json {
 public:
  enum class Type { Null, Bool, Number, String, Array, Object };

  // Nesting allowed in a document (deeper: a parse error). Claude's replies and the Worker's
  // need 4. Parsing, copying and destroying a tree all recurse once per level, so this bounds
  // their stack too (~2 KB on the ESP32 at 20, where 64 levels took ~6 KB to destroy).
  static constexpr int kMaxDepth = 20;

  Type type() const { return type_; }
  bool isNull() const { return type_ == Type::Null; }
  bool isBool() const { return type_ == Type::Bool; }
  bool isNumber() const { return type_ == Type::Number; }
  bool isString() const { return type_ == Type::String; }
  bool isArray() const { return type_ == Type::Array; }
  bool isObject() const { return type_ == Type::Object; }

  bool asBool() const { return b_; }
  double asNumber() const { return n_; }
  const std::string& asString() const { return s_; }
  const std::vector<Json>& items() const { return a_; }
  const std::map<std::string, Json>& members() const { return o_; }

  // Object lookup; returns a shared Null value when missing or not an object.
  const Json& operator[](const std::string& key) const;

  // Parses a complete JSON document. On failure returns false and sets err.
  static bool parse(const std::string& text, Json& out, std::string& err);

 private:
  friend class JsonParser;
  Type type_ = Type::Null;
  bool b_ = false;
  double n_ = 0;
  std::string s_;
  std::vector<Json> a_;
  std::map<std::string, Json> o_;
};

}  // namespace calc
