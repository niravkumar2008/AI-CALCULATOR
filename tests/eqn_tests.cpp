// Unit tests for EQN (core/eqn.*, MODE 5): the solvers, the coefficient editor, the screens,
// and the Device integration (MODE menu, exam mode, saveState()). runEqnTests() returns the
// number of failures and prints one line per failure; tests.cpp counts them in its total.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include "../core/device.h"
#include "../core/eqn.h"
#include "../core/font.h"
#include "../core/framebuffer.h"
#include "../core/text.h"

using namespace calc;
using eqn::Outcome;
using eqn::Root;

namespace {

int g_fail = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
  ++g_checks;
  if (!ok) {
    ++g_fail;
    std::printf("FAIL eqn_tests.cpp:%d: %s\n", line, what);
  }
}
#define ECHECK(cond) check((cond), #cond, __LINE__)

bool near(double a, double b, double tol = 1e-12) { return std::fabs(a - b) <= tol * (1 + std::fabs(b)); }
const std::string kMinus = "\xE2\x88\x92";

// ---------------------------------------------------------------- golden screens
std::string readFile(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}
void snapshot(const std::string& name, const Device& d) {
  Framebuffer fb;
  d.render(fb);
  const std::string got = fb.toAscii();
  const std::string path = "tests/golden/" + name + ".txt";
  if (std::getenv("UPDATE_GOLDEN")) std::ofstream(path, std::ios::binary) << got;
  const bool same = readFile(path) == got;
  if (!same) std::printf("snapshot mismatch: %s\n", name.c_str());
  ECHECK(same);
}

// ---------------------------------------------------------------- linear systems
Outcome lin2(double a1, double b1, double c1, double a2, double b2, double c2, double* x) {
  const double m[] = {a1, b1, c1, a2, b2, c2};
  return eqn::solveLinear(2, m, x);
}

void testLinear() {
  double x[3];
  ECHECK(lin2(1, 1, 3, 1, -1, 1, x) == Outcome::Ok && x[0] == 2 && x[1] == 1);
  // Decimal coefficients: exactly 1 and 1 on the screen (no 0.9999999999 or 1E-17).
  ECHECK(lin2(0.1, 0.2, 0.3, 0.3, -0.1, 0.2, x) == Outcome::Ok && near(x[0], 1) && near(x[1], 1));
  ECHECK(formatDecimal(x[0], NormMode::Norm1) == "1" && formatDecimal(x[1], NormMode::Norm1) == "1");
  // A zero to pivot past.
  ECHECK(lin2(0, 1, 1, 1, 0, 2, x) == Outcome::Ok && x[0] == 2 && x[1] == 1);
  // A result that is 0 stays exactly 0 (no -0, no noise).
  ECHECK(lin2(0.1, 0.7, 0.1, 0.3, 0.2, 0.3, x) == Outcome::Ok && near(x[0], 1) && x[1] == 0 && !std::signbit(x[1]));
  // Singular: inconsistent and dependent.
  ECHECK(lin2(1, 1, 1, 2, 2, 3, x) == Outcome::NoSolution);
  ECHECK(lin2(1, 1, 1, 2, 2, 2, x) == Outcome::Infinite);
  ECHECK(lin2(0.1, 0.2, 0.3, 0.3, 0.6, 0.9, x) == Outcome::Infinite);  // decimal rounding: still dependent
  ECHECK(lin2(0, 0, 0, 0, 0, 0, x) == Outcome::Infinite);
  ECHECK(lin2(0, 0, 5, 0, 0, 0, x) == Outcome::NoSolution);
  ECHECK(lin2(0, 0, 0, 1, 1, 2, x) == Outcome::Infinite);
  // 3 unknowns.
  {
    const double m[] = {2, 1, -1, 8, -3, -1, 2, -11, -2, 1, 2, -3};
    ECHECK(eqn::solveLinear(3, m, x) == Outcome::Ok && near(x[0], 2) && near(x[1], 3) && near(x[2], -1));
  }
  {
    const double m[] = {1, 2, 3, 6, 2, 5, 2, 4, 3, 7, 5, 10};  // row 3 = row 1 + row 2
    ECHECK(eqn::solveLinear(3, m, x) == Outcome::Infinite);
    const double n[] = {1, 2, 3, 6, 2, 5, 2, 4, 3, 7, 5, 11};
    ECHECK(eqn::solveLinear(3, n, x) == Outcome::NoSolution);
    const double z[] = {1, 1, 1, 3, 2, 2, 2, 6, 3, 3, 3, 9};  // rank 1
    ECHECK(eqn::solveLinear(3, z, x) == Outcome::Infinite);
  }
  {  // the 3x3 Hilbert matrix (condition number ~500), solution (1, 2, 3)
    const double h[3][3] = {{1, 1.0 / 2, 1.0 / 3}, {1.0 / 2, 1.0 / 3, 1.0 / 4}, {1.0 / 3, 1.0 / 4, 1.0 / 5}};
    double m[12];
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) m[i * 4 + j] = h[i][j];
      m[i * 4 + 3] = h[i][0] + 2 * h[i][1] + 3 * h[i][2];
    }
    ECHECK(eqn::solveLinear(3, m, x) == Outcome::Ok && near(x[0], 1, 1e-11) && near(x[1], 2, 1e-11) && near(x[2], 3, 1e-11));
  }
  // Scaling: huge and tiny equations, and unknowns of very different sizes.
  ECHECK(lin2(1e50, 1e50, 2e50, 1, -1, 0, x) == Outcome::Ok && near(x[0], 1) && near(x[1], 1));
  ECHECK(lin2(1e-60, 2e-60, 5e-60, 3e-60, -1e-60, 1e-60, x) == Outcome::Ok && near(x[0], 1) && near(x[1], 2));
  ECHECK(lin2(1e30, 2e-30, 3, 2e30, -1e-30, 1, x) == Outcome::Ok && near(x[0], 1e-30) && near(x[1], 1e30));
  ECHECK(lin2(1e-90, 1e-90, 1e-90, 1e-90, -1e-90, 1e-90, x) == Outcome::Ok && near(x[0], 1) && x[1] == 0);
  // Out of range: Math ERROR.
  ECHECK(lin2(1e-99, 0, 1e99, 0, 1, 1, x) == Outcome::MathError);
  ECHECK(lin2(NAN, 1, 1, 1, 1, 1, x) == Outcome::MathError);
  ECHECK(lin2(1e100, 1, 1, 1, 1, 1, x) == Outcome::MathError);
  double bad[3];
  const double m4[] = {1, 2, 3};
  ECHECK(eqn::solveLinear(1, m4, bad) == Outcome::MathError);
  // Nearly singular but regular: still solved (pivot well above 1E-12).
  ECHECK(lin2(1, 1, 2, 1, 1.0000001, 2.0000001, x) == Outcome::Ok && near(x[0], 1, 1e-6) && near(x[1], 1, 1e-6));
  // Random systems: the residual is rounding noise.
  unsigned seed = 12345;
  auto rnd = [&]() {
    seed = seed * 1103515245u + 12345u;
    return static_cast<double>((seed >> 8) % 20001) / 1000.0 - 10.0;
  };
  int bad3 = 0;
  for (int t = 0; t < 300; ++t) {
    double m[12];
    for (double& v : m) v = rnd();
    if (eqn::solveLinear(3, m, x) != Outcome::Ok) continue;
    for (int i = 0; i < 3; ++i) {
      const double r = m[i * 4] * x[0] + m[i * 4 + 1] * x[1] + m[i * 4 + 2] * x[2] - m[i * 4 + 3];
      const double mag = std::fabs(m[i * 4] * x[0]) + std::fabs(m[i * 4 + 1] * x[1]) + std::fabs(m[i * 4 + 2] * x[2]) +
                         std::fabs(m[i * 4 + 3]);
      if (std::fabs(r) > 1e-10 * mag) ++bad3;
    }
  }
  ECHECK(bad3 == 0);
}

