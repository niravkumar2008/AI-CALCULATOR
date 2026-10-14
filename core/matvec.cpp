#include "matvec.h"

#include <cmath>
#include <cstdlib>
#include <memory>

#include "stat.h"  // modeKeyToken, cellText, drawEntry, drawErrorScreen
#include "text.h"

namespace calc {

Mat Mat::zero(int rows, int cols) {
  Mat m;
  m.rows = static_cast<uint8_t>(rows);
  m.cols = static_cast<uint8_t>(cols);
  return m;  // Num() is an exact 0
}

namespace {

constexpr double kPi = 3.14159265358979323846;  // as calc_engine.cpp
constexpr double kE = 2.71828182845904523536;

struct MvFail {
  CalcError e;
  int pos;
};

// Same stack discipline as calc_engine.cpp's Parser and cmplx.cpp's (PORT_NOTES 2026-10-11): the
// precedence levels inline into one expr() frame per nesting level; whatever has big locals (a
// Mat) is a leaf.
#if defined(__GNUC__)
#define MV_NOINLINE __attribute__((noinline))
#define MV_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define MV_NOINLINE __declspec(noinline)
#define MV_INLINE __forceinline
#else
#define MV_NOINLINE
#define MV_INLINE inline
#endif

bool isDigitTok(Tok t) { return t <= Tok::D9; }
bool isRealFunc(Tok t) { return t >= Tok::Sin && t <= Tok::RanInt; }
bool isVarTok(Tok t) { return t >= Tok::MatA && t <= Tok::VctAns; }
bool isFuncTok(Tok t) { return isRealFunc(t) || t == Tok::Det || t == Tok::Trn; }
bool isValueTok(Tok t) { return (t >= Tok::Pi && t <= Tok::VarM) || isVarTok(t); }
bool startsOperandTok(Tok t) {
  return isDigitTok(t) || t == Tok::Dot || t == Tok::Exp10 || t == Tok::Open || isFuncTok(t) || isValueTok(t);
}

// One step of the real engine (exact fractions, its range checks); an error is thrown at `pos`.
Num op(Tok o, const Num& a, const Num& b, int pos) {
  Num out;
  CalcError e = CalcError::Math;
  if (!realOp(o, a, b, false, AngleUnit::Rad, nullptr, out, e)) throw MvFail{e, pos};
  return out;
}
Num op1(Tok o, const Num& a, int pos) { return op(o, a, Num(), pos); }

// a·d − b·c
Num cross2(const Num& a, const Num& d, const Num& b, const Num& c, int pos) {
  return op(Tok::Sub, op(Tok::Mul, a, d, pos), op(Tok::Mul, b, c, pos), pos);
}

// The cofactor of a 3 × 3 matrix's (i, j), its sign included (the cyclic form).
Num cofactor3(const Mat& a, int i, int j, int pos) {
  const int i1 = (i + 1) % 3, i2 = (i + 2) % 3, j1 = (j + 1) % 3, j2 = (j + 2) % 3;
  return cross2(a.at(i1, j1), a.at(i2, j2), a.at(i1, j2), a.at(i2, j1), pos);
}

// The size of the products a determinant adds up: below 1E-13 of it, an inexact determinant is
// rounding noise (0.1, 0.2 / 0.3, 0.6 is singular).
double detScale(const Mat& a) {
  double s = 1;
  for (int r = 0; r < a.rows; ++r) {
    double mx = 0;
    for (int c = 0; c < a.cols; ++c) mx = std::fmax(mx, std::fabs(a.at(r, c).v));
    s *= mx;
  }
  return s;
}

MV_NOINLINE Num det(const Mat& a, int pos) {
  if (a.rows != a.cols || !a.defined()) throw MvFail{CalcError::Dim, pos};
  Num d;
  if (a.rows == 1) {
    d = a.at(0, 0);
  } else if (a.rows == 2) {
    d = cross2(a.at(0, 0), a.at(1, 1), a.at(0, 1), a.at(1, 0), pos);
  } else {
    d = Num::ofInt(0);
    for (int j = 0; j < 3; ++j) d = op(Tok::Add, d, op(Tok::Mul, a.at(0, j), cofactor3(a, 0, j, pos), pos), pos);
  }
  if (!d.exact && std::fabs(d.v) <= 1e-13 * detScale(a)) d = Num::ofInt(0);
  return d;
}

MV_NOINLINE Mat inverse(const Mat& a, int pos) {
  const Num d = det(a, pos);  // Dim unless square
  if (d.v == 0) throw MvFail{CalcError::Math, pos};
  Mat r = Mat::zero(a.rows, a.cols);
  const int n = a.rows;
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      // inverse(j, i) = cofactor(i, j) / det
      Num c;
      if (n == 1) c = Num::ofInt(1);
      else if (n == 2) c = (i + j) % 2 ? op1(Tok::Neg, a.at(1 - i, 1 - j), pos) : a.at(1 - i, 1 - j);
      else c = cofactor3(a, i, j, pos);
      r.at(j, i) = op(Tok::Div, c, d, pos);
    }
  return r;
}

MV_NOINLINE Mat product(const Mat& a, const Mat& b, int pos) {
  if (a.cols != b.rows) throw MvFail{CalcError::Dim, pos};
  Mat r = Mat::zero(a.rows, b.cols);
  for (int i = 0; i < a.rows; ++i)
    for (int j = 0; j < b.cols; ++j) {
      Num s = op(Tok::Mul, a.at(i, 0), b.at(0, j), pos);
      for (int k = 1; k < a.cols; ++k) s = op(Tok::Add, s, op(Tok::Mul, a.at(i, k), b.at(k, j), pos), pos);
      r.at(i, j) = s;
    }
  return r;
}

struct Val {
  MvKind kind = MvKind::Scalar;
  int8_t slot = -1;  // a matrix / vector: its place in the parser's pool
  Num s;             // a scalar
};

class MvParser {
 public:
  MvParser(const std::vector<Tok>& t, Vars& v, const Mat* mv, bool vec, AngleUnit u, double (*rnd)())
      : t_(t), n_(t.size()), vars_(v), mv_(mv), vec_(vec), unit_(u), rnd_(rnd) {}

