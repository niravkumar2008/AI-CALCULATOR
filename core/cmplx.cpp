#include "cmplx.h"

#include <cmath>
#include <cstdlib>

namespace calc {

Num& ImagVars::get(Tok t) {
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

bool ImagVars::any() const {
  const Num* all[] = {&a, &b, &c, &d, &e, &f, &x, &y, &m, &ans, &preAns};
  for (const Num* n : all)
    if (n->v != 0) return true;
  return false;
}

namespace {

constexpr double kPi = 3.14159265358979323846;  // as calc_engine.cpp
constexpr double kE = 2.71828182845904523536;
const char* const kMinus = "\xE2\x88\x92";  // −

struct CxFail {
  CalcError e;
  int pos;
};

// Same stack discipline as calc_engine.cpp's Parser (PORT_NOTES 2026-10-11): the precedence
// levels inline into one expr() frame per nesting level; whatever has big locals stays a leaf.
#if defined(__GNUC__)
#define CX_NOINLINE __attribute__((noinline))
#define CX_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define CX_NOINLINE __declspec(noinline)
#define CX_INLINE __forceinline
#else
#define CX_NOINLINE
#define CX_INLINE inline
#endif

bool isDigitTok(Tok t) { return t <= Tok::D9; }
bool isRealFunc(Tok t) { return t >= Tok::Sin && t <= Tok::RanInt; }
bool isFuncTok(Tok t) { return isRealFunc(t) || t == Tok::Arg || t == Tok::Conjg; }
bool isValueTok(Tok t) { return (t >= Tok::Pi && t <= Tok::VarM) || t == Tok::ImagI; }
bool startsOperandTok(Tok t) {
  return isDigitTok(t) || t == Tok::Dot || t == Tok::Exp10 || t == Tok::Open || isFuncTok(t) || isValueTok(t) ||
         t == Tok::Integ || t == Tok::Deriv;
}

Num negated(const Num& a) {
  Num r = a;
  r.v = -a.v;
  r.num = -a.num;
  return r;
}

// As calc_engine's Parser::clean: sin(180°) is 0, arg(1+i) is 45.
double clean(double v) {
  if (std::fabs(v) < 1e-14) return 0;
  const double r = std::round(v * 1e13) / 1e13;
  return std::fabs(r - v) < 1e-15 ? r : v;
}

double fromRad(double a, AngleUnit u) {
  if (u == AngleUnit::Deg) return a * 180 / kPi;
  if (u == AngleUnit::Gra) return a * 200 / kPi;
  return a;
}

// A double result the calculator can hold (calc_engine's check()): finite, below 1E100, and
// 0 below 1E-99.
bool holdable(double& v) {
  if (!std::isfinite(v) || std::fabs(v) >= 1e100) return false;
  if (std::fabs(v) < 1e-99) v = 0;
  return true;
}

bool tryReal(Tok op, const Num& a, const Num& b, Num& out) {
  CalcError e;
  return realOp(op, a, b, false, AngleUnit::Rad, nullptr, out, e);
}

class CxParser {
 public:
  CxParser(const std::vector<Tok>& t, size_t n, Vars& v, const ImagVars& im, AngleUnit u, double (*rnd)())
      : t_(t), n_(n), vars_(v), imag_(im), unit_(u), rnd_(rnd) {}

  Cx run() {
    if (n_ == 0) throw CxFail{CalcError::Syntax, 0};
    Cx r;
    expr(r);
    if (i_ < n_) syntax();
    return r;
  }

 private:
  const std::vector<Tok>& t_;
  size_t n_;  // the end (before a trailing ▶r∠θ / ▶a+bi)
  Vars& vars_;
  const ImagVars& imag_;
  AngleUnit unit_;
  double (*rnd_)();
  size_t i_ = 0;
  int depth_ = 0;

  bool at(Tok k) const { return i_ < n_ && t_[i_] == k; }
  bool end() const { return i_ >= n_; }
  int mathPos() const { return static_cast<int>(i_ > 0 ? i_ - 1 : 0); }
  [[noreturn]] void syntax() const { throw CxFail{CalcError::Syntax, static_cast<int>(i_ < n_ ? i_ : n_)}; }
  [[noreturn]] void math() const { throw CxFail{CalcError::Math, mathPos()}; }

  void close() {
    if (at(Tok::Close)) {
      ++i_;
      return;
    }
    if (!end()) syntax();
  }
  // The shared nesting budget (calc_engine.h kMaxNesting): one more = Stack ERROR.
  void enter() {
    if (++depth_ > kMaxNesting) throw CxFail{CalcError::Stack, static_cast<int>(i_)};
  }

  // An all-real step: the real engine's own operation (its exact fractions, its errors).
  Num real(Tok op, const Num& a, const Num& b = Num(), bool two = false) const {
    Num out;
    CalcError e = CalcError::Math;
    if (!realOp(op, a, b, two, unit_, rnd_, out, e)) {
      if (e == CalcError::Syntax) syntax();
      throw CxFail{e, mathPos()};
    }
    return out;
  }

  // ---- the precedence levels (as calc_engine.cpp's Parser; ∠ shares nPr / nCr's level)
  void expr(Cx& out) {
    Cx r;
    bool first = true;
    Tok op = Tok::Add;
    for (;;) {
      mulDiv(first ? out : r);
      if (!first) combine(op, out, r);
      first = false;
      if (!at(Tok::Add) && !at(Tok::Sub)) return;
      op = t_[i_++];
    }
  }

  CX_INLINE void mulDiv(Cx& out) {
    Cx r;
    bool first = true;
    Tok op = Tok::Mul;
    for (;;) {
      perm(first ? out : r);
      if (!first) combine(op, out, r);
      first = false;
      if (!at(Tok::Mul) && !at(Tok::Div)) return;
      op = t_[i_++];
    }
  }

  CX_INLINE void perm(Cx& out) {
    Cx r;
    bool first = true;
    Tok op = Tok::NPr;
    for (;;) {
      implicitMul(first ? out : r);
      if (!first) combine(op, out, r);
      first = false;
      if (!at(Tok::NPr) && !at(Tok::NCr) && !at(Tok::Angle)) return;
      op = t_[i_++];
    }
  }

  CX_INLINE void implicitMul(Cx& out) {
    Cx r;
    bool first = true;
    for (;;) {
      neg(first ? out : r);
      if (!first) combine(Tok::Mul, out, r);
      first = false;
      if (end() || !startsOperandTok(t_[i_])) return;
    }
  }

  CX_INLINE void neg(Cx& out) {
    bool minus = false;
    while (at(Tok::Neg)) {
      ++i_;
      minus = !minus;
    }
    frac(out);
    if (minus) combine(Tok::Neg, out, out);
  }

  CX_INLINE void frac(Cx& out) {
    Cx r;
    bool first = true;
    for (;;) {
      power(first ? out : r);
      if (!first) combine(Tok::Frac, out, r);
      first = false;
      if (!at(Tok::Frac)) return;
      ++i_;
    }
  }

  CX_INLINE void power(Cx& out) {
    operand(out);
    while (at(Tok::Pow) || at(Tok::XRoot)) {
      const Tok op = t_[i_++];
      enter();
      Cx r;
      expr(r);
      close();
      --depth_;
      combine(op, out, r);
      postfix(out);
    }
  }

  CX_NOINLINE void operand(Cx& out) {
    primary(out);
    postfix(out);
  }

  // x², x³, x⁻¹ (complex too), x! and % (real only), as many as follow.
  CX_NOINLINE void postfix(Cx& v) {
    while (!end()) {
      const Tok k = t_[i_];
      if (k == Tok::Sq) v = mul(v, v);
      else if (k == Tok::Cube) v = mul(mul(v, v), v);
      else if (k == Tok::Inv) v = div(Cx::of(Num::ofInt(1)), v);
      else if (k == Tok::Fact || k == Tok::Pct) {
        if (!v.real()) math();
        v = Cx::of(real(k, v.re));
      } else {
        break;
      }
      ++i_;
    }
  }

  // In the recursion: a small frame (numbers, values and functions are out of line).
  CX_NOINLINE void primary(Cx& out) {
    if (end()) syntax();
    const Tok k = t_[i_];
    if (isDigitTok(k) || k == Tok::Dot || k == Tok::Exp10) return number(out);
    if (k == Tok::Open) {
      ++i_;
      enter();
      expr(out);
      close();
      --depth_;
      return;
    }
    if (isValueTok(k)) return value(out);
    if (isFuncTok(k)) return function(out);
    syntax();  // ∫( d/dx( "=" ▶ in the middle, an operator where a value belongs
  }

  CX_NOINLINE void number(Cx& out) {
    size_t i = i_;
    Num v;
    CalcError e = CalcError::Syntax;
    int pos = static_cast<int>(i_);
    if (!realNumber(t_, i, v, e, pos)) throw CxFail{e, pos};
    i_ = i;
    if (i_ > n_) {  // a number can't run into the ▶ suffix (it is no digit), but be safe
      i_ = n_;
      syntax();
    }
    out = Cx::of(v);
  }

  CX_NOINLINE void value(Cx& out) {
    const Tok k = t_[i_++];
    switch (k) {
      case Tok::ImagI:
        out.re = Num::ofInt(0);
        out.im = Num::ofInt(1);
        return;
      case Tok::Pi: out = Cx::of(Num::ofDouble(kPi)); return;
      case Tok::E: out = Cx::of(Num::ofDouble(kE)); return;
      case Tok::Ran: {  // as calc_engine.cpp: 3 decimal places
        const double r = std::floor(rnd_() * 1000) / 1000;
        out = Cx::of(real(Tok::Div, Num::ofInt(static_cast<long long>(r * 1000)), Num::ofInt(1000)));
        return;
      }
      default:
        out.re = vars_.get(k);
        out.im = imag_.get(k);
        return;
    }
  }

  // A function: its first argument straight into `out` (in the recursion: a small frame; the
  // second argument of log( GCD( LCM( RanInt#( has its own frame), then its value (apply()).
  CX_NOINLINE void function(Cx& out) {
    const Tok k = t_[i_++];
    enter();
    expr(out);
    if (at(Tok::Comma)) {
      if (k != Tok::Log && k != Tok::Gcd && k != Tok::Lcm && k != Tok::RanInt) syntax();
      ++i_;
      return secondArg(k, out);
    }
    if (k == Tok::Gcd || k == Tok::Lcm || k == Tok::RanInt) syntax();
    close();
    --depth_;
    apply(k, out, out, false);
  }

  CX_NOINLINE void secondArg(Tok k, Cx& a) {
    Cx b;
    expr(b);
    close();
    --depth_;
    apply(k, a, b, true);
  }

  // a = k(a [, b]).
  CX_NOINLINE void apply(Tok k, Cx& a, const Cx& b, bool two) const {
    switch (k) {
      case Tok::Abs: {
        Num r;
        if (a.real()) r = real(Tok::Abs, a.re);
        else if (!cxAbs(a, r)) math();
        a = Cx::of(r);
        return;
      }
      case Tok::Arg: {
        Num t;
        if (!cxArg(a, unit_, t)) math();
        a = Cx::of(t);
        return;
      }
      case Tok::Conjg:
        if (!a.real()) a.im = real(Tok::Neg, a.im);
        return;
      default:  // every other function: real arguments only (Math ERROR, as on the Casio)
        if (!a.real() || (two && !b.real())) math();
        a = Cx::of(real(k, a.re, b.re, two));
        return;
    }
  }

  // l = l op r. Out of line: its temporaries stay out of expr()'s frame.
  CX_NOINLINE void combine(Tok op, Cx& l, const Cx& r) const {
    switch (op) {
      case Tok::Add:
      case Tok::Sub:
        if (l.real() && r.real()) {
          l = Cx::of(real(op, l.re, r.re));
        } else {
          l.re = real(op, l.re, r.re);
          l.im = real(op, l.im, r.im);
        }
        return;
      case Tok::Mul: l = mul(l, r); return;
      case Tok::Div: l = div(l, r); return;
      case Tok::Pow: l = pow(l, r); return;
      case Tok::Angle: l = polar(l, r); return;
      case Tok::Neg:
        l.re = real(Tok::Neg, l.re);
        if (!l.real()) l.im = real(Tok::Neg, l.im);
        return;
      default:  // ⌟ ⁿ√ nPr nCr: real only
        if (!l.real() || !r.real()) math();
        l = Cx::of(real(op, l.re, r.re));
        return;
    }
  }

  CX_NOINLINE Cx mul(const Cx& a, const Cx& b) const {
    if (a.real() && b.real()) return Cx::of(real(Tok::Mul, a.re, b.re));
    Cx r;
    r.re = real(Tok::Sub, real(Tok::Mul, a.re, b.re), real(Tok::Mul, a.im, b.im));
    r.im = real(Tok::Add, real(Tok::Mul, a.re, b.im), real(Tok::Mul, a.im, b.re));
    return r;
  }

  CX_NOINLINE Cx div(const Cx& a, const Cx& b) const {
    if (b.re.v == 0 && b.im.v == 0) math();
    if (b.real()) {
      Cx r;
      r.re = real(Tok::Div, a.re, b.re);
      r.im = a.real() ? Num::ofInt(0) : real(Tok::Div, a.im, b.re);
      return r;
    }
    // (a+bi)/(c+di) = ((ac+bd) + (bc−ad)i) / (c²+d²): exact while the parts stay exact.
    const Num &p = a.re, &q = a.im, &c = b.re, &d = b.im;
    Num c2, d2, den, ac, bd, bc, ad, n1, n2;
    Cx r;
    if (tryReal(Tok::Mul, c, c, c2) && tryReal(Tok::Mul, d, d, d2) && tryReal(Tok::Add, c2, d2, den) &&
        tryReal(Tok::Mul, p, c, ac) && tryReal(Tok::Mul, q, d, bd) && tryReal(Tok::Add, ac, bd, n1) &&
        tryReal(Tok::Mul, q, c, bc) && tryReal(Tok::Mul, p, d, ad) && tryReal(Tok::Sub, bc, ad, n2) &&
        tryReal(Tok::Div, n1, den, r.re) && tryReal(Tok::Div, n2, den, r.im))
      return r;
    // c² + d² out of range (|c| ~ 1E50): Smith's division in doubles.
    double re, im;
    if (std::fabs(c.v) >= std::fabs(d.v)) {
      const double t = d.v / c.v, s = c.v + d.v * t;
      re = (p.v + q.v * t) / s;
      im = (q.v - p.v * t) / s;
    } else {
      const double t = c.v / d.v, s = d.v + c.v * t;
      re = (p.v * t + q.v) / s;
      im = (q.v * t - p.v) / s;
    }
    if (!holdable(re) || !holdable(im)) math();
    r.re = Num::ofDouble(re);
    r.im = Num::ofDouble(im);
    return r;
  }

  // z^n: a real base and exponent go to the real engine ((−8)^(1/3), 2^0.5); a complex base
  // takes an integer exponent (binary powering, exact while it can be).
  CX_NOINLINE Cx pow(const Cx& base, const Cx& ex) const {
    if (!ex.real()) math();
    if (base.real()) return Cx::of(real(Tok::Pow, base.re, ex.re));
    const double n = ex.re.v;
    if (n != std::floor(n) || std::fabs(n) > 1e9) math();
    long long k = static_cast<long long>(std::fabs(n));
    Cx result = Cx::of(Num::ofInt(1)), b = base;
    while (k) {
      if (k & 1) result = mul(result, b);
      k >>= 1;
      if (k) b = mul(b, b);
    }
    if (n < 0) result = div(Cx::of(Num::ofInt(1)), result);
    return result;
  }

  // r∠θ: r and θ real, θ in the angle unit (the real cos / sin: cos 90° is exactly 0).
  CX_NOINLINE Cx polar(const Cx& r, const Cx& t) const {
    if (!r.real() || !t.real()) math();
    Cx z;
    z.re = real(Tok::Mul, r.re, real(Tok::Cos, t.re));
    z.im = real(Tok::Mul, r.re, real(Tok::Sin, t.re));
    return z;
  }
};

// Rounding noise from cancellation ((1∠30)³ = 1.1E-16+i): an inexact part below 1E-14 of the
// other one is 0. An exact part is a true value and stays.
void tidyResult(Cx& z) {
  const double a = std::fabs(z.re.v), b = std::fabs(z.im.v);
  if (!z.re.exact && a != 0 && a < 1e-14 * b) z.re = Num::ofInt(0);
  if (!z.im.exact && b != 0 && b < 1e-14 * a) z.im = Num::ofInt(0);
  if (z.im.v == 0) z.im = Num::ofInt(0);  // no −0 imaginary part
}

// "4i", "−i", "+1⌟2i" (sign: always "−" for a negative part; "+" only after a real part).
std::string imagText(const Num& im, bool afterReal, bool preferFraction, NormMode norm) {
  const bool neg = im.v < 0;
  std::string mag = formatResult(neg ? negated(im) : im, preferFraction, norm);
  if (mag == "1") mag.clear();
  return std::string(neg ? kMinus : afterReal ? "+" : "") + mag + "i";
}

}  // namespace

CxResult evaluateComplex(const std::vector<Tok>& toks, Vars& vars, const ImagVars& imag, AngleUnit unit,
                         double (*random)()) {
  const EvalRun run;  // the shared run (budget, cancel hook): nothing here calls f again and again
  CxResult r;
  size_t n = toks.size();
  if (n > 0 && (toks[n - 1] == Tok::ToPolar || toks[n - 1] == Tok::ToRect)) {
    r.forced = toks[n - 1] == Tok::ToPolar ? 1 : 0;
    --n;
  }
  try {
    CxParser p(toks, n, vars, imag, unit, random);
    r.value = p.run();
    tidyResult(r.value);
  } catch (const CxFail& f) {
    r.error = f.e;
    r.errorPos = f.pos;
    r.forced = -1;
  }
  return r;
}

bool cxAbs(const Cx& z, Num& out) {
  CalcError e;
  if (z.real()) return realOp(Tok::Abs, z.re, Num(), false, AngleUnit::Rad, nullptr, out, e);
  Num a2, b2, s;
  // Exact parts: √(a²+b²) exactly when it is (|3+4i| = 5).
  if (z.re.exact && z.im.exact && tryReal(Tok::Mul, z.re, z.re, a2) && tryReal(Tok::Mul, z.im, z.im, b2) &&
      tryReal(Tok::Add, a2, b2, s) && realOp(Tok::Sqrt, s, Num(), false, AngleUnit::Rad, nullptr, out, e))
    return true;
  double h = std::hypot(z.re.v, z.im.v);
  if (!holdable(h)) return false;
  out = Num::ofDouble(h);
  return true;
}

bool cxArg(const Cx& z, AngleUnit unit, Num& out) {
  if (z.im.v == 0) {  // real: 0, or 180° for a negative one (atan2 would give −180° for a −0 part)
    if (z.re.v >= 0) out = Num::ofInt(0);
    else out = unit == AngleUnit::Deg ? Num::ofInt(180) : unit == AngleUnit::Gra ? Num::ofInt(200) : Num::ofDouble(kPi);
    return true;
  }
  double t = fromRad(std::atan2(z.im.v, z.re.v), unit);
  if (!std::isfinite(t)) return false;
  out = Num::ofDouble(clean(t));
  return true;
}

std::string complexText(const Cx& z, CxForm form, bool preferFraction, NormMode norm, AngleUnit unit) {
  if (form == CxForm::Polar) {
    if (z.re.v == 0 && z.im.v == 0) return "0";
    Num r, t;
    if (cxAbs(z, r) && cxArg(z, unit, t))
      return formatResult(r, preferFraction, norm) + "\xE2\x88\xA0" + formatResult(t, preferFraction, norm);
    // |z| out of range (parts near 1E100): show it as a+bi
  }
  if (z.real()) return formatResult(z.re, preferFraction, norm);
  const std::string re = z.re.v == 0 ? std::string() : formatResult(z.re, preferFraction, norm);
  return re + imagText(z.im, !re.empty(), preferFraction, norm);
}

void complexParts(const Cx& z, CxForm form, bool preferFraction, NormMode norm, AngleUnit unit, std::string& first,
                  std::string& second) {
  if (form == CxForm::Polar) {
    Num r, t;
    if (cxAbs(z, r) && cxArg(z, unit, t)) {
      first = formatResult(r, preferFraction, norm);
      second = "\xE2\x88\xA0" + formatResult(t, preferFraction, norm);
      return;
    }
  }
  first = formatResult(z.re, preferFraction, norm);
  second = z.im.v == 0 ? std::string("0") : imagText(z.im, false, preferFraction, norm);
}

}  // namespace calc