// ---------------------------------------------------------------- quadratics
void testQuadratic() {
  Root r[2];
  int n = 0;
  double xv = 0, yv = 0;
  ECHECK(eqn::solveQuadratic(1, -3, 2, r, n, &xv, &yv) == Outcome::Ok && n == 2 && r[0].re == 2 && r[1].re == 1 &&
         r[0].im == 0);
  ECHECK(xv == 1.5 && yv == -0.25);
  // a < 0: X1 = (−b + √D) / 2a is the smaller one, as on the Casio.
  ECHECK(eqn::solveQuadratic(-1, 3, -2, r, n, &xv, &yv) == Outcome::Ok && n == 2 && r[0].re == 1 && r[1].re == 2);
  ECHECK(xv == 1.5 && yv == 0.25);
  // Double roots, also from decimal coefficients (no spurious complex pair).
  ECHECK(eqn::solveQuadratic(1, -2, 1, r, n, &xv, &yv) == Outcome::Ok && n == 1 && r[0].re == 1 && r[0].im == 0 && yv == 0);
  ECHECK(eqn::solveQuadratic(1, -0.2, 0.01, r, n) == Outcome::Ok && n == 1 && near(r[0].re, 0.1) && r[0].im == 0);
  ECHECK(eqn::solveQuadratic(9, -6, 1, r, n) == Outcome::Ok && n == 1 && near(r[0].re, 1.0 / 3));
  ECHECK(eqn::solveQuadratic(1, 0, 0, r, n) == Outcome::Ok && n == 1 && r[0].re == 0 && !std::signbit(r[0].re));
  // Complex pairs (+im first).
  ECHECK(eqn::solveQuadratic(1, 2, 5, r, n) == Outcome::Ok && n == 2 && r[0].re == -1 && r[0].im == 2 && r[1].im == -2);
  ECHECK(eqn::rootText(r[0], true, NormMode::Norm1) == kMinus + "1+2i");
  ECHECK(eqn::rootText(r[1], true, NormMode::Norm1) == kMinus + "1" + kMinus + "2i");
  ECHECK(eqn::solveQuadratic(1, 0, 1, r, n) == Outcome::Ok && n == 2 && r[0].re == 0 && r[0].im == 1);
  ECHECK(eqn::rootText(r[0], true, NormMode::Norm1) == "i" && eqn::rootText(r[1], true, NormMode::Norm1) == kMinus + "i");
  ECHECK(eqn::solveQuadratic(-2, 0, -8, r, n) == Outcome::Ok && n == 2 && r[0].re == 0 && r[0].im == 2);
  ECHECK(eqn::solveQuadratic(1, 1, 1, r, n) == Outcome::Ok && near(r[0].re, -0.5) && near(r[0].im, std::sqrt(3.0) / 2));
  ECHECK(eqn::rootText(r[0], false, NormMode::Norm1) == kMinus + "0.5+0.8660254038i");
  ECHECK(eqn::rootText(r[0], true, NormMode::Norm1) == kMinus + "1\xE2\x8C\x9F" "2+0.8660254038i");
  // b = 0, c = 0.
  ECHECK(eqn::solveQuadratic(1, 0, -4, r, n) == Outcome::Ok && n == 2 && r[0].re == 2 && r[1].re == -2);
  ECHECK(eqn::solveQuadratic(1, -5, 0, r, n) == Outcome::Ok && n == 2 && r[0].re == 5 && r[1].re == 0);
  // Cancellation: the small root keeps its digits.
  ECHECK(eqn::solveQuadratic(1, -1e8, 1, r, n) == Outcome::Ok && near(r[0].re, 1e8) && near(r[1].re, 1e-8, 1e-14));
  // Scaling.
  ECHECK(eqn::solveQuadratic(1e90, -3e90, 2e90, r, n, &xv, &yv) == Outcome::Ok && near(r[0].re, 2) && near(r[1].re, 1) &&
         near(yv, -0.25e90));
  ECHECK(eqn::solveQuadratic(1e-90, -3e-90, 2e-90, r, n) == Outcome::Ok && near(r[0].re, 2) && near(r[1].re, 1));
  ECHECK(eqn::solveQuadratic(1e-50, 0, -1e50, r, n) == Outcome::Ok && near(r[0].re, 1e50) && near(r[1].re, -1e50));
  // Errors: a = 0, out of range.
  ECHECK(eqn::solveQuadratic(0, 1, 1, r, n) == Outcome::MathError);
  ECHECK(eqn::solveQuadratic(1e-99, 1e99, 1, r, n) == Outcome::MathError);  // a root near -1E198
  ECHECK(eqn::solveQuadratic(INFINITY, 1, 1, r, n) == Outcome::MathError);
}