  void run(Val& out) {
    if (n_ == 0) throw MvFail{CalcError::Syntax, 0};
    expr(out);
    if (i_ < n_) syntax();
  }
  const Mat& mat(const Val& v) const { return pool_[v.slot]; }

 private:
  const std::vector<Tok>& t_;
  size_t n_;
  Vars& vars_;
  const Mat* mv_;
  bool vec_;
  AngleUnit unit_;
  double (*rnd_)();
  size_t i_ = 0;
  int depth_ = 0;
  Mat pool_[kMvSlots];
  bool used_[kMvSlots] = {};

  bool at(Tok k) const { return i_ < n_ && t_[i_] == k; }
  bool end() const { return i_ >= n_; }
  int mathPos() const { return static_cast<int>(i_ > 0 ? i_ - 1 : 0); }
  [[noreturn]] void syntax() const { throw MvFail{CalcError::Syntax, static_cast<int>(i_ < n_ ? i_ : n_)}; }
  [[noreturn]] void fail(CalcError e) const { throw MvFail{e, mathPos()}; }
  void close() {
    if (at(Tok::Close)) {
      ++i_;
      return;
    }
    if (!end()) syntax();
  }
  void enter() {  // the shared nesting budget (calc_engine.h kMaxNesting): one more = Stack ERROR
    if (++depth_ > kMaxNesting) throw MvFail{CalcError::Stack, static_cast<int>(i_)};
  }

  int alloc() {
    for (int s = 0; s < kMvSlots; ++s)
      if (!used_[s]) {
        used_[s] = true;
        return s;
      }
    throw MvFail{CalcError::Stack, static_cast<int>(i_)};  // the matrix stack is full
  }
  void release(Val& v) {
    if (v.slot >= 0) used_[v.slot] = false;
    v.slot = -1;
  }
  Mat& m(const Val& v) { return pool_[v.slot]; }

  Num real(Tok o, const Num& a, const Num& b = Num(), bool two = false) const {
    Num out;
    CalcError e = CalcError::Math;
    if (!realOp(o, a, b, two, unit_, rnd_, out, e)) {
      if (e == CalcError::Syntax) syntax();
      fail(e);
    }
    return out;
  }

  // ---- the precedence levels (as calc_engine.cpp's Parser; the dot product shares × ÷'s)
  void expr(Val& out) {
    Val r;
    bool first = true;
    Tok o = Tok::Add;
    for (;;) {
      mulDiv(first ? out : r);
      if (!first) combine(o, out, r);
      first = false;
      if (!at(Tok::Add) && !at(Tok::Sub)) return;
      o = t_[i_++];
    }
  }

