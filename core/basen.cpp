#include "basen.h"

#include <cmath>

namespace calc {

namespace {

struct BaseFail {
  CalcError e;
  int pos;
};

constexpr int64_t kMin32 = -2147483648LL, kMax32 = 2147483647LL;

int radixOf(Base b) { return b == Base::Hex ? 16 : b == Base::Bin ? 2 : b == Base::Oct ? 8 : 10; }

// A digit token's value (0-9, A-F), or -1.
int digitValue(Tok t) {
  if (t <= Tok::D9) return static_cast<int>(t);
  if (t >= Tok::HexA && t <= Tok::HexF) return 10 + static_cast<int>(t) - static_cast<int>(Tok::HexA);
  return -1;
}

bool isValueTok(Tok t) { return t >= Tok::Ans && t <= Tok::VarM && t != Tok::Ran; }  // Ans PreAns A-F X Y M
bool startsOperandTok(Tok t) {
  return digitValue(t) >= 0 || (t >= Tok::PreD && t <= Tok::PreO) || t == Tok::Open || t == Tok::Not ||
         t == Tok::BNeg || isValueTok(t);
}

class BaseParser {
 public:
  BaseParser(const std::vector<Tok>& t, const Vars& v, Base b) : t_(t), n_(t.size()), vars_(v), base_(b) {}

  int64_t run() {
    if (n_ == 0) throw BaseFail{CalcError::Syntax, 0};
    const int64_t r = orLevel();
    if (i_ < n_) syntax();
    return r;
  }

 private:
  const std::vector<Tok>& t_;
  size_t n_;
  const Vars& vars_;
  Base base_;
  size_t i_ = 0;
  int depth_ = 0;

  bool at(Tok k) const { return i_ < n_ && t_[i_] == k; }
  bool end() const { return i_ >= n_; }
  int mathPos() const { return static_cast<int>(i_ > 0 ? i_ - 1 : 0); }
  [[noreturn]] void syntax() const { throw BaseFail{CalcError::Syntax, static_cast<int>(i_ < n_ ? i_ : n_)}; }
  [[noreturn]] void math() const { throw BaseFail{CalcError::Math, mathPos()}; }
  int64_t check(int64_t v) const {  // every step stays in 32 bits
    if (v < kMin32 || v > kMax32) math();
    return v;
  }
  void close() {
    if (at(Tok::Close)) {
      ++i_;
      return;
    }
    if (!end()) syntax();
  }
  void enter() {  // the shared nesting budget (calc_engine.h kMaxNesting): one more = Stack ERROR
    if (++depth_ > kMaxNesting) throw BaseFail{CalcError::Stack, static_cast<int>(i_)};
  }

  // or xor xnor (loosest), then and, + −, × ÷, implicit ×.
  int64_t orLevel() {
    int64_t l = andLevel();
    while (at(Tok::Or) || at(Tok::Xor) || at(Tok::Xnor)) {
      const Tok op = t_[i_++];
      const uint32_t a = static_cast<uint32_t>(l), b = static_cast<uint32_t>(andLevel());
      const uint32_t r = op == Tok::Or ? (a | b) : op == Tok::Xor ? (a ^ b) : ~(a ^ b);
      l = static_cast<int32_t>(r);
    }
    return l;
  }
  int64_t andLevel() {
    int64_t l = addLevel();
    while (at(Tok::And)) {
      ++i_;
      l = static_cast<int32_t>(static_cast<uint32_t>(l) & static_cast<uint32_t>(addLevel()));
    }
    return l;
  }
  int64_t addLevel() {
    int64_t l = mulLevel();
    while (at(Tok::Add) || at(Tok::Sub)) {
      const Tok op = t_[i_++];
      const int64_t r = mulLevel();
      l = check(op == Tok::Add ? l + r : l - r);
    }
    return l;
  }
  int64_t mulLevel() {
    int64_t l = implicitMul();
    while (at(Tok::Mul) || at(Tok::Div)) {
      const Tok op = t_[i_++];
      const int64_t r = implicitMul();
      if (op == Tok::Mul) {
        l = check(l * r);  // |l|, |r| <= 2^31: the product fits 64 bits
      } else {
        if (r == 0) math();
        l = check(l / r);  // cut toward 0, as the Casio cuts the fraction off
      }
    }
    return l;
  }
  int64_t implicitMul() {
    int64_t l = sign();
    while (!end() && startsOperandTok(t_[i_])) l = check(l * sign());
    return l;
  }
  // − / + where a value belongs (and the (−) token, should one be there): its sign.
  int64_t sign() {
    bool minus = false;
    while (at(Tok::Sub) || at(Tok::Add) || at(Tok::Neg)) minus ^= t_[i_++] != Tok::Add;
    const int64_t v = primary();
    return minus ? check(-v) : v;
  }

