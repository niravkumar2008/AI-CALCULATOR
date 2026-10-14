#include "eqn.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>

#include "stat.h"  // modeKeyToken, cellText, drawEntry, drawErrorScreen
#include "text.h"

namespace calc {

namespace {
constexpr int kCols = Framebuffer::kCols;
constexpr double kEps = DBL_EPSILON;
constexpr double kPi = 3.14159265358979323846;
const char* const kMinus = "\xE2\x88\x92";  // −, as formatDecimal() writes a negative number

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

double fallbackRandom() { return static_cast<double>(std::rand()) / (static_cast<double>(RAND_MAX) + 1.0); }

void rightText(Framebuffer& fb, int row, const std::string& s) {
  const int n = textLength(s);
  fb.drawText(n >= kCols ? 0 : kCols - n, row, s);
}

bool inRange(double v) { return std::isfinite(v) && std::fabs(v) < 1e100; }

// The power of 2 at or above m (> 0): dividing by it is exact.
double pow2Scale(double m) {
  int e = 0;
  std::frexp(m, &e);
  return std::ldexp(1.0, e);
}

// What the calculator can hold: below 1E-99 is 0 (as calc_engine's check()), and no −0.
double tidy(double v) {
  if (std::fabs(v) < 1e-99) return 0.0;
  return v + 0.0;
}
}  // namespace

const char* eqnTypeName(EqnType t) {
  switch (t) {
    case EqnType::Lin2: return "anX+bnY=cn";
    case EqnType::Lin3: return "anX+bnY+cnZ=dn";
    case EqnType::Quad: return "aX\xC2\xB2+bX+c=0";
    case EqnType::Cubic: return "aX\xC2\xB3+bX\xC2\xB2+cX+d=0";
    default: return "?";
  }
}

// ============================================================================= solvers

namespace eqn {

Outcome solveLinear(int n, const double* in, double* x) {
  if (n < 2 || n > 3) return Outcome::MathError;
  double m[3][4] = {};
  for (int i = 0; i < n; ++i)
    for (int j = 0; j <= n; ++j) {
      const double v = in[i * (n + 1) + j];
      if (!inRange(v)) return Outcome::MathError;
      m[i][j] = v;
    }
  // Equilibrate: every unknown's column to a largest |coefficient| of 1 (z_j = cs_j·x_j), then
  // every equation to a largest |coefficient| of 1, so 1E50 next to 1E-50 is no problem.
  double cs[3];
  for (int j = 0; j < n; ++j) {
    double mx = 0;
    for (int i = 0; i < n; ++i) mx = std::max(mx, std::fabs(m[i][j]));
    cs[j] = mx > 0 ? mx : 1.0;
    for (int i = 0; i < n; ++i) m[i][j] /= cs[j];
  }
  double bs = 0;
  for (int i = 0; i < n; ++i) {
    double mx = 0;
    for (int j = 0; j < n; ++j) mx = std::max(mx, std::fabs(m[i][j]));
    if (mx > 0)
      for (int j = 0; j <= n; ++j) m[i][j] /= mx;
    bs = std::max(bs, std::fabs(m[i][n]));
  }
  // Elimination with partial pivoting; a column whose best pivot is rounding noise has none.
  constexpr double kPivotTol = 1e-12;
  int rank = 0;
  for (int col = 0; col < n && rank < n; ++col) {
    int p = rank;
    for (int i = rank + 1; i < n; ++i)
      if (std::fabs(m[i][col]) > std::fabs(m[p][col])) p = i;
    if (std::fabs(m[p][col]) <= kPivotTol) continue;
    if (p != rank)
      for (int k = 0; k <= n; ++k) std::swap(m[p][k], m[rank][k]);
    for (int i = rank + 1; i < n; ++i) {
      const double f = m[i][col] / m[rank][col];
      for (int k = col; k <= n; ++k) m[i][k] -= f * m[rank][k];
      m[i][col] = 0;
    }
    ++rank;
  }
  if (rank < n) {
    // The equations left are 0 = constant: any constant beyond the noise contradicts.
    const double tolB = 1e-11 * bs;
    for (int i = rank; i < n; ++i)
      if (std::fabs(m[i][n]) > tolB) return Outcome::NoSolution;
    return Outcome::Infinite;
  }
  double z[3];
  for (int i = n - 1; i >= 0; --i) {
    double s = m[i][n], mag = std::fabs(m[i][n]);
    for (int k = i + 1; k < n; ++k) {
      s -= m[i][k] * z[k];
      mag += std::fabs(m[i][k] * z[k]);
    }
    // A difference within the rounding noise of its terms is 0 (x+y=2, x−y=0 style systems
    // with decimal coefficients would otherwise show 1E-17).
    if (std::fabs(s) <= 64 * kEps * mag) s = 0;
    z[i] = s / m[i][i];
  }
  for (int j = 0; j < n; ++j) {
    const double v = z[j] / cs[j];
    if (!inRange(v)) return Outcome::MathError;
    x[j] = tidy(v);
  }
  return Outcome::Ok;
}

Outcome solveQuadratic(double a, double b, double c, Root out[2], int& count, double* xv, double* yv) {
  count = 0;
  if (!inRange(a) || !inRange(b) || !inRange(c) || a == 0) return Outcome::MathError;
  // Scale to a largest |coefficient| in [0.5, 1) by a power of 2 (exact: 2X² − 3X + 1 keeps its
  // exact roots): no overflow or underflow in b² or 4ac.
  const double s = pow2Scale(std::max(std::fabs(a), std::max(std::fabs(b), std::fabs(c))));
  a /= s;
  b /= s;
  c /= s;
  // D = b² − 4ac with both products' rounding errors added back (fma), then 0 if it is within
  // the noise of the (typed, rounded) coefficients: a double root stays a double root.
  const double bb = b * b, bbErr = std::fma(b, b, -bb);
  const double ac4 = 4 * a * c, acErr = std::fma(4 * a, c, -ac4);
  double D = (bb - ac4) + (bbErr - acErr);
  if (std::fabs(D) <= 16 * kEps * (bb + std::fabs(ac4))) D = 0;
  Root r1, r2;
  if (D >= 0) {
    const double sd = std::sqrt(D);
    const double q = -0.5 * (b + std::copysign(sd, b));  // no cancellation
    const double big = q / a, small = q != 0 ? c / q : 0.0;
    // X1 = (−b + √D) / 2a: q / a when b < 0 (q = (−b + √D) / 2), else c / q.
    r1.re = b < 0 ? big : small;
    r2.re = b < 0 ? small : big;
    if (D == 0) r1.re = r2.re = -b / (2 * a);
  } else {
    r1.re = r2.re = -b / (2 * a);
    const double im = std::sqrt(-D) / (2 * std::fabs(a));
    // Below 1E-12 of the roots' size: rounding noise, not a complex pair.
    if (im < 1e-12 * std::hypot(r1.re, im)) {
      r1.im = r2.im = 0;
    } else {
      r1.im = im;
      r2.im = -im;
    }
  }
  for (Root* r : {&r1, &r2}) {
    if (!inRange(r->re) || !inRange(r->im)) return Outcome::MathError;
    r->re = tidy(r->re);
    r->im = tidy(r->im);
  }
  out[0] = r1;
  count = 1;
  if (r2.re != r1.re || r2.im != r1.im) out[count++] = r2;
  if (xv) *xv = tidy(-b / (2 * a));
  if (yv) *yv = tidy(-D / (4 * a) * s);  // c − b²/4a = −D/4a, back in the original scale
  return Outcome::Ok;
}

namespace {
// Newton's method on the monic cubic x³ + px² + qx + r from x, keeping a step only while it
// lowers |f|. (Not for a multiple root, where f' vanishes: the closed form is better there.)
double polish(double x, double p, double q, double r) {
  auto f = [&](double t) { return ((t + p) * t + q) * t + r; };
  double fx = f(x);
  for (int i = 0; i < 12 && fx != 0; ++i) {
    const double d = (3 * x + 2 * p) * x + q;
    if (d == 0 || !std::isfinite(d)) break;
    const double nx = x - fx / d;
    const double fn = f(nx);
    if (!(std::fabs(fn) < std::fabs(fx))) break;
    x = nx;
    fx = fn;
  }
  return x;
}
}  // namespace

Outcome solveCubic(double a, double b, double c, double d, Root out[3], int& count) {
  count = 0;
  if (!inRange(a) || !inRange(b) || !inRange(c) || !inRange(d) || a == 0) return Outcome::MathError;
  const double s = pow2Scale(std::max(std::max(std::fabs(a), std::fabs(b)), std::max(std::fabs(c), std::fabs(d))));
  a /= s;
  b /= s;
  c /= s;
  d /= s;
  Root roots[4];
  int n = 0;
  if (d == 0) {
    // X = 0 is a root exactly: the rest is the quadratic aX² + bX + c.
    roots[n++] = Root{};
    Root qr[2];
    int k = 0;
    const Outcome o = solveQuadratic(a, b, c, qr, k);
    if (o != Outcome::Ok) return o;
    for (int i = 0; i < k; ++i) roots[n++] = qr[i];
  } else {
    const double p = b / a, q = c / a, r = d / a;
    // The depressed cubic t³ + Pt + Q (x = t − p/3), with the size of the terms each came from.
    const double p3 = p / 3;
    const double P = q - p * p3;
    const double Q = 2 * p3 * p3 * p3 - p3 * q + r;
    const double scaleP = std::fabs(q) + std::fabs(p * p3);
    const double scaleQ = 2 * std::fabs(p3 * p3 * p3) + std::fabs(p3 * q) + std::fabs(r);
    if (!std::isfinite(P) || !std::isfinite(Q) || !std::isfinite(scaleP) || !std::isfinite(scaleQ))
      return Outcome::MathError;
    constexpr double K = 256 * kEps;
    if (std::fabs(P) <= K * scaleP && std::fabs(Q) <= K * scaleQ) {
      roots[n++] = Root{-p3, 0};  // a triple root
    } else {
      const double Q2 = Q / 2, P3 = P / 3;
      const double D = Q2 * Q2 + P3 * P3 * P3;
      // Δ's rounding noise from P's and Q's: dΔ = (Q/2)·dQ + (P/3)²·dP.
      const double errD = K * (std::fabs(Q2) * scaleQ + P3 * P3 * scaleP);
      if (std::fabs(D) <= errD && P != 0) {
        // A double root: t = 3Q/P once, −3Q/2P twice.
        roots[n++] = Root{polish(3 * Q / P - p3, p, q, r), 0};
        roots[n++] = Root{-3 * Q / (2 * P) - p3, 0};
      } else if (D > 0) {
        // One real root (Cardano, the cube root taken where nothing cancels), polished; the
        // complex pair from the quadratic left after dividing it out (Vieta: the other two
        // add up to −p − x1 and multiply to −r/x1).
        const double w = -Q2 - std::copysign(std::sqrt(D), Q2);
        const double u = std::cbrt(w);
        const double v = u != 0 ? -P3 / u : 0.0;
        const double x1 = polish(u + v - p3, p, q, r);
        roots[n++] = Root{x1, 0};
        Root qr[2];
        int k = 0;
        if (x1 != 0 && solveQuadratic(1, p + x1, -r / x1, qr, k) == Outcome::Ok) {
          for (int i = 0; i < k; ++i) roots[n++] = qr[i];
        } else {
          const double im = std::sqrt(3.0) / 2 * std::fabs(u - v);
          roots[n++] = Root{-(u + v) / 2 - p3, im};
          roots[n++] = Root{-(u + v) / 2 - p3, -im};
        }
      } else {
        // Three real roots (the trigonometric form), each polished.
        const double m = 2 * std::sqrt(-P3);
        double arg = 3 * Q / (P * m);
        arg = arg > 1 ? 1 : (arg < -1 ? -1 : arg);
        const double th = std::acos(arg) / 3;
        for (int k = 0; k < 3; ++k) roots[n++] = Root{polish(m * std::cos(th - 2 * kPi * k / 3) - p3, p, q, r), 0};
      }
    }
  }
  // Snap, check, then the distinct roots: reals from largest to smallest, then the pair.
  double size = 0;
  for (int i = 0; i < n; ++i) size = std::max(size, std::hypot(roots[i].re, roots[i].im));
  for (int i = 0; i < n; ++i) {
    if (!inRange(roots[i].re) || !inRange(roots[i].im)) return Outcome::MathError;
    if (std::fabs(roots[i].im) < 1e-12 * size) roots[i].im = 0;
    roots[i].re = tidy(roots[i].re);
    roots[i].im = tidy(roots[i].im);
  }
  std::stable_sort(roots, roots + n, [](const Root& x, const Root& y) {
    const bool xr = x.im == 0, yr = y.im == 0;
    if (xr != yr) return xr;                // reals first
    if (xr) return x.re > y.re;             // largest first
    return x.im > y.im;                     // +im first
  });
  for (int i = 0; i < n; ++i) {
    bool dup = false;
    for (int j = 0; j < count && !dup; ++j)
      dup = std::fabs(out[j].re - roots[i].re) <= 1e-12 * size && std::fabs(out[j].im - roots[i].im) <= 1e-12 * size;
    if (!dup && count < 3) out[count++] = roots[i];
  }
  return Outcome::Ok;
}

std::string rootText(const Root& r, bool preferFraction, NormMode norm) {
  auto num = [&](double v) { return formatResult(Num::ofDouble(v), preferFraction, norm); };
  if (r.im == 0) return num(r.re);
  std::string s = r.re != 0 ? num(r.re) : std::string();
  if (r.im < 0) s += kMinus;
  else if (r.re != 0) s += "+";
  if (std::fabs(r.im) != 1) s += num(std::fabs(r.im));
  return s + "i";
}

}  // namespace eqn

// ============================================================================= the mode

void EqnApp::reset() {
  screen_ = Screen::TypeSelect;
  type_ = EqnType::Lin2;
  chosen_ = false;
  for (auto& row : coef_)
    for (double& v : row) v = 0;
  row_ = col_ = 0;
  typing_ = false;
  entry_.clear();
  outcome_ = eqn::Outcome::Ok;
  nResults_ = shown_ = 0;
  showFraction_ = true;
  message_ = "";
  exit_ = false;
}

void EqnApp::setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)()) {
  vars_ = vars;
  unit_ = unit;
  norm_ = norm;
  random_ = random;
}