  MV_INLINE void mulDiv(Val& out) {
    Val r;
    bool first = true;
    Tok o = Tok::Mul;
    for (;;) {
      perm(first ? out : r);
      if (!first) combine(o, out, r);
      first = false;
      if (!at(Tok::Mul) && !at(Tok::Div) && !at(Tok::DotP)) return;
      o = t_[i_++];
    }
  }

  MV_INLINE void perm(Val& out) {
    Val r;
    bool first = true;
    Tok o = Tok::NPr;
    for (;;) {
      implicitMul(first ? out : r);
      if (!first) combine(o, out, r);
      first = false;
      if (!at(Tok::NPr) && !at(Tok::NCr)) return;
      o = t_[i_++];
    }
  }

  MV_INLINE void implicitMul(Val& out) {
    Val r;
    bool first = true;
    for (;;) {
      neg(first ? out : r);
      if (!first) combine(Tok::Mul, out, r);
      first = false;
      if (end() || !startsOperandTok(t_[i_])) return;
    }
  }

  MV_INLINE void neg(Val& out) {
    bool minus = false;
    while (at(Tok::Neg)) {
      ++i_;
      minus = !minus;
    }
    frac(out);
    if (minus) negate(out);
  }

  MV_INLINE void frac(Val& out) {
    Val r;
    bool first = true;
    for (;;) {
      power(first ? out : r);
      if (!first) combine(Tok::Frac, out, r);
      first = false;
      if (!at(Tok::Frac)) return;
      ++i_;
    }
  }

  MV_INLINE void power(Val& out) {
    operand(out);
    while (at(Tok::Pow) || at(Tok::XRoot)) {
      const Tok o = t_[i_++];
      enter();
      Val r;
      expr(r);
      close();
      --depth_;
      combine(o, out, r);  // scalars only (x² x³ x⁻¹ are the matrix powers)
      postfix(out);
    }
  }

  MV_NOINLINE void operand(Val& out) {
    primary(out);
    postfix(out);
  }

  // x², x³, x⁻¹ (a square matrix too), x! and % (scalars), as many as follow.
  MV_NOINLINE void postfix(Val& v) {
    while (!end()) {
      const Tok k = t_[i_];
      if (k != Tok::Sq && k != Tok::Cube && k != Tok::Inv && k != Tok::Fact && k != Tok::Pct) break;
      ++i_;
      if (v.kind == MvKind::Scalar) {
        v.s = real(k, v.s);
        continue;
      }
      if (v.kind == MvKind::Vector || k == Tok::Fact || k == Tok::Pct) fail(CalcError::Syntax);
      matPostfix(k, m(v));
    }
  }

  MV_NOINLINE void matPostfix(Tok k, Mat& a) const {
    const int pos = mathPos();
    if (k == Tok::Inv) {
      a = inverse(a, pos);
      return;
    }
    if (a.rows != a.cols) throw MvFail{CalcError::Dim, pos};
    const Mat sq = product(a, a, pos);
    a = k == Tok::Cube ? product(sq, a, pos) : sq;
  }

  // In the recursion: a small frame (numbers, values and functions are out of line).
  MV_NOINLINE void primary(Val& out) {
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
    syntax();  // ∫( d/dx( "=" i ∠ the BASE-N tokens, an operator where a value belongs
  }

  MV_NOINLINE void number(Val& out) {
    size_t i = i_;
    Num v;
    CalcError e = CalcError::Syntax;
    int pos = static_cast<int>(i_);
    if (!realNumber(t_, i, v, e, pos)) throw MvFail{e, pos};
    i_ = i;
    out.kind = MvKind::Scalar;
    out.s = v;
  }

