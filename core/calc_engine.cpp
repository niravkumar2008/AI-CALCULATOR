#include "calc_engine.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <climits>

namespace calc {

namespace {
constexpr double kPi = 3.14159265358979323846;  // not every compiler defines M_PI
constexpr double kE = 2.71828182845904523536;
}  // namespace

// ---------------------------------------------------------------- tokens

namespace {
struct TokInfo {
  const char* text;
  bool opens;  // needs a closing bracket
};

// Order must match enum Tok.
const TokInfo kTok[] = {
    {"0", false}, {"1", false}, {"2", false}, {"3", false}, {"4", false},
    {"5", false}, {"6", false}, {"7", false}, {"8", false}, {"9", false},
    {".", false}, {"E", false},
    {"+", false}, {"\xE2\x88\x92", false}, {"\xC3\x97", false}, {"\xC3\xB7", false},  // + − × ÷
    {"-", false},                        // (−) negative sign
    {"\xE2\x8C\x9F", false},             // ⌟ fraction bar
    {"^(", true}, {"\xE2\x81\xBF\xE2\x88\x9A(", true},  // ⁿ√(
    {"P", false}, {"C", false}, {",", false},
    {"(", true}, {")", false},
    {"\xC2\xB2", false}, {"\xC2\xB3", false}, {"\xE2\x81\xBB\xC2\xB9", false},  // ² ³ ⁻¹
    {"!", false}, {"%", false},
    {"sin(", true}, {"cos(", true}, {"tan(", true},
    {"sin\xE2\x81\xBB\xC2\xB9(", true}, {"cos\xE2\x81\xBB\xC2\xB9(", true}, {"tan\xE2\x81\xBB\xC2\xB9(", true},
    {"sinh(", true}, {"cosh(", true}, {"tanh(", true},
    {"sinh\xE2\x81\xBB\xC2\xB9(", true}, {"cosh\xE2\x81\xBB\xC2\xB9(", true}, {"tanh\xE2\x81\xBB\xC2\xB9(", true},
    {"log(", true}, {"ln(", true}, {"10^(", true}, {"e^(", true},
    {"\xE2\x88\x9A(", true}, {"\xC2\xB3\xE2\x88\x9A(", true},  // √( ³√(
    {"Abs(", true}, {"Int(", true}, {"Intg(", true}, {"Rnd(", true},
    {"GCD(", true}, {"LCM(", true}, {"RanInt#(", true},
    {"\xCF\x80", false}, {"e", false}, {"Ans", false}, {"PreAns", false}, {"Ran#", false},
    {"A", false}, {"B", false}, {"C", false}, {"D", false}, {"E", false}, {"F", false},
    {"X", false}, {"Y", false}, {"M", false},
};
static_assert(sizeof(kTok) / sizeof(kTok[0]) == static_cast<size_t>(Tok::Count), "token table");

bool isDigit(Tok t) { return t <= Tok::D9; }
bool isFunc(Tok t) { return t >= Tok::Sin && t <= Tok::RanInt; }
bool isValue(Tok t) { return t >= Tok::Pi && t <= Tok::VarM; }
bool startsOperand(Tok t) {
  return isDigit(t) || t == Tok::Dot || t == Tok::Exp10 || t == Tok::Open || isFunc(t) || isValue(t);
}
}  // namespace

const char* tokText(Tok t) { return kTok[static_cast<int>(t)].text; }
bool tokOpensBracket(Tok t) { return kTok[static_cast<int>(t)].opens; }

std::string exprText(const std::vector<Tok>& toks) {
  std::string s;
  for (Tok t : toks) s += tokText(t);
  return s;
}

// ---------------------------------------------------------------- typed text

namespace {
struct Alias {
  const char* text;
  Tok tok;
};
// Checked in order, so longer spellings come first where they overlap.
const Alias kAliases[] = {
    {"arcsinh(", Tok::Asinh}, {"arccosh(", Tok::Acosh}, {"arctanh(", Tok::Atanh},
    {"asinh(", Tok::Asinh}, {"acosh(", Tok::Acosh}, {"atanh(", Tok::Atanh},
    {"sinh\xE2\x81\xBB\xC2\xB9(", Tok::Asinh}, {"cosh\xE2\x81\xBB\xC2\xB9(", Tok::Acosh},
    {"tanh\xE2\x81\xBB\xC2\xB9(", Tok::Atanh},
    {"sinh(", Tok::Sinh}, {"cosh(", Tok::Cosh}, {"tanh(", Tok::Tanh},
    {"arcsin(", Tok::Asin}, {"arccos(", Tok::Acos}, {"arctan(", Tok::Atan},
    {"asin(", Tok::Asin}, {"acos(", Tok::Acos}, {"atan(", Tok::Atan},
    {"sin\xE2\x81\xBB\xC2\xB9(", Tok::Asin}, {"cos\xE2\x81\xBB\xC2\xB9(", Tok::Acos},
    {"tan\xE2\x81\xBB\xC2\xB9(", Tok::Atan},
    {"sin(", Tok::Sin}, {"cos(", Tok::Cos}, {"tan(", Tok::Tan},
    {"log10(", Tok::Log}, {"log(", Tok::Log}, {"ln(", Tok::Ln},
    {"10^(", Tok::Pow10}, {"exp(", Tok::Exp}, {"e^(", Tok::Exp},
    {"sqrt(", Tok::Sqrt}, {"\xE2\x88\x9A(", Tok::Sqrt},
    {"cbrt(", Tok::Cbrt}, {"\xC2\xB3\xE2\x88\x9A(", Tok::Cbrt}, {"\xE2\x88\x9B(", Tok::Cbrt},
    {"abs(", Tok::Abs}, {"int(", Tok::Int}, {"intg(", Tok::Intg}, {"rnd(", Tok::Rnd},
    {"gcd(", Tok::Gcd}, {"lcm(", Tok::Lcm},
    {"ans", Tok::Ans}, {"pi", Tok::Pi}, {"\xCF\x80", Tok::Pi},
    {"\xE2\x81\xBB\xC2\xB9", Tok::Inv}, {"\xC2\xB2", Tok::Sq}, {"\xC2\xB3", Tok::Cube},
    {"*", Tok::Mul}, {"\xC3\x97", Tok::Mul}, {"\xC2\xB7", Tok::Mul},
    {"/", Tok::Div}, {"\xC3\xB7", Tok::Div}, {"\xE2\x8C\x9F", Tok::Frac},
    {"+", Tok::Add}, {"!", Tok::Fact}, {"%", Tok::Pct}, {",", Tok::Comma},
    {"(", Tok::Open}, {")", Tok::Close}, {".", Tok::Dot},
};

bool matchAt(const std::string& s, size_t i, const char* word, bool fold) {
  for (size_t k = 0; word[k]; ++k) {
    if (i + k >= s.size()) return false;
    char c = s[i + k];
    if (fold && c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    if (c != word[k]) return false;
  }
  return true;
}

// Does the next token start an operand (so "-" before it is a minus sign)?
bool endsOperand(Tok t) {
  return isDigit(t) || t == Tok::Dot || t == Tok::Close || isValue(t) || t == Tok::Sq ||
         t == Tok::Cube || t == Tok::Inv || t == Tok::Fact || t == Tok::Pct;
}
}  // namespace

bool parseExpression(const std::string& s, std::vector<Tok>& out) {
  out.clear();
  int closeAfterOperand = 0;  // "2^3": the Casio's ^( needs a ")" after the 3
  auto finishOperand = [&]() {
    while (closeAfterOperand > 0) {
      out.push_back(Tok::Close);
      --closeAfterOperand;
    }
  };
  size_t i = 0;
  while (i < s.size()) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (c == ' ') {
      ++i;
      continue;
    }
    if (c >= '0' && c <= '9') {
      out.push_back(static_cast<Tok>(c - '0'));
      ++i;
      bool more = i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == 'E');
      if (!more) finishOperand();
      continue;
    }
    if (c == 'E' && !out.empty() && (isDigit(out.back()) || out.back() == Tok::Dot)) {
      out.push_back(Tok::Exp10);
      ++i;
      if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
        if (s[i] == '-') out.push_back(Tok::Neg);
        ++i;
      }
      continue;
    }
    // Minus: a sign at the start or after an operator, subtraction otherwise.
    const bool minusAscii = c == '-';
    const bool minusUni = matchAt(s, i, "\xE2\x88\x92", false) || matchAt(s, i, "\xE2\x80\x93", false);
    if (minusAscii || minusUni) {
      out.push_back(!out.empty() && endsOperand(out.back()) ? Tok::Sub : Tok::Neg);
      i += minusAscii ? 1 : 3;
      continue;
    }
    if (c == '^') {
      out.push_back(Tok::Pow);
      ++i;
      while (i < s.size() && s[i] == ' ') ++i;
      if (i < s.size() && s[i] == '(') ++i;  // "^(" is one key
      else ++closeAfterOperand;
      continue;
    }
    if (matchAt(s, i, "\xE2\x88\x9A", false) && !matchAt(s, i, "\xE2\x88\x9A(", false)) {  // "√3"
      out.push_back(Tok::Sqrt);
      ++closeAfterOperand;
      i += 3;
      continue;
    }
    if (c == 'e' && !matchAt(s, i, "e^(", false) && !matchAt(s, i, "exp(", false)) {
      out.push_back(Tok::E);
      ++i;
      finishOperand();
      continue;
    }
    bool found = false;
    for (const Alias& a : kAliases) {
      if (!matchAt(s, i, a.text, true)) continue;
      i += std::char_traits<char>::length(a.text);
      out.push_back(a.tok);
      if (isValue(a.tok) || a.tok == Tok::Close) finishOperand();
      found = true;
      break;
    }
    if (!found) {
      out.clear();
      return false;
    }
    if (static_cast<int>(out.size()) > kMaxTokens) {
      out.clear();
      return false;
    }
  }
  if (static_cast<int>(out.size()) > kMaxTokens) out.clear();
  return !out.empty();
}