Vars& EqnApp::vars() const { return vars_ ? *vars_ : ownVars_; }

int EqnApp::rowsOf(EqnType t) {
  switch (t) {
    case EqnType::Lin2: return 2;
    case EqnType::Lin3: return 3;
    default: return 1;
  }
}

int EqnApp::colsOf(EqnType t) { return t == EqnType::Lin3 || t == EqnType::Cubic ? 4 : 3; }

const char* EqnApp::columnName(int c) {
  static const char* const kNames[] = {"a", "b", "c", "d"};
  return c >= 0 && c < 4 ? kNames[c] : "";
}

void EqnApp::setType(EqnType t) {
  if (t >= EqnType::Count) return;
  if (!chosen_ || t != type_)
    for (auto& row : coef_)
      for (double& v : row) v = 0;
  type_ = t;
  chosen_ = true;
  row_ = col_ = 0;
  typing_ = false;
  entry_.clear();
  nResults_ = shown_ = 0;
  screen_ = Screen::Editor;
}

double EqnApp::coef(int r, int c) const {
  if (r < 0 || r >= kMaxRows || c < 0 || c >= kMaxCols) return 0;
  return coef_[r][c];
}

bool EqnApp::setCoef(int r, int c, double v) {
  if (r < 0 || r >= rows() || c < 0 || c >= cols() || !inRange(v)) return false;
  coef_[r][c] = tidy(v);
  return true;
}

