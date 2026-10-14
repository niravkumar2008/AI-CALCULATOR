#include "numeric.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "font.h"
#include "text.h"

namespace calc {
namespace numeric {

namespace {
constexpr double kEps = std::numeric_limits<double>::epsilon();
const char* kMinusSign = "\xE2\x88\x92";  // −

bool finite(double v) { return std::isfinite(v); }
double defaultRandom() { return 0.5; }  // a platform that gave none: Ran# still works

// ---------------------------------------------------------------- Gauss–Kronrod (QUADPACK qk15)

const double kXgk[8] = {0.991455371120812639206854697526329, 0.949107912342758524526189684047851,
                        0.864864423359769072789712788640926, 0.741531185599394439863864773280788,
                        0.586087235467691130294144845693013, 0.405845151377397166906606412076961,
                        0.207784955007898467600689403773245, 0.000000000000000000000000000000000};
const double kWgk[8] = {0.022935322010529224963732008058970, 0.063092092629978553290700663189204,
                        0.104790010322250183839876322541518, 0.140653259715525918745189590510238,
                        0.169004726639267902826583426598550, 0.190350578064785409913256402421014,
                        0.204432940075298892414161999234649, 0.209482141084727828012999174891714};
const double kWg[4] = {0.129484966168869693270611432679082, 0.279705391489276667901467771423780,
                       0.381830050505118944950369775488975, 0.417959183673469387755102040816327};

struct Piece {
  double a, b, val, err, abs;
  bool fine;  // too narrow to split again
};

// bad: where f was undefined (NaN when the sums overflowed instead).
bool gk15(const RealFn& f, double a, double b, Piece& p, double& bad) {
  const double c = 0.5 * (a + b), h = 0.5 * (b - a);
  double fc;
  bad = c;
  if (!f(c, fc) || !finite(fc)) return false;
  double resg = fc * kWg[3], resk = fc * kWgk[7], resabs = std::fabs(resk);
  double f1[7], f2[7];
  for (int j = 0; j < 7; ++j) {
    const double x = h * kXgk[j];
    bad = c - x;
    if (!f(c - x, f1[j]) || !finite(f1[j])) return false;
    bad = c + x;
    if (!f(c + x, f2[j]) || !finite(f2[j])) return false;
    resk += kWgk[j] * (f1[j] + f2[j]);
    resabs += kWgk[j] * (std::fabs(f1[j]) + std::fabs(f2[j]));
    if (j % 2 == 1) resg += kWg[j / 2] * (f1[j] + f2[j]);
  }
  const double reskh = resk * 0.5;
  double resasc = kWgk[7] * std::fabs(fc - reskh);
  for (int j = 0; j < 7; ++j) resasc += kWgk[j] * (std::fabs(f1[j] - reskh) + std::fabs(f2[j] - reskh));
  const double ah = std::fabs(h);
  p.a = a;
  p.b = b;
  p.val = resk * h;
  p.abs = resabs * ah;
  resasc *= ah;
  double err = std::fabs((resk - resg) * h);
  if (resasc != 0 && err != 0) err = resasc * std::min(1.0, std::pow(200 * err / resasc, 1.5));
  if (p.abs > std::numeric_limits<double>::min() / (50 * kEps)) err = std::max(50 * kEps * p.abs, err);
  p.err = err;
  p.fine = false;
  bad = std::numeric_limits<double>::quiet_NaN();
  return finite(p.val) && finite(err);
}

// Adds [lo, hi] to ps. Where f is undefined at a node (a singularity inside the piece:
// ∫(ln Abs X,−1,1), ∫(1/√(Abs X),−1,1)), the piece is split at that point instead, so it
// becomes an end (ends are never evaluated). Undefined at more than kMaxBadSplits points in all
// (√X over −1..1), or the run was stopped: false (Domain).
constexpr int kMaxBadSplits = 8;
bool addPieces(const RealFn& f, double lo, double hi, std::vector<Piece>& ps, int& badSplits) {
  double los[kMaxBadSplits + 2], his[kMaxBadSplits + 2];
  int n = 0;
  los[n] = lo;
  his[n++] = hi;
  while (n > 0) {
    --n;
    const double l = los[n], h = his[n];
    Piece p;
    double bad;
    if (gk15(f, l, h, p, bad)) {
      ps.push_back(p);
      continue;
    }
    if (!(bad > l && bad < h) || ++badSplits > kMaxBadSplits || evalRunStopped()) return false;
    los[n] = bad;
    his[n++] = h;
    los[n] = l;
    his[n++] = bad;
  }
  return true;
}

// ---------------------------------------------------------------- Ridders

// The Richardson tableau is kept one column at a time (prev = column i-1, cur = column i):
// 160 bytes of stack instead of a 10 x 10 table's 800 (the ESP32's loop task is small).
bool ridders(const RealFn& f, double x, double h, double& ans, double& err) {
  constexpr int N = 10;
  constexpr double kCon = 1.4, kCon2 = kCon * kCon, kSafe = 2.0;
  double prev[N], cur[N];
  auto central = [&](double hh, double& v) {
    double p, m;
    if (!f(x + hh, p) || !f(x - hh, m)) return false;
    v = (p - m) / (2 * hh);
    return finite(v);
  };
  double hh = h;
  if (!central(hh, prev[0])) return false;
  ans = prev[0];
  err = std::numeric_limits<double>::max();
  for (int i = 1; i < N; ++i) {
    hh /= kCon;
    if (!central(hh, cur[0])) return false;
    double fac = kCon2;
    for (int j = 1; j <= i; ++j) {
      cur[j] = (cur[j - 1] * fac - prev[j - 1]) / (fac - 1);
      fac *= kCon2;
      const double errt = std::max(std::fabs(cur[j] - cur[j - 1]), std::fabs(cur[j] - prev[j - 1]));
      if (errt <= err) {
        err = errt;
        ans = cur[j];
      }
    }
    if (std::fabs(cur[i] - prev[i - 1]) >= kSafe * err) break;
    for (int j = 0; j <= i; ++j) prev[j] = cur[j];
  }
  return finite(ans);
}

// One-sided slope at x (dir = +1 / -1), Richardson-extrapolated to O(h³).
bool oneSided(const RealFn& f, double x, double fx, double h, int dir, double& out) {
  double d[3];
  for (int k = 0; k < 3; ++k) {
    const double hk = dir * h / (1 << k);
    double y;
    if (!f(x + hk, y)) return false;
    d[k] = (y - fx) / hk;
  }
  const double r1 = 2 * d[1] - d[0], r2 = 2 * d[2] - d[1];
  out = (4 * r2 - r1) / 3;
  return finite(out);
}

bool isVar(Tok t) { return t >= Tok::VarA && t <= Tok::VarM; }

// Index of the first top-level "," or ")" after a template's opening token at `open`
// (the end of f), or toks.size().
size_t templateArgEnd(const std::vector<Tok>& toks, size_t open) {
  int level = 0;
  size_t i = open + 1;
  for (; i < toks.size(); ++i) {
    const Tok c = toks[i];
    if (level == 0 && (c == Tok::Comma || c == Tok::Close)) break;
    if (tokOpensBracket(c)) ++level;
    else if (c == Tok::Close) --level;
  }
  return i;
}

// Evaluates token lists with one variable bound, remembering the first error that is not a
// Math ERROR (a Syntax ERROR in f is reported as such, not as "undefined here").
struct Bound {
  const std::vector<Tok>& f;
  Tok var;
  Vars v;
  AngleUnit unit;
  double (*rnd)();
  CalcError hard = CalcError::None;
  int hardPos = 0;
  Bound(const std::vector<Tok>& toks, Tok var_, const Vars& vars, AngleUnit u, double (*r)())
      : f(toks), var(var_), v(vars), unit(u), rnd(r ? r : defaultRandom) {}
  bool at(const std::vector<Tok>& toks, double x, double& y) {
    if (!evalRunStep()) return false;  // the run's budget is spent (or AC): undefined from now on
    v.get(var) = Num::ofDouble(x);
    const EvalResult r = evaluate(toks, v, unit, rnd);
    if (r.error != CalcError::None) {
      if (r.error != CalcError::Math && hard == CalcError::None) {
        hard = r.error;
        hardPos = r.errorPos;
      }
      return false;
    }
    y = r.value.v;
    return true;
  }
  bool operator()(double x, double& y) { return at(f, x, y); }
};

Num rootNum(double x) {
  if (std::fabs(x) < 1e15 && x == std::floor(x)) return Num::ofInt(static_cast<long long>(x));
  return Num::ofDouble(x);
}

std::string rightAlign(const std::string& label, const std::string& value) {
  int pad = Framebuffer::kCols - textLength(label) - textLength(value);
  if (pad < 1) pad = 1;
  return label + std::string(static_cast<size_t>(pad), ' ') + value;
}
}  // namespace

// ---------------------------------------------------------------- ∫

Status integrate(const RealFn& f, double a, double b, double tol, double& out, double* errEstimate) {
  out = 0;
  if (errEstimate) *errEstimate = 0;
  if (!finite(a) || !finite(b)) return Status::Domain;
  if (a == b) return Status::Ok;
  if (!(tol > 0)) tol = kIntegralTol;
  double sign = 1;
  if (a > b) {
    std::swap(a, b);
    sign = -1;
  }
  std::vector<Piece> ps;
  ps.reserve(kMaxIntervals + kMaxBadSplits + 2);
  int badSplits = 0;
  // Never accept a single piece's estimate: 4 equal pieces to start with, and over a wide range
  // also break points 1, 10, 100 ... away from 0 (or from the end nearer 0). One piece's 15
  // nodes can miss a narrow peak completely and "converge" on a wrong value (∫(1/(1+X²),−1E10,
  // 1E10) gave π/2); the graded pieces put nodes where the peak usually is.
  double cuts[32];  // 3 + 1 + 2 x 13 at most
  int nc = 0;
  for (int k = 1; k < 4; ++k) cuts[nc++] = a + (b - a) * 0.25 * k;
  if (b - a > 1e3) {
    const double o = a > 0 ? a : (b < 0 ? b : 0.0);
    if (o > a && o < b) cuts[nc++] = o;
    double s = 1;
    while (s < (b - a) * 1e-12) s *= 10;  // at most 13 a side
    for (; s < b - a && nc + 2 <= static_cast<int>(sizeof cuts / sizeof cuts[0]); s *= 10) {
      cuts[nc++] = o + s;
      cuts[nc++] = o - s;
    }
  }
  std::sort(cuts, cuts + nc);
  double lo = a;
  for (int k = 0; k <= nc; ++k) {
    const double hi = k < nc ? cuts[k] : b;
    if (!(hi > lo && hi <= b)) continue;  // outside (a, b) or a repeat
    if (k < nc && hi - lo <= 1e-13 * std::max(std::fabs(lo), std::fabs(hi))) continue;  // too close to keep
    if (!addPieces(f, lo, hi, ps, badSplits)) return Status::Domain;
    lo = hi;
  }
  for (;;) {
    double val = 0, err = 0, abs = 0;
    for (const Piece& p : ps) {
      val += p.val;
      err += p.err;
      abs += p.abs;
    }
    const double target = std::max({tol * std::fabs(val), 100 * kEps * abs, tol * 1e-6 * abs});
    if (err <= target) {
      out = std::fabs(val) <= 100 * kEps * abs ? 0 : sign * val;
      if (errEstimate) *errEstimate = err;
      return Status::Ok;
    }
    if (static_cast<int>(ps.size()) >= kMaxIntervals) break;
    int worst = -1;
    for (size_t i = 0; i < ps.size(); ++i)
      if (!ps[i].fine && (worst < 0 || ps[i].err > ps[static_cast<size_t>(worst)].err)) worst = static_cast<int>(i);
    if (worst < 0) break;
    Piece& w = ps[static_cast<size_t>(worst)];
    const double m = 0.5 * (w.a + w.b);
    if (!(m > w.a && m < w.b) || (w.b - w.a) <= 1e-13 * std::max(std::fabs(w.a), std::fabs(w.b))) {
      w.fine = true;
      continue;
    }
    const double wa = w.a, wb = w.b;
    ps[static_cast<size_t>(worst)] = ps.back();  // the order of the pieces doesn't matter
    ps.pop_back();
    if (!addPieces(f, wa, m, ps, badSplits) || !addPieces(f, m, wb, ps, badSplits)) return Status::Domain;
  }
  return Status::NoConverge;
}

// ---------------------------------------------------------------- d/dx

Status derivative(const RealFn& f, double x0, double tol, double& out) {
  out = 0;
  if (!finite(x0)) return Status::Domain;
  if (!(tol > 0)) tol = kDerivTol;
  double f0;
  if (!f(x0, f0) || !finite(f0)) return Status::Domain;
  // Ridders from a small first step (1E-3 of the scale: a step of 0.1·|x0| straddled whole
  // periods of cos X at 100), smaller while its error estimate is above tol, keeping the best.
  const double scale = std::max(1.0, std::fabs(x0));
  double h = 1e-3 * scale, d = 0, err = 0, hBest = h;
  bool ok = false;
  for (int attempt = 0; attempt < 10; ++attempt, h /= 8) {
    double dk, ek;
    if (!ridders(f, x0, h, dk, ek)) continue;  // undefined on one side at this step
    if (!ok || ek < err) {
      d = dk;
      err = ek;
      hBest = h;
    }
    ok = true;
    if (err <= tol * std::max(1.0, std::fabs(d))) break;
  }
  if (!ok) return Status::Domain;  // undefined on one side, however close (√X at 0)
  if (!(err <= std::max(tol, 1e-6) * std::max(1.0, std::fabs(d)))) return Status::NoConverge;
  // A corner (Abs(X) at 0): the central difference looks fine, the one-sided slopes don't agree
  // however close. Smooth functions' one-sided slopes agree once the step is small enough.
  double hk = hBest, fw = 0, bw = 0;
  bool sides = false, agree = false;
  for (int attempt = 0; attempt < 8 && !agree; ++attempt, hk /= 8) {
    if (!oneSided(f, x0, f0, hk, 1, fw) || !oneSided(f, x0, f0, hk, -1, bw)) continue;
    sides = true;
    agree = std::fabs(fw - bw) <= 1e-2 * std::max(1.0, std::fabs(d));
  }
  if (sides && !agree) return Status::NoConverge;
  // A tiny value (below the acceptance tolerance) that shrinks with the step (an 8 times
  // smaller step gives less than a quarter of it) is heading for 0: d/dx(X·Abs(X),0), whose
  // central difference is the step itself. One no bigger than its own error estimate is 0 too.
  double d2, err2;
  if (std::fabs(d) <= std::max(tol, 1e-6) && ridders(f, x0, hBest / 8, d2, err2) &&
      std::fabs(d2) < 0.25 * std::fabs(d))
    d = 0;
  out = std::fabs(d) <= 2 * err ? 0 : d;
  return Status::Ok;
}

// ---------------------------------------------------------------- SOLVE

Status solve(const EquationFn& eq, double x0, double& x, double& lr, int* iterations) {
  double L = 0, R = 0;
  auto G = [&](double t, double& g, double& l, double& r) {
    if (!finite(t) || !eq(t, l, r)) return false;
    g = l - r;
    return finite(g);
  };
  double g;
  x = x0;
  lr = 0;
  if (iterations) *iterations = 0;
  if (!G(x, g, L, R)) return Status::Domain;
  auto done = [&]() {
    lr = std::fabs(g) <= 1e-13 * std::max({1.0, std::fabs(L), std::fabs(R)}) ? 0 : g;
    return Status::Ok;
  };
  bool haveB = false, havePrev = false;
  double lo = 0, hi = 0, glo = 0, xp = 0, gp = 0;
  int kicks = 0;
  // Steps and convergence are relative to x itself (a root at 1E-15 is found to 10 digits, not
  // stopped at 9E-13 because the step got below 1E-12), down to a floor tied to the guess's size
  // (a root at exactly 0 still ends).
  const double s0 = std::max(1.0, std::fabs(x0));
  for (int it = 0; it < kSolveIterations; ++it) {
    if (iterations) *iterations = it + 1;
    if (g == 0) return done();
    const double scale = std::max(1.0, std::fabs(x));
    const double rel = std::max(std::fabs(x), 1e-12 * s0);
    if (haveB && hi - lo <= 1e-14 * rel) {  // bracket closed: a root, or a pole (1/X=0)
      if (std::fabs(g) <= 1e-6 * std::max({1.0, std::fabs(L), std::fabs(R)})) return done();
      return Status::NoConverge;
    }
    // Slope by central differences (one-sided at a domain edge), with a step relative to x (at
    // 1E-10 a step of 1E-9 swamped the slope of X³); where that sees no change (x = 0, e^X
    // there), again with the step relative to the guess's size.
    double d = std::numeric_limits<double>::quiet_NaN();
    for (int pass = 0; pass < 2 && !(finite(d) && d != 0); ++pass) {
      const double hd = 1e-6 * std::max(std::fabs(x), (pass == 0 ? 1e-12 : 1.0) * s0);
      if (pass == 1 && std::fabs(x) >= s0) break;  // the same step again
      double gpl, gmi, l2, r2;
      const bool okp = G(x + hd, gpl, l2, r2), okm = G(x - hd, gmi, l2, r2);
      if (okp && okm) d = (gpl - gmi) / (2 * hd);
      else if (okp) d = (gpl - g) / hd;
      else if (okm) d = (g - gmi) / hd;
    }
    double s = std::numeric_limits<double>::quiet_NaN();
    if (finite(d) && d != 0) s = g / d;
    if (!finite(s) && havePrev && gp != g) s = g * (x - xp) / (g - gp);  // secant
    if (finite(s) && std::fabs(s) <= 1e-12 * rel) {  // converged: one more step to polish
      double g2, L2, R2;
      if (G(x - s, g2, L2, R2) && std::fabs(g2) <= std::fabs(g)) {
        x -= s;
        g = g2;
        L = L2;
        R = R2;
      }
      return done();
    }
    bool explore = false, bisect = false;
    if (!finite(s)) {
      if (haveB) {
        s = x - 0.5 * (lo + hi);
        bisect = true;
      } else if (kicks < 10) {
        s = ((kicks % 2) ? 1.0 : -1.0) * 0.5 * scale * std::ldexp(1.0, kicks / 2);
        ++kicks;
        explore = true;
      } else {
        return Status::NoConverge;
      }
    }
    const double maxStep = 1e3 * scale;
    if (std::fabs(s) > maxStep) s = std::copysign(maxStep, s);
    double xn = x - s;
    if (haveB && !(xn > lo && xn < hi)) {
      xn = 0.5 * (lo + hi);
      s = x - xn;
      bisect = true;
    }
    // Damping: halve the step until |L−R| shrinks (and f is defined there).
    double gn = 0, Ln = 0, Rn = 0;
    bool ok = false;
    for (int tries = 0; tries < 40; ++tries) {
      if (G(xn, gn, Ln, Rn) && (explore || bisect || std::fabs(gn) < std::fabs(g))) {
        ok = true;
        break;
      }
      if (bisect) break;
      s *= 0.5;
      xn = x - s;
    }
    if (!ok && haveB && !bisect) {
      xn = 0.5 * (lo + hi);
      ok = G(xn, gn, Ln, Rn);
    }
    if (!ok && !haveB) {  // stuck in a dip that isn't a root: jump out, a few times
      while (!ok && kicks < 10) {
        const double k = ((kicks % 2) ? 1.0 : -1.0) * 0.5 * scale * std::ldexp(1.0, kicks / 2);
        ++kicks;
        xn = x + k;
        ok = G(xn, gn, Ln, Rn);
      }
    }
    if (!ok) return Status::NoConverge;
    // Keep a sign-change bracket once there is one.
    if ((gn < 0) != (g < 0) || gn == 0) {
      if (!haveB) {
        haveB = true;
        lo = std::min(x, xn);
        hi = std::max(x, xn);
        glo = x < xn ? g : gn;
      } else if (xn > lo && xn < hi) {
        if ((gn < 0) == (glo < 0)) {
          lo = xn;
          glo = gn;
        } else {
          hi = xn;
        }
      }
    } else if (haveB && xn > lo && xn < hi) {
      if ((gn < 0) == (glo < 0)) {
        lo = xn;
        glo = gn;
      } else {
        hi = xn;
      }
    }
    xp = x;
    gp = g;
    havePrev = true;
    x = xn;
    g = gn;
    L = Ln;
    R = Rn;
  }
  if (g == 0) return done();
  return Status::NoConverge;
}

// ---------------------------------------------------------------- token helpers

const char* varName(Tok v) {
  switch (v) {
    case Tok::VarA: return "A";
    case Tok::VarB: return "B";
    case Tok::VarC: return "C";
    case Tok::VarD: return "D";
    case Tok::VarE: return "E";
    case Tok::VarF: return "F";
    case Tok::VarX: return "X";
    case Tok::VarY: return "Y";
    case Tok::VarM: return "M";
    default: return "";
  }
}

EvalResult evalWith(const std::vector<Tok>& f, Tok var, double x, const Vars& vars, AngleUnit unit, double (*rnd)()) {
  Vars v = vars;
  if (isVar(var)) v.get(var) = Num::ofDouble(x);
  return evaluate(f, v, unit, rnd ? rnd : defaultRandom);
}

std::vector<Tok> variablesIn(const std::vector<Tok>& toks) {
  std::vector<bool> bound(toks.size(), false);
  for (size_t i = 0; i < toks.size(); ++i) {
    if (toks[i] != Tok::Integ && toks[i] != Tok::Deriv) continue;
    const size_t e = templateArgEnd(toks, i);
    for (size_t k = i + 1; k < e; ++k) bound[k] = true;
  }
  std::vector<Tok> out;
  for (size_t i = 0; i < toks.size(); ++i) {
    const Tok t = toks[i];
    if (!isVar(t) || (t == Tok::VarX && bound[i])) continue;
    if (std::find(out.begin(), out.end(), t) == out.end()) out.push_back(t);
  }
  return out;
}

bool splitEquation(const std::vector<Tok>& toks, std::vector<Tok>& lhs, std::vector<Tok>& rhs, bool& hasEq,
                   int& eqPos, int& errorPos) {
  lhs.clear();
  rhs.clear();
  hasEq = false;
  eqPos = -1;
  errorPos = 0;
  for (size_t i = 0; i < toks.size(); ++i) {
    if (toks[i] != Tok::Eq) continue;
    if (hasEq) {
      errorPos = static_cast<int>(i);
      return false;
    }
    hasEq = true;
    eqPos = static_cast<int>(i);
  }
  if (!hasEq) {
    lhs = toks;
    return !toks.empty();
  }
  lhs.assign(toks.begin(), toks.begin() + eqPos);
  rhs.assign(toks.begin() + eqPos + 1, toks.end());
  if (lhs.empty() || rhs.empty()) {
    errorPos = lhs.empty() ? 0 : eqPos + 1;
    return false;
  }
  return true;
}

EvalResult integrateExpr(const std::vector<Tok>& f, double a, double b, double tol, const Vars& vars,
                         AngleUnit unit, double (*rnd)()) {
  const EvalRun run;
  Bound fx(f, Tok::VarX, vars, unit, rnd);
  double out = 0;
  const Status st = integrate([&](double x, double& y) { return fx(x, y); }, a, b, tol, out);
  EvalResult r;
  if (fx.hard != CalcError::None) {
    r.error = fx.hard;
    r.errorPos = fx.hardPos;
  } else if (st != Status::Ok) {
    r.error = CalcError::Math;
  } else {
    r.value = Num::ofDouble(out);
  }
  return r;
}

EvalResult derivativeExpr(const std::vector<Tok>& f, double x0, double tol, const Vars& vars, AngleUnit unit,
                          double (*rnd)()) {
  const EvalRun run;
  Bound fx(f, Tok::VarX, vars, unit, rnd);
  double out = 0;
  const Status st = derivative([&](double x, double& y) { return fx(x, y); }, x0, tol, out);
  EvalResult r;
  if (fx.hard != CalcError::None) {
    r.error = fx.hard;
    r.errorPos = fx.hardPos;
  } else if (st != Status::Ok) {
    r.error = CalcError::Math;
  } else {
    r.value = Num::ofDouble(out);
  }
  return r;
}

EvalResult evalTemplate(Tok kind, const std::vector<Tok>& f, double a, double b, double tol, const Vars& vars,
                        AngleUnit unit, double (*rnd)()) {
  if (kind == Tok::Integ) return integrateExpr(f, a, b, tol, vars, unit, rnd);
  if (kind == Tok::Deriv) return derivativeExpr(f, a, tol, vars, unit, rnd);
  EvalResult r;
  r.error = CalcError::Syntax;
  return r;
}

SolveOutcome solveExpr(const std::vector<Tok>& equation, Tok var, double guess, const Vars& vars, AngleUnit unit,
                       double (*rnd)()) {
  const EvalRun run;  // one budget for the whole search (and every ∫ in the equation)
  SolveOutcome o;
  std::vector<Tok> lhs, rhs;
  bool hasEq;
  int eqPos, errPos;
  if (!splitEquation(equation, lhs, rhs, hasEq, eqPos, errPos)) {
    o.error = CalcError::Syntax;
    o.errorPos = errPos;
    return o;
  }
  Bound side(lhs, var, vars, unit, rnd);
  // Check both sides at the guess first, so their own errors (Syntax, Math) show where they are.
  for (int k = 0; k < (hasEq ? 2 : 1); ++k) {
    const EvalResult r = evalWith(k == 0 ? lhs : rhs, var, guess, vars, unit, rnd);
    if (evalRunStopped()) {  // an ∫ in it ran out of time (or AC) already
      o.kind = SolveOutcome::CantSolve;
      return o;
    }
    if (r.error != CalcError::None) {
      o.error = r.error;
      o.errorPos = r.errorPos + (k == 0 ? 0 : eqPos + 1);
      return o;
    }
  }
  const Status st = solve(
      [&](double x, double& l, double& r) {
        if (!side.at(lhs, x, l)) return false;
        r = 0;
        return !hasEq || side.at(rhs, x, r);
      },
      guess, o.x, o.lr, &o.iterations);
  if (evalRunStopped()) {  // out of time (or AC): "Can't Solve", whatever the last step saw
    o.kind = SolveOutcome::CantSolve;
  } else if (st == Status::Ok) {
    o.kind = SolveOutcome::Solved;
  } else if (st == Status::Domain) {
    o.error = CalcError::Math;
  } else {
    o.kind = SolveOutcome::CantSolve;
  }
  return o;
}

// ---------------------------------------------------------------- runners

bool RunnerBase::startCommon(const std::vector<Tok>& expr, Vars& vars, AngleUnit unit, NormMode norm,
                             double (*rnd)()) {
  started_ = false;
  exit_ = Exit::None;
  if (expr.empty()) return false;
  expr_ = expr;
  vars_ = &vars;
  unit_ = unit;
  norm_ = norm;
  rnd_ = rnd ? rnd : defaultRandom;
  started_ = true;
  gotoPos_ = 0;
  input_.clear();
  inputCursor_ = 0;
  phase_ = Phase::Prompt;
  return true;
}

void RunnerBase::beginPrompt(Tok var) {
  promptVar_ = var;
  input_.clear();
  inputCursor_ = 0;
  phase_ = Phase::Prompt;
}

void RunnerBase::insert(Tok t) {
  if (!active() || phase_ != Phase::Prompt || t == Tok::Count) return;
  if (static_cast<int>(input_.size()) >= kMaxTokens) return;
  input_.insert(input_.begin() + inputCursor_, t);
  ++inputCursor_;
}

bool RunnerBase::acceptPrompt() {
  if (input_.empty()) return true;  // = alone keeps the value shown
  const EvalResult r = evaluate(input_, *vars_, unit_, rnd_);
  if (r.error != CalcError::None) {
    fail(r.error, r.errorPos, true);
    return false;
  }
  vars_->get(promptVar_) = r.value;
  return true;
}

void RunnerBase::fail(CalcError e, int pos, bool inPrompt, const char* text) {
  phase_ = Phase::Error;
  errorText_ = text ? text : errorText(e);
  errorInPrompt_ = inPrompt;
  errorPos_ = pos;
}

void RunnerBase::storeAns(const Num& v) {
  if (!updatesAns_) return;
  vars_->preAns = vars_->ans;
  vars_->ans = v;
}

bool RunnerBase::keyCommon(DKey k) {
  const int n = static_cast<int>(expr_.size());
  switch (phase_) {
    case Phase::Error:
      if (k == DKey::AC) {
        exit_ = Exit::Done;
      } else if (k == DKey::Left || k == DKey::Right) {
        if (errorInPrompt_) {  // back to the typed value, cursor where it went wrong
          phase_ = Phase::Prompt;
          const int m = static_cast<int>(input_.size());
          inputCursor_ = errorPos_ < 0 ? 0 : (errorPos_ > m ? m : errorPos_);
        } else {
          exit_ = Exit::Goto;
          gotoPos_ = errorPos_ < 0 ? 0 : (errorPos_ > n ? n : errorPos_);
        }
      }
      return true;  // nothing else does anything on an error screen
    case Phase::Prompt:
      if (k == DKey::AC) {
        exit_ = Exit::Done;
      } else if (k == DKey::Del) {
        if (inputCursor_ > 0) {
          input_.erase(input_.begin() + inputCursor_ - 1);
          --inputCursor_;
        }
      } else if (k == DKey::Left) {
        if (inputCursor_ > 0) --inputCursor_;
      } else if (k == DKey::Right) {
        if (inputCursor_ < static_cast<int>(input_.size())) ++inputCursor_;
      } else {
        return false;
      }
      return true;
    case Phase::Result:
      if (k == DKey::AC) {
        exit_ = Exit::Done;
      } else if (k == DKey::Left || k == DKey::Right) {  // edit: ◀ cursor at the end, ▶ at the start
        exit_ = Exit::Goto;
        gotoPos_ = k == DKey::Left ? n : 0;
      } else {
        return false;
      }
      return true;
  }
  return false;
}

std::string RunnerBase::fmt(const Num& n) const { return formatResult(n, true, norm_); }
std::string RunnerBase::fmt(double v) const { return formatDecimal(v, norm_); }

void RunnerBase::promptScreen(Screen& s, const std::string& title, const std::string& subtitle) const {
  s.rows[0] = title;
  if (!subtitle.empty()) s.rows[1] = subtitle;
  if (input_.empty()) {
    s.big = fmt(vars_->get(promptVar_));
    return;
  }
  // The typed value on the bottom row, scrolled so the cursor shows.
  std::vector<uint32_t> cps;
  int cursorCell = 0;
  for (size_t i = 0; i <= input_.size(); ++i) {
    if (static_cast<int>(i) == inputCursor_) cursorCell = static_cast<int>(cps.size());
    if (i == input_.size()) break;
    for (uint32_t cp : decodeUtf8(tokText(input_[i]))) cps.push_back(cp);
  }
  int first = cursorCell - (Framebuffer::kCols - 1);
  if (first < 0) first = 0;
  const size_t a = static_cast<size_t>(first);
  const size_t b = std::min(cps.size(), a + static_cast<size_t>(Framebuffer::kCols));
  s.rows[5] = encodeUtf8(std::vector<uint32_t>(cps.begin() + static_cast<std::ptrdiff_t>(a),
                                               cps.begin() + static_cast<std::ptrdiff_t>(b)));
  s.cursorRow = 6;
  s.cursorCol = cursorCell - first;
}

void RunnerBase::errorScreen(Screen& s) const {
  s.rows[0] = "  " + errorText_;
  s.rows[2] = "  [AC]  :Cancel";
  s.rows[3] = "  [\xE2\x97\x80][\xE2\x96\xB6]:Goto";
}

void RunnerBase::exprRows(Screen& s, int maxRows) const {
  const std::vector<uint32_t> cps = decodeUtf8(exprText(expr_));
  for (int r = 0; r < maxRows; ++r) {
    const size_t a = static_cast<size_t>(r * Framebuffer::kCols);
    if (a >= cps.size()) break;
    const size_t b = std::min(cps.size(), a + static_cast<size_t>(Framebuffer::kCols));
    s.rows[r] = encodeUtf8(std::vector<uint32_t>(cps.begin() + static_cast<std::ptrdiff_t>(a),
                                                 cps.begin() + static_cast<std::ptrdiff_t>(b)));
  }
}

void RunnerBase::render(Framebuffer& fb) const {
  if (!started_) return;
  const Screen s = screen();
  for (int r = 0; r < 6; ++r)
    if (!s.rows[r].empty()) fb.drawText(0, r + 1, s.rows[r]);
  if (s.cursorRow >= 1 && s.cursorRow <= 6) {
    const int col = s.cursorCol;
    fb.fillRect(col == 0 ? 0 : col * 5 - 1, s.cursorRow * 8, 1, 7, true);
  }
  if (!s.big.empty()) {
    const int n = textLength(s.big);
    if (n * 10 <= Framebuffer::kWidth) fb.drawTextPx(Framebuffer::kWidth - n * 10 + 1, 46, s.big, 2);
    else fb.drawTextPx(Framebuffer::kWidth - n * 5 + 1, 52, s.big, 1);
  }
}

// ---- CALC

bool CalcRunner::start(const std::vector<Tok>& expr, Vars& vars, AngleUnit unit, NormMode norm, double (*rnd)()) {
  if (!startCommon(expr, vars, unit, norm, rnd)) return false;
  list_.clear();
  idx_ = 0;
  assignTo_ = Tok::Count;
  std::vector<Tok> lhs, rhs;
  bool hasEq;
  int eqPos, errPos;
  if (!splitEquation(expr, lhs, rhs, hasEq, eqPos, errPos)) {
    fail(CalcError::Syntax, errPos, false);
    return true;
  }
  if (hasEq) {  // only "variable = expression" can be calculated
    if (lhs.size() != 1 || !isVar(lhs[0])) {
      fail(CalcError::Syntax, eqPos, false);
      return true;
    }
    assignTo_ = lhs[0];
    body_ = rhs;
    bodyOffset_ = eqPos + 1;
  } else {
    body_ = expr;
    bodyOffset_ = 0;
  }
  list_ = variablesIn(body_);
  next();
  return true;
}

void CalcRunner::next() {
  if (idx_ < list_.size()) return beginPrompt(list_[idx_]);
  const EvalResult r = evaluate(body_, *vars_, unit_, rnd_);
  if (r.error != CalcError::None) return fail(r.error, bodyOffset_ + r.errorPos, false);
  result_ = r.value;
  if (assignTo_ != Tok::Count) vars_->get(assignTo_) = result_;
  storeAns(result_);
  phase_ = Phase::Result;
}

void CalcRunner::onKey(DKey k, bool shift) {
  if (!active() || keyCommon(k)) return;
  const bool go = k == DKey::Eq || (k == DKey::Calc && !shift);
  if (!go) return;
  if (phase_ == Phase::Prompt) {
    if (!acceptPrompt()) return;
    ++idx_;
    next();
  } else if (phase_ == Phase::Result) {  // the Casio loop: prompt again from the first variable
    idx_ = 0;
    next();
  }
}

Screen CalcRunner::screen() const {
  Screen s;
  if (!started_) return s;
  if (phase_ == Phase::Error) errorScreen(s);
  else if (phase_ == Phase::Prompt) promptScreen(s, std::string(varName(promptVar_)) + "?", "");
  else {
    exprRows(s, 4);
    s.big = fmt(result_);
  }
  return s;
}

// ---- SOLVE

bool SolveRunner::start(const std::vector<Tok>& expr, Vars& vars, AngleUnit unit, NormMode norm, double (*rnd)()) {
  updatesAns_ = false;  // the root goes into its variable; Ans stays (setUpdatesAns after start to change)
  if (!startCommon(expr, vars, unit, norm, rnd)) return false;
  others_.clear();
  idx_ = 0;
  std::vector<Tok> lhs, rhs;
  bool hasEq;
  int eqPos, errPos;
  if (!splitEquation(expr, lhs, rhs, hasEq, eqPos, errPos)) {
    fail(CalcError::Syntax, errPos, false);
    return true;
  }
  const std::vector<Tok> vs = variablesIn(expr);
  if (vs.empty()) {  // nothing to solve for
    fail(CalcError::Syntax, 0, false);
    return true;
  }
  solveVar_ = std::find(vs.begin(), vs.end(), Tok::VarX) != vs.end() ? Tok::VarX : vs[0];
  for (Tok v : vs)
    if (v != solveVar_) others_.push_back(v);
  next();
  return true;
}

void SolveRunner::next() { beginPrompt(idx_ < others_.size() ? others_[idx_] : solveVar_); }

void SolveRunner::run() {
  const SolveOutcome o = solveExpr(expr_, solveVar_, vars_->get(solveVar_).v, *vars_, unit_, rnd_);
  if (o.kind == SolveOutcome::Error) return fail(o.error, o.errorPos, false);
  if (o.kind == SolveOutcome::CantSolve) return fail(CalcError::Math, 0, false, "Can't Solve");
  x_ = o.x;
  lr_ = o.lr;
  const Num root = rootNum(x_);
  vars_->get(solveVar_) = root;
  storeAns(root);
  phase_ = Phase::Result;
}

void SolveRunner::onKey(DKey k, bool /*shift*/) {
  if (!active() || keyCommon(k)) return;
  if (k != DKey::Eq && k != DKey::Calc) return;
  if (phase_ == Phase::Prompt) {
    if (!acceptPrompt()) return;
    if (idx_ < others_.size()) {
      ++idx_;
      next();
    } else {
      run();
    }
  } else if (phase_ == Phase::Result) {  // solve again: prompts again, the root as the new guess
    idx_ = 0;
    next();
  }
}

Screen SolveRunner::screen() const {
  Screen s;
  if (!started_) return s;
  const std::string name = varName(solveVar_);
  if (phase_ == Phase::Error) {
    errorScreen(s);
  } else if (phase_ == Phase::Prompt) {
    const bool forRoot = idx_ >= others_.size();
    promptScreen(s, std::string(varName(promptVar_)) + "?", forRoot ? "Solve for " + name : "");
  } else {
    exprRows(s, 3);
    s.rows[4] = rightAlign(name + "=", fmt(rootNum(x_)));
    s.rows[5] = rightAlign(std::string("L") + kMinusSign + "R=", fmt(lr_));
  }
  return s;
}

}  // namespace numeric
}  // namespace calc