// ---------------------------------------------------------------- cubics
double resid(double a, double b, double c, double d, const Root& r) {  // |f(root)| / the terms' size
  // complex evaluation by Horner
  double pr = a, pi = 0;
  for (double k : {b, c, d}) {
    const double nr = pr * r.re - pi * r.im + k, ni = pr * r.im + pi * r.re;
    pr = nr;
    pi = ni;
  }
  const double m = std::hypot(r.re, r.im);
  const double mag = std::fabs(a) * m * m * m + std::fabs(b) * m * m + std::fabs(c) * m + std::fabs(d);
  return std::hypot(pr, pi) / (mag > 0 ? mag : 1);
}

void testCubic() {
  Root r[3];
  int n = 0;
  ECHECK(eqn::solveCubic(1, -6, 11, -6, r, n) == Outcome::Ok && n == 3 && near(r[0].re, 3) && near(r[1].re, 2) &&
         near(r[2].re, 1) && r[0].im == 0 && r[1].im == 0 && r[2].im == 0);
  // One real root and a complex pair.
  ECHECK(eqn::solveCubic(1, 0, 0, -1, r, n) == Outcome::Ok && n == 3 && near(r[0].re, 1) && r[0].im == 0 &&
         near(r[1].re, -0.5) && near(r[1].im, std::sqrt(3.0) / 2) && near(r[2].im, -std::sqrt(3.0) / 2));
  ECHECK(eqn::rootText(r[1], false, NormMode::Norm1) == kMinus + "0.5+0.8660254038i");
  ECHECK(eqn::solveCubic(1, 0, 1, 0, r, n) == Outcome::Ok && n == 3 && r[0].re == 0 && r[0].im == 0 &&
         r[1].re == 0 && r[1].im == 1 && r[2].im == -1);  // x³ + x: 0, ±i
  ECHECK(eqn::solveCubic(1, -1, 1, -1, r, n) == Outcome::Ok && n == 3 && near(r[0].re, 1) && near(r[1].im, 1) &&
         std::fabs(r[1].re) < 1e-15);  // (x−1)(x²+1)
  // Triple roots: one value, no imaginary part, no 1E-5 wobble from the cube root.
  ECHECK(eqn::solveCubic(1, -6, 12, -8, r, n) == Outcome::Ok && n == 1 && r[0].re == 2 && r[0].im == 0);
  ECHECK(eqn::solveCubic(1, -0.3, 0.03, -0.001, r, n) == Outcome::Ok && n == 1 && near(r[0].re, 0.1) && r[0].im == 0);
  ECHECK(eqn::solveCubic(27, -27, 9, -1, r, n) == Outcome::Ok && n == 1 && near(r[0].re, 1.0 / 3) && r[0].im == 0);
  ECHECK(eqn::solveCubic(1, 3, 3, 1, r, n) == Outcome::Ok && n == 1 && near(r[0].re, -1));
  ECHECK(eqn::solveCubic(2, 0, 0, 0, r, n) == Outcome::Ok && n == 1 && r[0].re == 0 && !std::signbit(r[0].re));
  ECHECK(eqn::solveCubic(1, -3e30, 3e60, -1e90, r, n) == Outcome::Ok && n == 1 && near(r[0].re, 1e30, 1e-12));
  // Double roots: two values, no imaginary parts.
  ECHECK(eqn::solveCubic(1, -4, 5, -2, r, n) == Outcome::Ok && n == 2 && near(r[0].re, 2) && near(r[1].re, 1) &&
         r[0].im == 0 && r[1].im == 0);
  ECHECK(eqn::solveCubic(1, 0.1, -0.05, 0.003, r, n) == Outcome::Ok && n == 2 && near(r[0].re, 0.1, 1e-9) &&
         near(r[1].re, -0.3) && r[0].im == 0 && r[1].im == 0);
  ECHECK(eqn::solveCubic(1, 0, -3, 2, r, n) == Outcome::Ok && n == 2 && near(r[0].re, 1) && near(r[1].re, -2));
  ECHECK(eqn::solveCubic(1, -1, 0, 0, r, n) == Outcome::Ok && n == 2 && r[0].re == 1 && r[1].re == 0);  // x²(x−1)
  // d = 0: X = 0 exactly.
  ECHECK(eqn::solveCubic(1, 0, -1, 0, r, n) == Outcome::Ok && n == 3 && r[0].re == 1 && r[1].re == 0 && r[2].re == -1);
  // Scaling.
  ECHECK(eqn::solveCubic(1e60, -6e60, 11e60, -6e60, r, n) == Outcome::Ok && n == 3 && near(r[0].re, 3) &&
         near(r[2].re, 1));
  ECHECK(eqn::solveCubic(1e-60, -6e-60, 11e-60, -6e-60, r, n) == Outcome::Ok && n == 3 && near(r[1].re, 2));
  ECHECK(eqn::solveCubic(1, -1e6, -1, 1e6, r, n) == Outcome::Ok && n == 3 && near(r[0].re, 1e6) && near(r[1].re, 1) &&
         near(r[2].re, -1));
  // Close but distinct roots stay distinct.
  ECHECK(eqn::solveCubic(1, -3.001, 3.002, -1.001, r, n) == Outcome::Ok && n == 2 && near(r[0].re, 1.001, 1e-9) &&
         near(r[1].re, 1, 1e-9) && r[0].im == 0 && r[1].im == 0);  // (x−1)²(x−1.001)
  ECHECK(eqn::solveCubic(1, -3.003, 3.006002, -1.003002, r, n) == Outcome::Ok && n == 3 && near(r[0].re, 1.002, 1e-9) &&
         near(r[1].re, 1.001, 1e-9) && near(r[2].re, 1, 1e-9));  // (x−1)(x−1.001)(x−1.002)
  // Errors.
  ECHECK(eqn::solveCubic(0, 1, 1, 1, r, n) == Outcome::MathError);
  ECHECK(eqn::solveCubic(1e-99, 1e99, 0, 1, r, n) == Outcome::MathError);
  // Random cubics: every root solves the equation to rounding noise; three roots counted
  // with conjugate pairs; Vieta's sum.
  unsigned seed = 777;
  auto rnd = [&]() {
    seed = seed * 1103515245u + 12345u;
    return static_cast<double>((seed >> 8) % 20001) / 1000.0 - 10.0;
  };
  int badRoot = 0, badSum = 0;
  for (int t = 0; t < 500; ++t) {
    double a = rnd(), b = rnd(), c = rnd(), d = rnd();
    if (a == 0) a = 1;
    if (eqn::solveCubic(a, b, c, d, r, n) != Outcome::Ok) {
      ++badRoot;
      continue;
    }
    for (int i = 0; i < n; ++i)
      if (resid(a, b, c, d, r[i]) > 1e-12) ++badRoot;
    if (n == 3) {
      const double sum = r[0].re + r[1].re + r[2].re;
      if (std::fabs(sum + b / a) > 1e-9 * (1 + std::fabs(b / a))) ++badSum;
    }
  }
  ECHECK(badRoot == 0 && badSum == 0);
  // Random cubics built from chosen roots (some repeated): the right number of distinct roots.
  int badCount = 0;
  for (int t = 0; t < 300; ++t) {
    const double x1 = std::round(rnd()), x2 = t % 3 == 0 ? x1 : std::round(rnd()), x3 = t % 5 == 0 ? x1 : std::round(rnd());
    const double b = -(x1 + x2 + x3), c = x1 * x2 + x1 * x3 + x2 * x3, d = -x1 * x2 * x3;
    if (eqn::solveCubic(1, b, c, d, r, n) != Outcome::Ok) {
      ++badCount;
      continue;
    }
    int distinct = 1 + (x2 != x1) + (x3 != x1 && x3 != x2);
    if (n != distinct) ++badCount;
    for (int i = 0; i < n; ++i)
      if (r[i].im != 0 || (std::fabs(r[i].re - x1) > 1e-9 && std::fabs(r[i].re - x2) > 1e-9 && std::fabs(r[i].re - x3) > 1e-9))
        ++badCount;
  }
  ECHECK(badCount == 0);
}