std::string EqnApp::resultText(int i) const { return eqn::rootText(result(i).value, showFraction_, norm_); }

void EqnApp::addResult(const std::string& label, double re, double im) {
  if (nResults_ >= kMaxResults) return;
  results_[nResults_].label = label;
  results_[nResults_].value = eqn::Root{re, im};
  ++nResults_;
}

void EqnApp::showError(const char* msg) {
  message_ = msg;
  screen_ = Screen::Error;
}

bool EqnApp::solve() {
  typing_ = false;
  entry_.clear();
  nResults_ = shown_ = 0;
  showFraction_ = true;
  eqn::Outcome o = eqn::Outcome::MathError;
  switch (type_) {
    case EqnType::Lin2:
    case EqnType::Lin3: {
      const int n = rows();
      double a[12], x[3];
      for (int i = 0; i < n; ++i)
        for (int j = 0; j <= n; ++j) a[i * (n + 1) + j] = coef_[i][j];
      o = eqn::solveLinear(n, a, x);
      if (o == eqn::Outcome::Ok) {
        static const char* const kNames[] = {"X=", "Y=", "Z="};
        for (int j = 0; j < n; ++j) addResult(kNames[j], x[j]);
      }
      break;
    }
    case EqnType::Quad: {
      eqn::Root r[2];
      int k = 0;
      double xv = 0, yv = 0;
      o = eqn::solveQuadratic(coef_[0][0], coef_[0][1], coef_[0][2], r, k, &xv, &yv);
      if (o == eqn::Outcome::Ok) {
        for (int i = 0; i < k; ++i) addResult(k == 1 ? "X=" : (i == 0 ? "X1=" : "X2="), r[i].re, r[i].im);
        if (inRange(xv) && inRange(yv)) {
          const bool minimum = coef_[0][0] > 0;
          addResult(minimum ? "X-Value Minimum=" : "X-Value Maximum=", xv);
          addResult(minimum ? "Y-Value Minimum=" : "Y-Value Maximum=", yv);
        }
      }
      break;
    }
    case EqnType::Cubic: {
      eqn::Root r[3];
      int k = 0;
      o = eqn::solveCubic(coef_[0][0], coef_[0][1], coef_[0][2], coef_[0][3], r, k);
      if (o == eqn::Outcome::Ok) {
        static const char* const kNames[] = {"X1=", "X2=", "X3="};
        for (int i = 0; i < k; ++i) addResult(k == 1 ? "X=" : kNames[i], r[i].re, r[i].im);
      }
      break;
    }
    default: break;
  }
  outcome_ = o;
  switch (o) {
    case eqn::Outcome::Ok:
      screen_ = Screen::Result;
      return true;
    case eqn::Outcome::NoSolution:
      message_ = "No Solution";
      screen_ = Screen::Message;
      return false;
    case eqn::Outcome::Infinite:
      message_ = "Infinite Solution";
      screen_ = Screen::Message;
      return false;
    default:
      showError(errorText(CalcError::Math));
      return false;
  }
}