// ---------------------------------------------------------------- numbers

namespace {
constexpr long long kMaxExact = 1000000000000000LL;  // keep num/den below 1e15

long long gcdLL(long long a, long long b) {
  a = std::llabs(a);
  b = std::llabs(b);
  while (b) {
    long long t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// Exact n/d in lowest terms, or an inexact value if it gets too big. Plain
// 64-bit maths with overflow checks: the ESP32 has no 128-bit integers.
Num makeExact(long long n, long long d) {
  Num r;
  r.v = static_cast<double>(n) / static_cast<double>(d);
  r.exact = false;
  if (d == 0 || n == LLONG_MIN || d == LLONG_MIN) return r;
  if (d < 0) {
    n = -n;
    d = -d;
  }
  const long long g = gcdLL(n, d);
  if (g > 1) {
    n /= g;
    d /= g;
  }
  if (n > kMaxExact || n < -kMaxExact || d > kMaxExact) return r;
  r.exact = true;
  r.num = n;
  r.den = d;
  return r;
}

bool mulOk(long long a, long long b, long long& out) { return !__builtin_mul_overflow(a, b, &out); }
bool addOk(long long a, long long b, long long& out) { return !__builtin_add_overflow(a, b, &out); }

Num exactPlus(const Num& a, const Num& b) {
  long long x, y, n, d;
  if (a.exact && b.exact && mulOk(a.num, b.den, x) && mulOk(b.num, a.den, y) && addOk(x, y, n) &&
      mulOk(a.den, b.den, d))
    return makeExact(n, d);
  return Num::ofDouble(a.v + b.v);
}

Num exactTimes(const Num& a, const Num& b) {
  if (a.exact && b.exact) {
    // Cancel across first so fewer products overflow: (a/b)(c/d) = (a/g1)(c/g2) / ((b/g2)(d/g1)).
    const long long g1 = gcdLL(a.num, b.den), g2 = gcdLL(b.num, a.den);
    const long long an = g1 ? a.num / g1 : a.num, bd = g1 ? b.den / g1 : b.den;
    const long long bn = g2 ? b.num / g2 : b.num, ad = g2 ? a.den / g2 : a.den;
    long long n, d;
    if (mulOk(an, bn, n) && mulOk(ad, bd, d)) return makeExact(n, d);
  }
  return Num::ofDouble(a.v * b.v);
}

bool isInt(const Num& n) {
  if (n.exact) return n.den == 1;
  return std::fabs(n.v) < 1e15 && n.v == std::floor(n.v);
}
long long asInt(const Num& n) { return n.exact ? n.num : static_cast<long long>(std::llround(n.v)); }
}  // namespace

Num Num::ofInt(long long n) { return makeExact(n, 1); }
Num Num::ofDouble(double d) {
  Num r;
  r.v = d;
  r.exact = false;
  return r;
}

Num& Vars::get(Tok t) {
  switch (t) {
    case Tok::VarA: return a;
    case Tok::VarB: return b;
    case Tok::VarC: return c;
    case Tok::VarD: return d;
    case Tok::VarE: return e;
    case Tok::VarF: return f;
    case Tok::VarX: return x;
    case Tok::VarY: return y;
    case Tok::VarM: return m;
    case Tok::PreAns: return preAns;
    default: return ans;
  }
}

// ---------------------------------------------------------------- evaluation

namespace {

struct Fail {
  CalcError e;
  int pos;
};

class Parser {
 public:
  Parser(const std::vector<Tok>& t, Vars& v, AngleUnit u, double (*rnd)()) : t_(t), vars_(v), unit_(u), rnd_(rnd) {}

  Num run() {
    if (t_.empty()) throw Fail{CalcError::Syntax, 0};
    Num r = addSub();
    if (i_ < t_.size()) throw Fail{CalcError::Syntax, static_cast<int>(i_)};
    return check(r);
  }

 private:
  const std::vector<Tok>& t_;
  Vars& vars_;
  AngleUnit unit_;
  double (*rnd_)();
  size_t i_ = 0;
  int depth_ = 0;

  bool at(Tok k) const { return i_ < t_.size() && t_[i_] == k; }
  bool end() const { return i_ >= t_.size(); }
  [[noreturn]] void syntax() const { throw Fail{CalcError::Syntax, static_cast<int>(i_ < t_.size() ? i_ : t_.size())}; }
  [[noreturn]] void math() const { throw Fail{CalcError::Math, static_cast<int>(i_ > 0 ? i_ - 1 : 0)}; }

  Num check(Num n) const {
    if (std::isnan(n.v) || std::isinf(n.v) || std::fabs(n.v) >= 1e100) math();
    if (!n.exact && std::fabs(n.v) < 1e-99) n.v = 0;
    return n;
  }

  // Closing bracket: optional at the very end (the Casio auto-closes).
  void close() {
    if (at(Tok::Close)) {
      ++i_;
      return;
    }
    if (!end()) syntax();
  }

  Num addSub() {
    Num l = mulDiv();
    while (at(Tok::Add) || at(Tok::Sub)) {
      bool add = t_[i_++] == Tok::Add;
      Num r = mulDiv();
      l = check(add ? plus(l, r) : minus(l, r));
    }
    return l;
  }

  Num mulDiv() {
    Num l = perm();
    while (at(Tok::Mul) || at(Tok::Div)) {
      bool mul = t_[i_++] == Tok::Mul;
      Num r = perm();
      l = check(mul ? times(l, r) : divide(l, r));
    }
    return l;
  }

  Num perm() {
    Num l = implicitMul();
    while (at(Tok::NPr) || at(Tok::NCr)) {
      bool p = t_[i_++] == Tok::NPr;
      Num r = implicitMul();
      l = check(nPr(l, r, p));
    }
    return l;
  }

  // "2π", "3sin(30)", "(1+2)(3+4)": multiplication without a sign binds
  // tighter than × and ÷ (so 1÷2π = 1÷(2π), as on the Casio).
  Num implicitMul() {
    Num l = neg();
    while (!end() && startsOperand(t_[i_])) l = check(times(l, neg()));
    return l;
  }

  Num neg() {
    if (at(Tok::Neg)) {
      ++i_;
      Num r = neg();
      return negate(r);
    }
    return frac();
  }

  Num frac() {
    Num l = power();
    while (at(Tok::Frac)) {
      ++i_;
      Num r = power();
      l = check(divide(l, r));
    }
    return l;
  }

  Num power() {
    Num l = postfix();
    while (at(Tok::Pow) || at(Tok::XRoot)) {
      bool p = t_[i_++] == Tok::Pow;
      if (++depth_ > 24) throw Fail{CalcError::Stack, static_cast<int>(i_)};
      Num r = addSub();
      close();
      --depth_;
      l = check(p ? pow(l, r) : root(l, r));
    }
    return l;
  }

  Num postfix() {
    Num v = primary();
    while (!end()) {
      Tok k = t_[i_];
      if (k == Tok::Sq) v = times(v, v);
      else if (k == Tok::Cube) v = times(times(v, v), v);
      else if (k == Tok::Inv) v = divide(Num::ofInt(1), v);
      else if (k == Tok::Fact) v = factorial(v);
      else if (k == Tok::Pct) v = divide(v, Num::ofInt(100));
      else break;
      ++i_;
      v = check(v);
    }
    return v;
  }

  Num number() {
    // digits [. digits] [E [-] digits]; "E3" alone means 1E3
    long long mant = 0;
    int scale = 0, digits = 0;
    bool dot = false, any = false;
    while (!end() && (isDigit(t_[i_]) || t_[i_] == Tok::Dot)) {
      if (t_[i_] == Tok::Dot) {
        if (dot) syntax();
        dot = true;
      } else {
        any = true;
        if (digits < 17) {
          mant = mant * 10 + static_cast<int>(t_[i_]);
          if (mant != 0) ++digits;
          if (dot) ++scale;
        } else if (!dot) {
          --scale;  // too many digits: keep magnitude, drop precision
        }
      }
      ++i_;
    }
    if (dot && !any) syntax();
    int ex = 0;
    if (at(Tok::Exp10)) {
      ++i_;
      if (!any) mant = 1;
      bool negEx = false;
      if (at(Tok::Neg) || at(Tok::Sub)) {
        negEx = true;
        ++i_;
      }
      int n = 0;
      while (!end() && isDigit(t_[i_])) {
        if (++n > 2) syntax();
        ex = ex * 10 + static_cast<int>(t_[i_]);
        ++i_;
      }
      if (n == 0) syntax();
      if (negEx) ex = -ex;
    }
    int e10 = ex - scale;
    if (e10 >= 0 && e10 <= 18) {
      long long m = mant;
      bool ok = true;
      for (int k = 0; k < e10 && ok; ++k) ok = mulOk(m, 10, m);
      if (ok) return check(makeExact(m, 1));
    }
    if (e10 < 0 && e10 >= -18) {
      long long d = 1;
      for (int k = 0; k < -e10; ++k) d *= 10;  // at most 10^18: fits
      return check(makeExact(mant, d));
    }
    return check(Num::ofDouble(static_cast<double>(mant) * std::pow(10.0, e10)));
  }

  Num primary() {
    if (end()) syntax();
    Tok k = t_[i_];
    if (isDigit(k) || k == Tok::Dot || k == Tok::Exp10) return number();
    if (k == Tok::Open) {
      ++i_;
      if (++depth_ > 24) throw Fail{CalcError::Stack, static_cast<int>(i_)};
      Num r = addSub();
      close();
      --depth_;
      return r;
    }
    if (isValue(k)) {
      ++i_;
      switch (k) {
        case Tok::Pi: return Num::ofDouble(kPi);
        case Tok::E: return Num::ofDouble(kE);
        case Tok::Ran: {
          double r = std::floor(rnd_() * 1000) / 1000;  // 3 decimal places, like Ran#
          return makeExact(static_cast<long long>(r * 1000), 1000);
        }
        default: return vars_.get(k);
      }
    }
    if (isFunc(k)) return function();
    syntax();
  }

  double toRad(double a) const {
    if (unit_ == AngleUnit::Deg) return a * kPi / 180;
    if (unit_ == AngleUnit::Gra) return a * kPi / 200;
    return a;
  }
  double fromRad(double a) const {
    if (unit_ == AngleUnit::Deg) return a * 180 / kPi;
    if (unit_ == AngleUnit::Gra) return a * 200 / kPi;
    return a;
  }
  // Removes the last-bit noise so sin(180°) is 0 and cos(60°) is exactly 0.5.
  static double clean(double v) {
    if (std::fabs(v) < 1e-14) return 0;
    double r = std::round(v * 1e13) / 1e13;
    return std::fabs(r - v) < 1e-15 ? r : v;
  }

  Num function() {
    Tok k = t_[i_++];
    if (++depth_ > 24) throw Fail{CalcError::Stack, static_cast<int>(i_)};
    Num a = addSub();
    Num b;
    bool two = false;
    if (at(Tok::Comma)) {
      if (k != Tok::Log && k != Tok::Gcd && k != Tok::Lcm && k != Tok::RanInt) syntax();
      ++i_;
      b = addSub();
      two = true;
    } else if (k == Tok::Gcd || k == Tok::Lcm || k == Tok::RanInt) {
      syntax();
    }
    close();
    --depth_;
    const double x = a.v;
    switch (k) {
      case Tok::Sin: return check(Num::ofDouble(clean(std::sin(reduce(x)))));
      case Tok::Cos: return check(Num::ofDouble(clean(std::cos(reduce(x)))));
      case Tok::Tan: {
        if (unit_ != AngleUnit::Rad) {
          double q = unit_ == AngleUnit::Deg ? 90 : 100;
          double m = std::fmod(std::fabs(x), 2 * q);
          if (std::fabs(m - q) < 1e-9) math();
        }
        return check(Num::ofDouble(clean(std::tan(reduce(x)))));
      }
      case Tok::Asin:
        if (x < -1 || x > 1) math();
        return check(Num::ofDouble(clean(fromRad(std::asin(x)))));
      case Tok::Acos:
        if (x < -1 || x > 1) math();
        return check(Num::ofDouble(clean(fromRad(std::acos(x)))));
      case Tok::Atan: return check(Num::ofDouble(clean(fromRad(std::atan(x)))));
      case Tok::Sinh: return check(Num::ofDouble(std::sinh(x)));
      case Tok::Cosh: return check(Num::ofDouble(std::cosh(x)));
      case Tok::Tanh: return check(Num::ofDouble(std::tanh(x)));
      case Tok::Asinh: return check(Num::ofDouble(std::asinh(x)));
      case Tok::Acosh:
        if (x < 1) math();
        return check(Num::ofDouble(std::acosh(x)));
      case Tok::Atanh:
        if (x <= -1 || x >= 1) math();
        return check(Num::ofDouble(std::atanh(x)));
      case Tok::Log:
        if (two) {  // log(base, value)
          if (x <= 0 || x == 1 || b.v <= 0) math();
          return check(Num::ofDouble(clean(std::log(b.v) / std::log(x))));
        }
        if (x <= 0) math();
        return check(exactLog10(a));
      case Tok::Ln:
        if (x <= 0) math();
        return check(Num::ofDouble(std::log(x)));
      case Tok::Pow10: return check(pow(Num::ofInt(10), a));
      case Tok::Exp: return check(Num::ofDouble(std::exp(x)));
      case Tok::Sqrt: return check(root(Num::ofInt(2), a));
      case Tok::Cbrt: return check(root(Num::ofInt(3), a));
      case Tok::Abs: return a.exact ? makeExact(std::llabs(a.num), a.den) : Num::ofDouble(std::fabs(x));
      case Tok::Int: return a.exact ? makeExact(a.num / a.den, 1) : Num::ofDouble(std::trunc(x));
      case Tok::Intg: return a.exact ? makeExact(floorDiv(a.num, a.den), 1) : Num::ofDouble(std::floor(x));
      case Tok::Rnd: {
        if (a.exact && a.den == 1) return a;
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.9e", x);  // 10 significant digits, as displayed
        return check(Num::ofDouble(std::atof(buf)));
      }
      case Tok::Gcd:
      case Tok::Lcm: {
        if (!isInt(a) || !isInt(b)) math();
        long long p = asInt(a), q = asInt(b);
        long long g = gcdLL(p, q);
        if (k == Tok::Gcd) return Num::ofInt(g);
        if (g == 0) return Num::ofInt(0);
        long long l;
        if (!mulOk(std::llabs(p / g), std::llabs(q), l)) math();
        return check(makeExact(l, 1));
      }
      case Tok::RanInt: {
        if (!isInt(a) || !isInt(b) || asInt(a) > asInt(b)) math();
        long long lo = asInt(a), hi = asInt(b);
        return Num::ofInt(lo + static_cast<long long>(rnd_() * static_cast<double>(hi - lo + 1)));
      }
      default: syntax();
    }
  }

  double reduce(double x) const {
    // Reduce in the angle unit first so sin(3600) in degrees stays exact.
    if (unit_ == AngleUnit::Deg) return toRad(std::fmod(x, 360.0));
    if (unit_ == AngleUnit::Gra) return toRad(std::fmod(x, 400.0));
    return x;
  }

  static long long floorDiv(long long n, long long d) {
    long long q = n / d;
    if ((n % d != 0) && ((n < 0) != (d < 0))) --q;
    return q;
  }

  Num exactLog10(const Num& a) const {
    if (a.exact && a.den == 1) {  // log(1000) = 3 exactly
      long long n = a.num;
      int e = 0;
      while (n % 10 == 0 && n > 1) {
        n /= 10;
        ++e;
      }
      if (n == 1) return Num::ofInt(e);
    }
    return Num::ofDouble(clean(std::log10(a.v)));
  }

  static Num plus(const Num& a, const Num& b) {
    return exactPlus(a, b);
  }
  static Num minus(const Num& a, const Num& b) { return plus(a, negate(b)); }
  static Num negate(const Num& a) {
    Num r = a;
    r.v = -a.v;
    r.num = -a.num;
    return r;
  }
  static Num times(const Num& a, const Num& b) {
    return exactTimes(a, b);
  }
  Num divide(const Num& a, const Num& b) const {
    if (b.v == 0) math();
    if (a.exact && b.exact) {
      Num inv;  // b flipped over
      inv = makeExact(b.den, b.num);
      if (inv.exact) return exactTimes(a, inv);
    }
    return Num::ofDouble(a.v / b.v);
  }

  Num pow(const Num& base, const Num& ex) const {
    if (ex.exact && ex.den == 1 && base.exact && ex.num >= -64 && ex.num <= 64) {
      if (base.v == 0 && ex.num < 0) math();
      Num r = Num::ofInt(1);
      for (long long k = 0; k < std::llabs(ex.num) && r.exact; ++k) r = times(r, base);
      if (r.exact) return ex.num < 0 ? divide(Num::ofInt(1), r) : r;
    }
    if (base.v < 0) {
      // (−8)^(1/3): a real root exists when the exponent's denominator is odd.
      if (ex.exact && ex.den % 2 == 1) {
        double m = std::pow(-base.v, ex.v);
        return Num::ofDouble(ex.num % 2 ? -m : m);
      }
      if (ex.v != std::floor(ex.v)) math();
    }
    if (base.v == 0 && ex.v < 0) math();
    return Num::ofDouble(std::pow(base.v, ex.v));
  }

  Num root(const Num& index, const Num& x) const {
    if (index.v == 0) math();
    if (x.exact && index.exact && index.den == 1 && index.num >= 2 && index.num <= 3) {
      long long n = index.num;
      auto exactRoot = [&](long long v, long long& out) {
        long long s = static_cast<long long>(std::llround(n == 2 ? std::sqrt(std::fabs(static_cast<double>(v)))
                                                                  : std::cbrt(std::fabs(static_cast<double>(v)))));
        for (long long c = s - 1; c <= s + 1; ++c)
          if (c >= 0 && (n == 2 ? c * c : c * c * c) == std::llabs(v)) {
            out = v < 0 ? -c : c;
            return true;
          }
        return false;
      };
      long long rn, rd;
      if ((x.num >= 0 || n == 3) && exactRoot(x.num, rn) && exactRoot(x.den, rd)) return makeExact(rn, rd);
    }
    if (x.v < 0) {
      bool oddInt = index.exact && index.den == 1 && (index.num % 2 != 0);
      if (!oddInt) math();
      return Num::ofDouble(-std::pow(-x.v, 1.0 / index.v));
    }
    return Num::ofDouble(std::pow(x.v, 1.0 / index.v));
  }

  Num factorial(const Num& a) const {
    if (!isInt(a) || a.v < 0 || a.v > 69) math();
    Num r = Num::ofInt(1);
    for (long long k = 2; k <= asInt(a); ++k) r = times(r, Num::ofInt(k));
    return r;
  }

  Num nPr(const Num& n, const Num& r, bool perm) const {
    if (!isInt(n) || !isInt(r)) math();
    long long N = asInt(n), R = asInt(r);
    if (N < 0 || R < 0 || R > N || N >= 10000000000LL) math();
    Num out = Num::ofInt(1);
    if (!perm && R > N - R) R = N - R;  // C(n, r) = C(n, n-r)
    for (long long k = 0; k < R; ++k) {
      out = times(out, Num::ofInt(N - k));
      if (!perm) out = divide(out, Num::ofInt(k + 1));
      if (std::fabs(out.v) >= 1e100) math();
    }
    return out;
  }
};

}  // namespace

EvalResult evaluate(const std::vector<Tok>& toks, Vars& vars, AngleUnit unit, double (*random)()) {
  EvalResult r;
  try {
    Parser p(toks, vars, unit, random);
    r.value = p.run();
  } catch (const Fail& f) {
    r.error = f.e;
    r.errorPos = f.pos;
  }
  return r;
}

// ---------------------------------------------------------------- formatting

namespace {
const char* kSup[] = {"\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3", "\xE2\x81\xB4",
                      "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7", "\xE2\x81\xB8", "\xE2\x81\xB9"};
const char* kMinus = "\xE2\x88\x92";  // −

std::string trimZeros(std::string s) {
  if (s.find('.') == std::string::npos) return s;
  while (!s.empty() && s.back() == '0') s.pop_back();
  if (!s.empty() && s.back() == '.') s.pop_back();
  return s;
}
}  // namespace

std::string formatDecimal(double v, NormMode norm) {
  if (v == 0) return "0";
  char buf[48];
  std::snprintf(buf, sizeof buf, "%.9e", v);  // d.ddddddddde±xx: 10 significant digits
  std::string s = buf;
  size_t ePos = s.find('e');
  int ex = std::atoi(s.c_str() + ePos + 1);
  std::string mant = s.substr(0, ePos);
  bool negative = mant[0] == '-';
  if (negative) mant.erase(0, 1);
  const double lo = norm == NormMode::Norm1 ? 1e-2 : 1e-9;
  const double a = std::fabs(std::atof(buf));
  std::string out;
  if (a >= 1e10 || a < lo) {
    out = trimZeros(mant) + "\xC3\x97" "10";  // ×10 then a superscript exponent
    if (ex < 0) out += "\xE2\x81\xBB";          // ⁻
    for (char c : std::to_string(std::abs(ex))) out += kSup[c - '0'];
  } else {
    std::string digits;
    for (char c : mant)
      if (c != '.') digits += c;  // 10 digits
    if (ex >= 0) {
      out = digits.substr(0, ex + 1) + "." + digits.substr(ex + 1);
    } else {
      out = "0." + std::string(-ex - 1, '0') + digits;
    }
    out = trimZeros(out);
  }
  return (negative ? std::string(kMinus) : std::string()) + out;
}

bool fractionText(const Num& n, std::string& out) {
  long long num = n.num, den = n.den;
  if (!n.exact) {
    // Values like sin(30°) = 0.5 or 2÷3 computed inexactly: show as a
    // fraction when a small denominator matches to 12 digits.
    if (!(std::fabs(n.v) < 1e9)) return false;
    bool found = false;
    for (long long d = 2; d <= 1000 && !found; ++d) {
      double p = std::round(n.v * d);
      if (std::fabs(p / d - n.v) <= 1e-12 * std::max(1.0, std::fabs(n.v))) {
        num = static_cast<long long>(p);
        den = d;
        found = true;
      }
    }
    if (!found) return false;
    long long g = gcdLL(num, den);
    num /= g;
    den /= g;
  }
  if (den == 1) return false;
  std::string a = std::to_string(std::llabs(num)), b = std::to_string(den);
  if (a.size() + b.size() > 10) return false;  // the Casio's limit: 10 digits in all
  out = (num < 0 ? std::string(kMinus) : std::string()) + a + "\xE2\x8C\x9F" + b;  // a⌟b
  return true;
}

std::string formatResult(const Num& n, bool preferFraction, NormMode norm) {
  std::string f;
  if (preferFraction && fractionText(n, f)) return f;
  return formatDecimal(n.v, norm);
}

const char* errorText(CalcError e) {
  switch (e) {
    case CalcError::Syntax: return "Syntax ERROR";
    case CalcError::Math: return "Math ERROR";
    case CalcError::Stack: return "Stack ERROR";
    case CalcError::Argument: return "Argument ERROR";
    default: return "";
  }
}

}  // namespace calc