  int64_t primary() {
    if (end()) syntax();
    const Tok k = t_[i_];
    if (digitValue(k) >= 0) return number(base_);
    if (k >= Tok::PreD && k <= Tok::PreO) {
      ++i_;
      static const Base kPre[] = {Base::Dec, Base::Hex, Base::Bin, Base::Oct};
      if (end() || digitValue(t_[i_]) < 0) syntax();  // a prefix goes right before a number
      return number(kPre[static_cast<int>(k) - static_cast<int>(Tok::PreD)]);
    }
    if (k == Tok::Open || k == Tok::Not || k == Tok::BNeg) {
      ++i_;
      enter();
      const int64_t v = orLevel();
      close();
      --depth_;
      if (k == Tok::Not) return static_cast<int32_t>(~static_cast<uint32_t>(v));
      if (k == Tok::BNeg) return check(-v);
      return v;
    }
    if (isValueTok(k)) {
      ++i_;
      // A memory or Ans: its integer part (a value from COMP may have a fraction).
      const double d = const_cast<Vars&>(vars_).get(k).v;
      if (!std::isfinite(d)) math();
      const double c = std::trunc(d);
      if (c < static_cast<double>(kMin32) || c > static_cast<double>(kMax32)) math();
      return static_cast<int64_t>(c);
    }
    syntax();  // a function, π, ".", ×10ˣ, i ...: not in BASE-N
  }

  // Digits (0-9, A-F) in base b: their bits, as the Casio reads them (BIN 16, HEX / OCT 32 bits).
  int64_t number(Base b) {
    const int radix = radixOf(b);
    const uint64_t limit = b == Base::Bin ? 0xFFFFu : b == Base::Dec ? static_cast<uint64_t>(kMax32) : 0xFFFFFFFFu;
    uint64_t v = 0;
    bool over = false;
    while (!end() && digitValue(t_[i_]) >= 0) {
      const int d = digitValue(t_[i_]);
      if (d >= radix) syntax();  // 2 in BIN, A without HEX
      ++i_;
      v = v * static_cast<uint64_t>(radix) + static_cast<uint64_t>(d);
      if (v > limit) over = true, v = limit;  // keep reading the digits (a later bad one is Syntax)
    }
    if (over) math();
    if (b == Base::Bin && v >= 0x8000u) return static_cast<int64_t>(v) - 0x10000;
    if ((b == Base::Hex || b == Base::Oct) && v >= 0x80000000u) return static_cast<int64_t>(v) - 0x100000000LL;
    return static_cast<int64_t>(v);
  }
};

}  // namespace

BaseResult evaluateBaseN(const std::vector<Tok>& toks, const Vars& vars, Base base) {
  const EvalRun run;  // the shared run: nothing here repeats, but the scope is the same as COMP's
  BaseResult r;
  try {
    BaseParser p(toks, vars, base);
    const int64_t v = p.run();
    if (!baseFits(v, base)) throw BaseFail{CalcError::Math, static_cast<int>(toks.size())};
    r.value = static_cast<int32_t>(v);
  } catch (const BaseFail& f) {
    r.error = f.e;
    r.errorPos = f.pos;
  }
  return r;
}

bool baseFits(int64_t v, Base base) {
  if (base == Base::Bin) return v >= -32768 && v <= 32767;
  return v >= kMin32 && v <= kMax32;
}

std::string baseText(int32_t v, Base base) {
  if (base == Base::Dec) {
    const int64_t a = v < 0 ? -static_cast<int64_t>(v) : v;
    return (v < 0 ? "\xE2\x88\x92" : "") + std::to_string(a);  // − as COMP shows it
  }
  // The two's complement bits, zero-padded to the base's width, as the Casio shows them.
  const int bits = base == Base::Hex ? 4 : base == Base::Oct ? 3 : 1;
  const int digits = base == Base::Hex ? 8 : base == Base::Oct ? 11 : 16;
  uint32_t u = static_cast<uint32_t>(v);
  if (base == Base::Bin) u &= 0xFFFFu;
  std::string s(static_cast<size_t>(digits), '0');
  for (int i = digits - 1; i >= 0; --i, u >>= bits) s[static_cast<size_t>(i)] = "0123456789ABCDEF"[u & ((1u << bits) - 1)];
  return s;
}

const char* baseName(Base base) {
  switch (base) {
    case Base::Hex: return "Hex";
    case Base::Bin: return "Bin";
    case Base::Oct: return "Oct";
    default: return "Dec";
  }
}

}  // namespace calc