bool EqnApp::onKey(DKey k, bool shift, bool alpha) {
  if (k == DKey::Shift || k == DKey::Alpha || k == DKey::Mode || k == DKey::On || k >= DKey::Count) return false;
  switch (screen_) {
    case Screen::TypeSelect: keyTypeSelect(k); break;
    case Screen::Editor: keyEditor(k, shift, alpha); break;
    case Screen::Result: keyResult(k, alpha); break;
    case Screen::Message:
      if (k != DKey::AC && k != DKey::Eq && k != DKey::Left && k != DKey::Right) return false;
      screen_ = Screen::Editor;
      break;
    case Screen::Error:
      if (k == DKey::AC) {
        typing_ = false;
        entry_.clear();
      } else if (k != DKey::Left && k != DKey::Right) {
        return false;
      }
      screen_ = Screen::Editor;
      break;
  }
  return true;
}

void EqnApp::keyTypeSelect(DKey k) {
  const int d = digitOf(k);
  if (d >= 1 && d <= 4) {
    setType(static_cast<EqnType>(d - 1));
    return;
  }
  if (k == DKey::AC) {
    if (chosen_) screen_ = Screen::Editor;
    else exit_ = true;
  }
}

void EqnApp::keyEditor(DKey k, bool shift, bool alpha) {
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
    case DKey::Down: if (row_ + 1 < rows()) ++row_; return;
    case DKey::Left: if (col_ > 0) --col_; return;
    case DKey::Right: if (col_ + 1 < cols()) ++col_; return;
    case DKey::Del: coef_[row_][col_] = 0; return;
    case DKey::AC: screen_ = Screen::TypeSelect; return;
    case DKey::Eq: solve(); return;
    default: break;
  }
  if (modeKeyToken(k, shift, alpha, t)) {
    typing_ = true;
    entry_.assign(1, t);
  }
}