// ---------------------------------------------------------------- the editor (EqnApp alone)
void press(EqnApp& e, DKey k, bool shift = false, bool alpha = false) { e.onKey(k, shift, alpha); }
const DKey kDigits[] = {DKey::D0, DKey::D1, DKey::D2, DKey::D3, DKey::D4,
                        DKey::D5, DKey::D6, DKey::D7, DKey::D8, DKey::D9};
// Types a number ("-" = (−), "." = .) and presses =.
void enter(EqnApp& e, const char* s) {
  for (const char* p = s; *p; ++p) {
    if (*p == '-') press(e, DKey::Neg);
    else if (*p == '.') press(e, DKey::Dot);
    else if (*p == '/') press(e, DKey::Div);
    else press(e, kDigits[*p - '0']);
  }
  press(e, DKey::Eq);
}

void testEditor() {
  EqnApp e;
  Vars v;
  v.a = Num::ofInt(5);
  e.setEnv(&v, AngleUnit::Deg, NormMode::Norm1, nullptr);
  ECHECK(e.screen() == EqnApp::Screen::TypeSelect && !e.typeChosen());
  press(e, DKey::D5);  // not a type
  ECHECK(e.screen() == EqnApp::Screen::TypeSelect);
  press(e, DKey::AC);  // AC on the first type list: leave
  ECHECK(e.wantsExit());
  e.resume();
  ECHECK(!e.wantsExit());
  press(e, DKey::D1);
  ECHECK(e.screen() == EqnApp::Screen::Editor && e.type() == EqnType::Lin2 && e.rows() == 2 && e.cols() == 3);
  ECHECK(e.curRow() == 0 && e.curCol() == 0);
  // = moves right, then to the next equation's first cell.
  enter(e, "1");
  ECHECK(e.curRow() == 0 && e.curCol() == 1 && e.coef(0, 0) == 1);
  enter(e, "1");
  enter(e, "3");
  ECHECK(e.curRow() == 1 && e.curCol() == 0);
  enter(e, "1");
  enter(e, "-1");
  enter(e, "1");
  ECHECK(e.curRow() == 1 && e.curCol() == 2 && e.coef(1, 1) == -1);  // the last cell stays
  // Arrows stop at the edges.
  press(e, DKey::Right);
  press(e, DKey::Down);
  ECHECK(e.curRow() == 1 && e.curCol() == 2);
  for (int i = 0; i < 4; ++i) press(e, DKey::Left);
  press(e, DKey::Up);
  press(e, DKey::Up);
  ECHECK(e.curRow() == 0 && e.curCol() == 0);
  // = solves: X=, Y=; ▼ / = step, ▲ back, = after the last returns to the editor.
  press(e, DKey::Eq);
  ECHECK(e.screen() == EqnApp::Screen::Result && e.resultCount() == 2 && e.resultIndex() == 0);
  ECHECK(e.shownLabel() == "X=" && e.shownText() == "2");
  press(e, DKey::Up);
  ECHECK(e.resultIndex() == 0);
  press(e, DKey::Down);
  ECHECK(e.shownLabel() == "Y=" && e.shownText() == "1");
  press(e, DKey::Down);
  ECHECK(e.resultIndex() == 1);
  press(e, DKey::Up);
  press(e, DKey::Eq);
  ECHECK(e.resultIndex() == 1);
  press(e, DKey::Eq);
  ECHECK(e.screen() == EqnApp::Screen::Editor);
  press(e, DKey::Eq);
  press(e, DKey::AC);  // AC from the results: the editor
  ECHECK(e.screen() == EqnApp::Screen::Editor && e.coef(0, 0) == 1);
  // Typing: an expression, DEL backspaces, AC cancels; a variable (ALPHA (−) = A).
  press(e, DKey::D7);
  ECHECK(e.typing() && e.entry().size() == 1);
  press(e, DKey::Del);
  ECHECK(e.typing() && e.entry().empty());
  press(e, DKey::D7);
  press(e, DKey::AC);
  ECHECK(!e.typing() && e.coef(0, 0) == 1 && e.screen() == EqnApp::Screen::Editor);
  enter(e, "1/4");
  ECHECK(e.coef(0, 0) == 0.25 && e.curCol() == 1);
  press(e, DKey::Neg, false, true);  // A
  press(e, DKey::Eq);
  ECHECK(e.coef(0, 1) == 5);
  ECHECK(v.ans.v == 0);  // nothing touched the calculator's memories
  // DEL (not typing) zeroes the cell.
  press(e, DKey::Left);
  press(e, DKey::Del);
  ECHECK(e.coef(0, 1) == 0 && !e.typing());
  // A bad entry: Syntax ERROR; ◀ goes back with the entry kept, AC drops it.
  press(e, DKey::D1);
  press(e, DKey::Div);
  press(e, DKey::Eq);
  ECHECK(e.screen() == EqnApp::Screen::Error && std::string(e.message()) == "Syntax ERROR");
  press(e, DKey::Left);
  ECHECK(e.screen() == EqnApp::Screen::Editor && e.typing() && e.entry().size() == 2);
  press(e, DKey::Eq);
  press(e, DKey::AC);
  ECHECK(e.screen() == EqnApp::Screen::Editor && !e.typing() && e.coef(0, 1) == 0);
  // 1÷0: Math ERROR.
  enter(e, "1/0");
  ECHECK(e.screen() == EqnApp::Screen::Error && std::string(e.message()) == "Math ERROR");
  press(e, DKey::AC);
  // AC from the editor: the type list; AC there: back to the editor (a type was chosen).
  press(e, DKey::AC);
  ECHECK(e.screen() == EqnApp::Screen::TypeSelect);
  press(e, DKey::AC);
  ECHECK(e.screen() == EqnApp::Screen::Editor && !e.wantsExit());
  // The same type keeps the coefficients; another zeroes them.
  press(e, DKey::AC);
  press(e, DKey::D1);
  ECHECK(e.coef(0, 0) == 0.25);
  press(e, DKey::AC);
  press(e, DKey::D3);
  ECHECK(e.type() == EqnType::Quad && e.rows() == 1 && e.cols() == 3 && e.coef(0, 0) == 0);
  // 2X² − 3X + 1 = 0: 1 and 1/2, then the vertex; S⇔D shows 0.5.
  enter(e, "2");
  enter(e, "-3");
  enter(e, "1");
  ECHECK(e.curCol() == 2);
  press(e, DKey::Eq);
  ECHECK(e.resultCount() == 4 && e.result(0).label == "X1=" && e.result(1).label == "X2=");
  ECHECK(e.resultText(0) == "1" && e.resultText(1) == "1\xE2\x8C\x9F" "2");
  ECHECK(e.result(2).label == "X-Value Minimum=" && e.resultText(2) == "3\xE2\x8C\x9F" "4");
  ECHECK(e.result(3).label == "Y-Value Minimum=" && e.resultText(3) == kMinus + "1\xE2\x8C\x9F" "8");
  press(e, DKey::SD);
  ECHECK(e.resultText(1) == "0.5" && e.resultText(3) == kMinus + "0.125");
  press(e, DKey::AC);
  // a = 0: Math ERROR, AC back to the editor.
  press(e, DKey::Left);
  press(e, DKey::Left);
  press(e, DKey::Del);
  press(e, DKey::Eq);
  ECHECK(e.screen() == EqnApp::Screen::Error && e.outcome() == Outcome::MathError);
  press(e, DKey::AC);
  ECHECK(e.screen() == EqnApp::Screen::Editor);
  // −X² ... a maximum.
  enter(e, "-1");
  press(e, DKey::Eq);
  ECHECK(e.result(e.resultCount() - 2).label == "X-Value Maximum=");
  press(e, DKey::AC);
  // Cubic: one cell row of four.
  press(e, DKey::AC);
  press(e, DKey::D4);
  ECHECK(e.type() == EqnType::Cubic && e.cols() == 4);
  enter(e, "1");
  enter(e, "0");
  enter(e, "0");
  enter(e, "-8");
  press(e, DKey::Eq);
  ECHECK(e.resultCount() == 3 && e.shownLabel() == "X1=" && e.shownText() == "2");
  press(e, DKey::Down);
  ECHECK(e.shownText() == kMinus + "1+1.732050808i");
  // Singular systems: messages; AC / = / ◀ back.
  press(e, DKey::AC);
  press(e, DKey::AC);
  press(e, DKey::D1);
  enter(e, "1");
  enter(e, "1");
  enter(e, "1");
  enter(e, "2");
  enter(e, "2");
  enter(e, "3");
  press(e, DKey::Eq);
  ECHECK(e.screen() == EqnApp::Screen::Message && std::string(e.message()) == "No Solution");
  press(e, DKey::Eq);
  ECHECK(e.screen() == EqnApp::Screen::Editor);
  press(e, DKey::Left);
  press(e, DKey::Left);
  enter(e, "2");  // row 2 = 2 x row 1 now... (2, 2, 3): make the constant 2
  enter(e, "2");
  enter(e, "2");
  press(e, DKey::Eq);
  ECHECK(e.screen() == EqnApp::Screen::Message && std::string(e.message()) == "Infinite Solution");
  press(e, DKey::Right);
  ECHECK(e.screen() == EqnApp::Screen::Editor);
  // 3 unknowns via the keys.
  press(e, DKey::AC);
  press(e, DKey::D2);
  ECHECK(e.rows() == 3 && e.cols() == 4);
  for (const char* s : {"2", "1", "-1", "8", "-3", "-1", "2", "-11", "-2", "1", "2", "-3"}) enter(e, s);
  press(e, DKey::Eq);
  ECHECK(e.resultCount() == 3 && e.resultText(0) == "2" && e.resultText(1) == "3" && e.resultText(2) == kMinus + "1");
  ECHECK(e.result(2).label == "Z=");
  // setCoef(): out of the grid or not holdable.
  ECHECK(!e.setCoef(3, 0, 1) && !e.setCoef(0, 4, 1) && !e.setCoef(0, 0, 1e100) && !e.setCoef(0, 0, NAN));
  ECHECK(e.setCoef(2, 3, -1e-120) && e.coef(2, 3) == 0);  // below 1E-99: 0
  // reset(): everything fresh.
  e.reset();
  ECHECK(e.screen() == EqnApp::Screen::TypeSelect && !e.typeChosen() && e.coef(0, 0) == 0);
}