  MV_NOINLINE void value(Val& out) {
    const Tok k = t_[i_];
    out.kind = MvKind::Scalar;
    if (isVarTok(k)) {
      // MatA..MatAns in MATRIX, VctA..VctAns in VECTOR; one without a size yet: Dim ERROR.
      const bool vecTok = k >= Tok::VctA;
      if (vecTok != vec_) syntax();
      const Mat& src = mv_[static_cast<int>(k) - static_cast<int>(vecTok ? Tok::VctA : Tok::MatA)];
      if (!src.defined()) throw MvFail{CalcError::Dim, static_cast<int>(i_)};
      ++i_;
      out.slot = static_cast<int8_t>(alloc());
      out.kind = vec_ ? MvKind::Vector : MvKind::Matrix;
      m(out) = src;
      return;
    }
    ++i_;
    switch (k) {
      case Tok::Pi: out.s = Num::ofDouble(kPi); return;
      case Tok::E: out.s = Num::ofDouble(kE); return;
      case Tok::Ran: {  // as calc_engine.cpp: 3 decimal places
        const double r = std::floor(rnd_() * 1000) / 1000;
        out.s = real(Tok::Div, Num::ofInt(static_cast<long long>(r * 1000)), Num::ofInt(1000));
        return;
      }
      default: out.s = vars_.get(k); return;
    }
  }

  // A function: its first argument straight into `out`, then its value (apply()).
  MV_NOINLINE void function(Val& out) {
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
    Val none;
    apply(k, out, none, false);
  }

  MV_NOINLINE void secondArg(Tok k, Val& a) {
    Val b;
    expr(b);
    close();
    --depth_;
    apply(k, a, b, true);
  }

  // a = k(a [, b]).
  MV_NOINLINE void apply(Tok k, Val& a, Val& b, bool two) {
    if (a.kind == MvKind::Scalar && b.kind == MvKind::Scalar) {
      if (k == Tok::Det || k == Tok::Trn) fail(CalcError::Syntax);  // det( Trn( take a matrix
      a.s = real(k, a.s, b.s, two);
      return;
    }
    if (two) fail(CalcError::Syntax);
    const int pos = mathPos();
    Mat& x = m(a);
    if (k == Tok::Abs) {  // each element of a matrix; a vector's magnitude
      if (a.kind == MvKind::Matrix) {
        for (int r = 0; r < x.rows; ++r)
          for (int c = 0; c < x.cols; ++c) x.at(r, c) = op1(Tok::Abs, x.at(r, c), pos);
        return;
      }
      Num s = Num::ofInt(0);
      for (int c = 0; c < x.cols; ++c) s = op(Tok::Add, s, op(Tok::Mul, x.at(0, c), x.at(0, c), pos), pos);
      toScalar(a, op1(Tok::Sqrt, s, pos));
      return;
    }
    if (a.kind != MvKind::Matrix || (k != Tok::Det && k != Tok::Trn)) fail(CalcError::Syntax);
    if (k == Tok::Det) return toScalar(a, det(x, pos));
    transpose(x);
  }

  MV_NOINLINE static void transpose(Mat& x) {
    Mat t = Mat::zero(x.cols, x.rows);
    for (int r = 0; r < x.rows; ++r)
      for (int c = 0; c < x.cols; ++c) t.at(c, r) = x.at(r, c);
    x = t;
  }

  void toScalar(Val& v, const Num& s) {
    release(v);
    v.kind = MvKind::Scalar;
    v.s = s;
  }

  MV_NOINLINE void negate(Val& v) {
    if (v.kind == MvKind::Scalar) {
      v.s = real(Tok::Neg, v.s);
      return;
    }
    Mat& x = m(v);
    for (int r = 0; r < x.rows; ++r)
      for (int c = 0; c < x.cols; ++c) x.at(r, c) = op1(Tok::Neg, x.at(r, c), mathPos());
  }