void EqnApp::commitEntry() {
  if (entry_.empty()) {
    typing_ = false;
    return;
  }
  Vars copy = vars();  // EQN never changes the calculator's memories or Ans
  const EvalResult r = calc::evaluate(entry_, copy, unit_, random_ ? random_ : fallbackRandom);
  if (r.error != CalcError::None) {
    showError(errorText(r.error));
    return;
  }
  if (!setCoef(row_, col_, r.value.v)) {
    showError(errorText(CalcError::Math));
    return;
  }
  typing_ = false;
  entry_.clear();
  // On to the next cell: right, then the next equation's first.
  if (col_ + 1 < cols()) {
    ++col_;
  } else if (row_ + 1 < rows()) {
    ++row_;
    col_ = 0;
  }
}

void EqnApp::keyResult(DKey k, bool alpha) {
  switch (k) {
    case DKey::Down:
      if (shown_ + 1 < nResults_) ++shown_;
      break;
    case DKey::Eq:
      if (shown_ + 1 < nResults_) ++shown_;
      else screen_ = Screen::Editor;  // = after the last: back to the coefficients (as the Casio)
      break;
    case DKey::Up:
      if (shown_ > 0) --shown_;
      break;
    case DKey::AC:
      screen_ = Screen::Editor;
      break;
    case DKey::SD:
      if (!alpha) showFraction_ = !showFraction_;
      break;
    default:
      break;
  }
}