// ---------------------------------------------------------------- Device: MODE 5, exam, state
void keys(Device& d, std::initializer_list<DKey> ks) {
  for (DKey k : ks) d.onKey(k);
}
void type(Device& d, const char* s) {  // keysForChar() characters
  for (; *s; ++s) {
    DKey k[2];
    int n = 0;
    if (calc::keysForChar(*s, k, n))
      for (int i = 0; i < n; ++i) d.onKey(k[i]);
  }
}

void testDevice() {
  using K = DKey;
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode});
    snapshot("eqn_menu_mode", d);
    keys(d, {K::D5});
    ECHECK(d.view() == View::Eqn && d.mode() == Mode::Eqn);
    snapshot("eqn_type", d);
    keys(d, {K::AC});  // AC on the type list: COMP
    ECHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
    keys(d, {K::Mode, K::D5, K::D1});
    ECHECK(d.eqn().screen() == EqnApp::Screen::Editor);
    type(d, "2=3=8=1=");
    snapshot("eqn_editor_lin2", d);
    type(d, "~4");
    snapshot("eqn_editor_typing", d);
    type(d, "=~7=");
    type(d, "=");
    ECHECK(d.eqn().screen() == EqnApp::Screen::Result && d.eqn().shownText() == "1" && d.eqn().resultText(1) == "2");
    snapshot("eqn_result_x", d);
    ECHECK(d.vars().ans.v == 0);  // results are not stored to Ans (as on the Casio)
    // ON: like AC (the editor), never out of the mode.
    keys(d, {K::On});
    ECHECK(d.view() == View::Eqn && d.eqn().screen() == EqnApp::Screen::Editor);
    // MODE 1 leaves.
    keys(d, {K::Mode, K::D1});
    ECHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
    // MODE 4 (Apps) and back: EQN keeps its place under the apps.
    keys(d, {K::Mode, K::D5, K::D2});
    type(d, "1=1=1=6=");
    keys(d, {K::Mode, K::D4});
    ECHECK(d.view() == View::Apps);
    keys(d, {K::AC});
    ECHECK(d.view() == View::Eqn && d.mode() == Mode::Eqn && d.eqn().coef(0, 3) == 6);
    snapshot("eqn_editor_lin3", d);
    // SETUP over EQN returns to it.
    keys(d, {K::Shift, K::Mode, K::D2});
    ECHECK(d.view() == View::Eqn && d.eqn().angleUnit() == AngleUnit::Rad);
  }
  {  // all the result screens
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D5, K::D3});
    type(d, "1=2=5=");
    snapshot("eqn_editor_quad", d);
    type(d, "=");
    snapshot("eqn_result_complex", d);
    type(d, "dd");
    snapshot("eqn_result_vertex_x", d);
    type(d, "d");
    snapshot("eqn_result_vertex_y", d);
    keys(d, {K::AC, K::AC, K::D4});
    type(d, "3=0=0=1=");
    snapshot("eqn_editor_cubic", d);
    type(d, "=d");
    // 0.3466806371+0.6004684775i: too long for big digits, too long for one row: two rows.
    ECHECK(textLength(d.eqn().shownText()) > Framebuffer::kCols);
    snapshot("eqn_result_complex_long", d);
    keys(d, {K::AC, K::AC, K::D1});
    type(d, "1=1=1=2=2=3==");
    snapshot("eqn_no_solution", d);
    keys(d, {K::AC});
    type(d, "<<2=2=2==");
    snapshot("eqn_infinite", d);
    keys(d, {K::AC, K::AC, K::D3});
    type(d, "0==");
    // a = 0 → Math ERROR; the cursor moved on, so the solve is the second =
    ECHECK(d.eqn().screen() == EqnApp::Screen::Error);
    snapshot("eqn_math_error", d);
    keys(d, {K::AC});
    ECHECK(d.eqn().screen() == EqnApp::Screen::Editor);
  }
  {  // exam mode allows EQN (a calculator mode); the apps stay off
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::Mode, K::D8, K::Eq});
    ECHECK(d.examActive());
    keys(d, {K::AC, K::Mode});
    snapshot("eqn_menu_mode_exam", d);
    keys(d, {K::D5});
    ECHECK(d.view() == View::Eqn && d.mode() == Mode::Eqn);
    keys(d, {K::D3});
    type(d, "1=~3=2==");
    ECHECK(d.eqn().screen() == EqnApp::Screen::Result && d.eqn().shownText() == "2");
    snapshot("eqn_exam_result", d);
    {  // ▼ shows on the inverted exam bar (it used to be drawn black on black)
      Framebuffer fb;
      d.render(fb);
      int set = 0;
      for (int y = 0; y < 8; ++y)
        for (int x = 120; x < 125; ++x) set += fb.get(x, y) ? 1 : 0;
      ECHECK(set > 0 && set < 40);
    }
    keys(d, {K::Mode, K::D4});
    ECHECK(d.view() == View::Notice);
    keys(d, {K::AC});
    ECHECK(d.view() == View::Eqn);
  }
  {  // saveState(): the type and coefficients survive a deep sleep; damaged blocks are refused
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D5, K::D2});
    type(d, "2=1=~1=8=~3=~1=2=~1.1=~2=1=2=~3=");
    const std::string s = d.saveState();
    ECHECK(s.size() <= 2048);
    Device e;
    e.tick(0);
    ECHECK(e.restoreState(s) && e.mode() == Mode::Eqn && e.view() == View::Eqn);
    ECHECK(e.eqn().typeChosen() && e.eqn().type() == EqnType::Lin3 && e.eqn().screen() == EqnApp::Screen::Editor);
    bool same = true;
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 4; ++c) same = same && e.eqn().coef(r, c) == d.eqn().coef(r, c);
    ECHECK(same && e.eqn().coef(1, 3) == -1.1);
    // Still fits with a long replay history.
    Device h;
    h.tick(0);
    for (int i = 0; i < 60; ++i) type(h, "123456789+987654321*2=");
    keys(h, {K::Mode, K::D5, K::D4});
    type(h, "1=2=3=4=");
    const std::string hs = h.saveState();
    Device h2;
    h2.tick(0);
    ECHECK(hs.size() <= 2048 && h2.restoreState(hs) && h2.eqn().type() == EqnType::Cubic && h2.eqn().coef(0, 3) == 4);
    // No type chosen yet.
    Device f;
    f.tick(0);
    keys(f, {K::Mode, K::D5});
    Device g;
    g.tick(0);
    ECHECK(g.restoreState(f.saveState()) && g.view() == View::Eqn && !g.eqn().typeChosen() &&
           g.eqn().screen() == EqnApp::Screen::TypeSelect);
    // Damaged: a bad type, a bad mode byte, a non-finite coefficient (re-signed so the
    // checksum passes: the parser itself must refuse them).
    auto resign = [](std::string body) {
      body.resize(body.size() - 4);
      uint32_t a = 1, b = 0;
      for (unsigned char ch : body) {
        a = (a + ch) % 65521;
        b = (b + a) % 65521;
      }
      const uint32_t sum = (b << 16) | a;
      for (int i = 0; i < 4; ++i) body += static_cast<char>((sum >> (8 * i)) & 0xFF);
      return body;
    };
    {
      std::string bad = s;
      const size_t tag = bad.size() - 4 - 12 * 8 - 2;
      ECHECK(bad[tag] == 'E' && bad[tag + 1] == 1);
      bad[tag + 1] = 7;
      Device x;
      x.tick(0);
      ECHECK(!x.restoreState(resign(bad)) && x.mode() == Mode::Comp);
      bad = s;
      bad[1] = 5;  // mode byte CMPLX with an EQN block: refused (and 6, beyond Cmplx: cmplx_tests)
      ECHECK(!x.restoreState(resign(bad)));
      bad = s;
      for (int i = 0; i < 8; ++i) bad[tag + 2 + i] = static_cast<char>(0xFF);  // a NaN
      ECHECK(!x.restoreState(resign(bad)));
      bad = s;
      bad.erase(bad.size() - 4 - 8, 8);  // one coefficient short
      ECHECK(!x.restoreState(resign(bad)));
      ECHECK(x.restoreState(resign(s)));  // the resign itself is right
    }
  }
}