  // l = l op r. Out of line: its temporaries stay out of expr()'s frame.
  MV_NOINLINE void combine(Tok o, Val& l, Val& r) {
    if (l.kind == MvKind::Scalar && r.kind == MvKind::Scalar) {
      if (o == Tok::DotP) fail(CalcError::Syntax);  // · takes two vectors
      l.s = real(o, l.s, r.s);
      return;
    }
    const int pos = mathPos();
    if (o == Tok::Mul && l.kind == MvKind::Scalar) {  // k × M: M's slot becomes the result's
      scale(m(r), l.s, Tok::Mul, pos);
      l.kind = r.kind;
      l.slot = r.slot;
      r.slot = -1;
      return;
    }
    if ((o == Tok::Mul || o == Tok::Div) && r.kind == MvKind::Scalar) {  // M × k, M ÷ k
      scale(m(l), r.s, o, pos);
      return;
    }
    if (l.kind != r.kind || r.kind == MvKind::Scalar) fail(CalcError::Syntax);  // k + M, M ⌟ ..., k ÷ M ...
    Mat& a = m(l);
    const Mat& b = m(r);
    switch (o) {
      case Tok::Add:
      case Tok::Sub:
        if (a.rows != b.rows || a.cols != b.cols) fail(CalcError::Dim);
        for (int i = 0; i < a.rows; ++i)
          for (int j = 0; j < a.cols; ++j) a.at(i, j) = op(o, a.at(i, j), b.at(i, j), pos);
        break;
      case Tok::Mul:
        if (l.kind == MvKind::Matrix) a = product(a, b, pos);
        else crossProduct(a, b, pos);
        break;
      case Tok::DotP: {
        if (l.kind != MvKind::Vector) fail(CalcError::Syntax);
        if (a.cols != b.cols) fail(CalcError::Dim);
        Num s = Num::ofInt(0);
        for (int j = 0; j < a.cols; ++j) s = op(Tok::Add, s, op(Tok::Mul, a.at(0, j), b.at(0, j), pos), pos);
        release(r);
        return toScalar(l, s);
      }
      default:
        fail(CalcError::Syntax);  // M ÷ M, ^, ⌟, nPr ...
    }
    release(r);
  }

  MV_NOINLINE static void scale(Mat& x, const Num& k, Tok o, int pos) {
    for (int i = 0; i < x.rows; ++i)
      for (int j = 0; j < x.cols; ++j) x.at(i, j) = o == Tok::Div ? op(Tok::Div, x.at(i, j), k, pos) : op(Tok::Mul, k, x.at(i, j), pos);
  }

  MV_NOINLINE static void crossProduct(Mat& a, const Mat& b, int pos) {
    if (a.cols != 3 || b.cols != 3) throw MvFail{CalcError::Dim, pos};  // 3D only
    Mat c = Mat::zero(1, 3);
    for (int j = 0; j < 3; ++j) {
      const int j1 = (j + 1) % 3, j2 = (j + 2) % 3;
      c.at(0, j) = cross2(a.at(0, j1), b.at(0, j2), a.at(0, j2), b.at(0, j1), pos);
    }
    a = c;
  }
};

const char* const kNames[2][4] = {{"MatA", "MatB", "MatC", "MatAns"}, {"VctA", "VctB", "VctC", "VctAns"}};

std::string pad(std::string s, int width) {
  const int n = textLength(s);
  if (n < width) s.append(static_cast<size_t>(width - n), ' ');
  return s;
}
std::string two(const std::string& a, const std::string& b) { return pad(a, 11) + b; }

void rightText(Framebuffer& fb, int row, const std::string& s) {
  const int n = textLength(s);
  fb.drawText(n >= Framebuffer::kCols ? 0 : Framebuffer::kCols - n, row, s);
}

double fallbackRandom() { return static_cast<double>(std::rand()) / (static_cast<double>(RAND_MAX) + 1.0); }

int digitOf(DKey k) {
  switch (k) {
    case DKey::D0: return 0;
    case DKey::D1: return 1;
    case DKey::D2: return 2;
    case DKey::D3: return 3;
    case DKey::D4: return 4;
    case DKey::D5: return 5;
    case DKey::D6: return 6;
    case DKey::D7: return 7;
    case DKey::D8: return 8;
    case DKey::D9: return 9;
    default: return -1;
  }
}

// Dim's sizes, in the Casio's order (m×n? 1:3×3 ... 6:2×1, ▼ 1:1×3 2:1×2 3:1×1).
const uint8_t kSizes[9][2] = {{3, 3}, {3, 2}, {3, 1}, {2, 3}, {2, 2}, {2, 1}, {1, 3}, {1, 2}, {1, 1}};
const char* const kTimes = "\xC3\x97";  // ×

}  // namespace

MvResult evaluateMatVec(const std::vector<Tok>& toks, Vars& vars, const Mat* mv, bool vectors, AngleUnit unit,
                        double (*random)()) {
  const EvalRun run;  // the shared run: nothing here repeats, but the scope is the same as COMP's
  MvResult r;
  // ~2.3 KB with its matrix stack: on the heap, not the (small ESP32) loop stack.
  std::unique_ptr<MvParser> p(new MvParser(toks, vars, mv, vectors, unit, random ? random : fallbackRandom));
  try {
    Val v;
    p->run(v);
    r.kind = v.kind;
    if (v.kind == MvKind::Scalar) r.scalar = v.s;
    else r.m = p->mat(v);
  } catch (const MvFail& f) {
    r.error = f.e;
    r.errorPos = f.pos;
    r.kind = MvKind::Scalar;
  }
  return r;
}