// ============================================================================= render

void EqnApp::render(Framebuffer& fb) const {
  fb.setIcon(Icon::Up, false);
  fb.setIcon(Icon::Down, false);
  switch (screen_) {
    case Screen::TypeSelect:
      for (int i = 0; i < static_cast<int>(EqnType::Count); ++i)
        fb.drawText(0, 1 + i, std::to_string(i + 1) + ":" + eqnTypeName(static_cast<EqnType>(i)));
      break;
    case Screen::Editor: drawEditor(fb); break;
    case Screen::Result: drawResult(fb); break;
    case Screen::Message: fb.drawText(0, 1, message_); break;
    case Screen::Error: drawErrorScreen(fb, message_); break;
  }
}

void EqnApp::drawEditor(Framebuffer& fb) const {
  // Columns a b c (d) under their letters; four columns are 5 characters wide, three 6.
  const int nc = cols(), nr = rows();
  const int w = nc == 4 ? 5 : 6;
  auto colStart = [&](int c) { return nc == 4 ? 2 + c * 6 : 3 + c * 7; };
  for (int c = 0; c < nc; ++c) fb.drawText(colStart(c) + (w - 1) / 2, 1, columnName(c));
  for (int r = 0; r < nr; ++r) {
    const int row = 2 + r;
    if (nr > 1) fb.drawText(nc == 4 ? 0 : 1, row, std::to_string(r + 1));  // the equation's number
    for (int c = 0; c < nc; ++c) {
      const std::string s = cellText(coef_[r][c], w);
      fb.drawText(colStart(c) + w - static_cast<int>(s.size()), row, s);
      if (r == row_ && c == col_) fb.invertRect(colStart(c) * 5 - 1, row * 8, w * 5 + 1, 8);
    }
  }
  if (typing_) drawEntry(fb, 6, 1, exprText(entry_), true);
  else rightText(fb, 6, formatDecimal(coef_[row_][col_], norm_));
}

void EqnApp::drawResult(Framebuffer& fb) const {
  if (nResults_ == 0) return;
  const Result& res = result(shown_);
  fb.drawText(0, 1, res.label);
  const std::string t = resultText(shown_);
  const int n = textLength(t);
  if (n * 10 <= Framebuffer::kWidth) {
    fb.drawTextPx(Framebuffer::kWidth - n * 10 + 1, 46, t, 2);  // big digits, as COMP's result
  } else if (n <= kCols) {
    rightText(fb, 6, t);
  } else {
    // a+bi too long for one line: the real part, then the imaginary part under it.
    const eqn::Root re{res.value.re, 0}, im{0, res.value.im};
    rightText(fb, 5, eqn::rootText(re, showFraction_, norm_));
    std::string s = eqn::rootText(im, showFraction_, norm_);
    if (res.value.im > 0) s = "+" + s;
    rightText(fb, 6, s);
  }
  fb.setIcon(Icon::Up, shown_ > 0);
  fb.setIcon(Icon::Down, shown_ + 1 < nResults_);
}

}  // namespace calc