// Every screen draws only glyphs the font has, and the result rows stay on the screen.
void testRender() {
  EqnApp e;
  Framebuffer fb;
  for (int t = 0; t < static_cast<int>(EqnType::Count); ++t) {
    bool ok = true;
    for (uint32_t cp : decodeUtf8(eqnTypeName(static_cast<EqnType>(t)))) ok = ok && hasGlyph(cp);
    ECHECK(ok && textLength(std::string("1:") + eqnTypeName(static_cast<EqnType>(t))) <= Framebuffer::kCols);
  }
  fb.clear();
  e.render(fb);
  ECHECK(fb.toAscii().find('#') != std::string::npos);
  // The cell highlight follows the cursor in a 4-column grid.
  e.setType(EqnType::Cubic);
  e.onKey(DKey::Right, false, false);
  e.onKey(DKey::Right, false, false);
  e.onKey(DKey::Right, false, false);
  fb.clear();
  e.render(fb);
  ECHECK(fb.get(20 * 5 + 2, 2 * 8 + 3) && !fb.get(2 * 5 + 2, 2 * 8 + 3));  // d's cell inverted, a's not
  // Up / Down icons on the results.
  e.setCoef(0, 0, 1);
  e.setCoef(0, 3, -1);
  e.solve();
  fb.clear();
  e.render(fb);
  ECHECK(!fb.icon(Icon::Up) && fb.icon(Icon::Down));
  e.onKey(DKey::Down, false, false);
  e.onKey(DKey::Down, false, false);
  fb.clear();
  e.render(fb);
  ECHECK(fb.icon(Icon::Up) && !fb.icon(Icon::Down));
}

}  // namespace

int runEqnTests() {
  testLinear();
  testQuadratic();
  testCubic();
  testEditor();
  testDevice();
  testRender();
  std::printf("eqn: %d checks, %d failed\n", g_checks, g_fail);
  return g_fail;
}