bool matDet(const Mat& a, Num& out, CalcError& err) {
  try {
    out = det(a, 0);
    return true;
  } catch (const MvFail& f) {
    err = f.e;
    return false;
  }
}

bool matInverse(const Mat& a, Mat& out, CalcError& err) {
  try {
    out = inverse(a, 0);
    return true;
  } catch (const MvFail& f) {
    err = f.e;
    return false;
  }
}

std::string mvCellText(const Num& v, int width) {
  std::string f;
  if (fractionText(v, f) && textLength(f) <= width) return f;
  return cellText(v.v, width);
}

// ============================================================================= the screens

void MatVecApp::reset(bool vectors) {
  vectors_ = vectors;
  for (Mat& m : var_) m = Mat();
  screen_ = Screen::Select;
  menuBack_ = Screen::Select;
  menuFromCalc_ = true;
  forDim_ = true;
  pick_ = dimPage_ = 0;
  ed_ = row_ = col_ = 0;
  typing_ = sto_ = false;
  entry_.clear();
  message_ = "";
  action_ = Action::None;
}

void MatVecApp::setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)()) {
  vars_ = vars;
  unit_ = unit;
  norm_ = norm;
  random_ = random;
}

Vars& MatVecApp::calcVars() const { return vars_ ? *vars_ : ownVars_; }

const char* MatVecApp::varName(bool vectors, int i) { return i >= 0 && i < 4 ? kNames[vectors ? 1 : 0][i] : ""; }

bool MatVecApp::setVar(int i, const Mat& m) {
  if (i < 0 || i > 3) return false;
  if (m.defined()) {
    if (m.rows > Mat::kMax || m.cols < 1 || m.cols > Mat::kMax) return false;
    if (vectors_ && (m.rows != 1 || m.cols < 2)) return false;
  } else if (m.cols != 0) {
    return false;
  }
  var_[i] = m;
  if (screen_ == Screen::Editor && ed_ == i) openEditor(i);  // the cursor stays inside
  return true;
}

MatVecApp::Action MatVecApp::takeAction(Tok& t) {
  const Action a = action_;
  t = actionTok_;
  action_ = Action::None;
  return a;
}

void MatVecApp::openMenu() {
  menuFromCalc_ = true;
  sto_ = false;
  screen_ = Screen::Menu;
}

void MatVecApp::showAns(const Mat& m) {
  var_[3] = m;
  openEditor(3);
}

void MatVecApp::openEditor(int i) {
  ed_ = i;
  row_ = col_ = 0;
  typing_ = sto_ = false;
  entry_.clear();
  screen_ = Screen::Editor;
}

void MatVecApp::onKey(DKey k, bool shift, bool alpha) {
  action_ = Action::None;
  if (k == DKey::Shift || k == DKey::Alpha || k == DKey::Mode || k == DKey::On || k >= DKey::Count) return;
  if (shift && (k == DKey::Up || k == DKey::Down)) return;  // SHIFT ▲▼ is the platform's (brightness)
  const int d = digitOf(k);
  auto toCalc = [&] { action_ = Action::ToCalc; };
  auto insert = [&](Tok t) {
    action_ = Action::Insert;
    actionTok_ = t;
  };
  // SHIFT 4 (MATRIX) / SHIFT 5 (VECTOR): the menu, from any screen but a cell being typed.
  if (shift && !alpha && d == (vectors_ ? 5 : 4) && !(screen_ == Screen::Editor && typing_)) {
    if (screen_ != Screen::Menu) {
      menuBack_ = screen_;
      menuFromCalc_ = false;
    }
    sto_ = false;
    screen_ = Screen::Menu;
    return;
  }
  switch (screen_) {
    case Screen::Select:
      if (k == DKey::AC) return toCalc();
      if (d >= 1 && d <= 3) {
        pick_ = d - 1;
        if (forDim_ || !var_[pick_].defined()) {  // Data on one without a size: its size first
          dimPage_ = 0;
          screen_ = Screen::Dim;
        } else {
          openEditor(pick_);
        }
      }
      return;
    case Screen::Dim: {
      if (k == DKey::AC) return toCalc();
      if (!vectors_ && (k == DKey::Up || k == DKey::Down)) {
        dimPage_ = k == DKey::Down ? 1 : 0;
        return;
      }
      int rows = 0, cols = 0;
      if (vectors_ && (d == 1 || d == 2)) {
        rows = 1;
        cols = d == 1 ? 3 : 2;
      } else if (!vectors_ && d >= 1 && d <= (dimPage_ ? 3 : 6)) {
        rows = kSizes[dimPage_ * 6 + d - 1][0];
        cols = kSizes[dimPage_ * 6 + d - 1][1];
      }
      if (rows) {
        var_[pick_] = Mat::zero(rows, cols);  // a new size: every element 0
        openEditor(pick_);
      }
      return;
    }
    case Screen::Menu:
      if (k == DKey::AC) {
        if (menuFromCalc_) return toCalc();
        screen_ = menuBack_;
        return;
      }
      if (d == 1 || d == 2) {
        forDim_ = d == 1;
        screen_ = Screen::Select;
      } else if (d >= 3 && d <= 6) {
        insert(static_cast<Tok>(static_cast<int>(vectors_ ? Tok::VctA : Tok::MatA) + d - 3));
      } else if (d == 7) {
        insert(vectors_ ? Tok::DotP : Tok::Det);
      } else if (d == 8 && !vectors_) {
        insert(Tok::Trn);
      }
      return;
    case Screen::Editor:
      return keyEditor(k, shift, alpha);
    case Screen::Error:
      if (k == DKey::AC) {
        typing_ = false;
        entry_.clear();
      } else if (k != DKey::Left && k != DKey::Right) {
        return;
      }
      screen_ = Screen::Editor;
      return;
  }
}

void MatVecApp::keyEditor(DKey k, bool shift, bool alpha) {
  Mat& m = var_[ed_];
  if (sto_) {  // STO A / B / C: a copy of what this editor shows, then that one's editor
    sto_ = false;
    const int dst = k == DKey::Neg ? 0 : k == DKey::Dms ? 1 : k == DKey::Hyp ? 2 : -1;
    if (dst >= 0 && !alpha) {
      var_[dst] = m;
      openEditor(dst);
    }
    return;
  }
  Tok t;
  if (typing_) {
    if (k == DKey::Eq) {
      commitEntry();
    } else if (k == DKey::Del) {
      if (!entry_.empty()) entry_.pop_back();
    } else if (k == DKey::AC) {
      typing_ = false;
      entry_.clear();
    } else if (modeKeyToken(k, shift, alpha, t)) {
      if (static_cast<int>(entry_.size()) < kMaxTokens) entry_.push_back(t);
    }
    return;
  }
  switch (k) {
    case DKey::Up: if (row_ > 0) --row_; return;
    case DKey::Down: if (row_ + 1 < m.rows) ++row_; return;
    case DKey::Left: if (col_ > 0) --col_; return;
    case DKey::Right: if (col_ + 1 < m.cols) ++col_; return;
    case DKey::AC: action_ = Action::ToCalc; return;
    case DKey::Rcl:
      if (shift) sto_ = true;
      return;
    default: break;
  }
  if (ed_ == 3) {
    // The MatAns / VctAns screen: these keys go on calculating with it (as "MatAns" and the key).
    struct M { DKey k; bool shift; Tok t; };
    static const M kOps[] = {{DKey::Add, false, Tok::Add}, {DKey::Sub, false, Tok::Sub}, {DKey::Mul, false, Tok::Mul},
                             {DKey::Div, false, Tok::Div}, {DKey::Inv, false, Tok::Inv}, {DKey::Sq, false, Tok::Sq},
                             {DKey::Sq, true, Tok::Cube}};
    for (const M& o : kOps)
      if (o.k == k && o.shift == shift && !alpha && (!vectors_ || o.t <= Tok::Div)) {  // VctAns: + − × ÷
        action_ = Action::Insert;
        actionTok_ = o.t;
        return;
      }
    return;  // read only
  }
  if (k == DKey::Del) {
    m.at(row_, col_) = Num::ofInt(0);
    return;
  }
  if (k == DKey::Eq) return;
  if (modeKeyToken(k, shift, alpha, t)) {
    typing_ = true;
    entry_.assign(1, t);
  }
}

void MatVecApp::commitEntry() {
  if (entry_.empty()) {
    typing_ = false;
    return;
  }
  Vars copy = calcVars();  // an element's expression never changes the memories or Ans
  const EvalResult r = calc::evaluate(entry_, copy, unit_, random_ ? random_ : fallbackRandom);
  if (r.error != CalcError::None) {
    message_ = errorText(r.error);
    screen_ = Screen::Error;
    return;
  }
  Mat& m = var_[ed_];
  m.at(row_, col_) = r.value;
  typing_ = false;
  entry_.clear();
  // On to the next element: right, then the next row's first.
  if (col_ + 1 < m.cols) {
    ++col_;
  } else if (row_ + 1 < m.rows) {
    ++row_;
    col_ = 0;
  }
}

void MatVecApp::render(Framebuffer& fb) const {
  fb.setIcon(Icon::Up, false);
  fb.setIcon(Icon::Down, false);
  const int v = vectors_ ? 1 : 0;
  switch (screen_) {
    case Screen::Select:
      fb.drawText(0, 1, vectors_ ? "Vector?" : "Matrix?");
      fb.drawText(0, 2, two(std::string("1:") + kNames[v][0], std::string("2:") + kNames[v][1]));
      fb.drawText(0, 3, std::string("3:") + kNames[v][2]);
      break;
    case Screen::Dim:
      if (vectors_) {
        fb.drawText(0, 1, std::string(kNames[1][pick_]) + "(m) m?");
        fb.drawText(0, 2, two("1:3", "2:2"));
        break;
      }
      fb.drawText(0, 1, std::string(kNames[0][pick_]) + "(m" + kTimes + "n) m" + kTimes + "n?");
      for (int i = 0; i < (dimPage_ ? 3 : 6); i += 2) {
        auto item = [&](int j) {
          const uint8_t* s = kSizes[dimPage_ * 6 + j];
          return std::to_string(j + 1) + ":" + std::to_string(s[0]) + kTimes + std::to_string(s[1]);
        };
        fb.drawText(0, 2 + i / 2, i + 1 < (dimPage_ ? 3 : 6) ? two(item(i), item(i + 1)) : item(i));
      }
      fb.setIcon(Icon::Up, dimPage_ == 1);
      fb.setIcon(Icon::Down, dimPage_ == 0);
      break;
    case Screen::Menu:
      fb.drawText(0, 1, two("1:Dim", "2:Data"));
      fb.drawText(0, 2, two(std::string("3:") + kNames[v][0], std::string("4:") + kNames[v][1]));
      fb.drawText(0, 3, two(std::string("5:") + kNames[v][2], std::string("6:") + kNames[v][3]));
      fb.drawText(0, 4, vectors_ ? std::string("7:Dot") : two("7:det", "8:Trn"));
      break;
    case Screen::Editor: drawEditor(fb); break;
    case Screen::Error: drawErrorScreen(fb, message_); break;
  }
}

void MatVecApp::drawEditor(Framebuffer& fb) const {
  const Mat& m = var_[ed_];
  const std::string size = vectors_ ? std::to_string(m.cols) : std::to_string(m.rows) + kTimes + std::to_string(m.cols);
  fb.drawText(0, 1, std::string(kNames[vectors_ ? 1 : 0][ed_]) + "(" + size + ")");
  if (sto_) fb.drawText(22, 1, "STO");
  // [ the elements ], right-aligned in equal cells across columns 1-23.
  const int nc = m.cols > 0 ? m.cols : 1;
  const int w = (23 - (nc - 1)) / nc;
  for (int r = 0; r < m.rows; ++r) {
    const int row = 2 + r;
    fb.drawText(0, row, "[");
    fb.drawText(24, row, "]");
    for (int c = 0; c < m.cols; ++c) {
      const std::string s = mvCellText(m.at(r, c), w);
      const int start = 1 + c * (w + 1);
      fb.drawText(start + w - textLength(s), row, s);
      if (r == row_ && c == col_) fb.invertRect(start * 5 - 1, row * 8, w * 5 + 1, 8);
    }
  }
  if (typing_) drawEntry(fb, 6, 1, exprText(entry_), true);
  else if (m.defined()) rightText(fb, 6, formatResult(m.at(row_, col_), true, norm_));
}

}  // namespace calc
