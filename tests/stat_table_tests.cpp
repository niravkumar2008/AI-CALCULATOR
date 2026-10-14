// Unit tests for STAT (core/stat.*) and TABLE (core/table.*). The lead hooks
// runStatTableTests() into tests.cpp; it returns the number of failures and prints one
// line per failure.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/font.h"
#include "../core/framebuffer.h"
#include "../core/stat.h"
#include "../core/table.h"

using namespace calc;

namespace {

int g_fail = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
  ++g_checks;
  if (!ok) {
    ++g_fail;
    std::printf("FAIL stat_table_tests.cpp:%d: %s\n", line, what);
  }
}
#define SCHECK(cond) check((cond), #cond, __LINE__)

bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) <= tol * (1 + std::fabs(b)); }

double stat(const StatApp& app, StatVar v) {
  double d = -12345.678;
  app.value(v, d);
  return d;
}
bool statFails(const StatApp& app, StatVar v) {
  double d;
  CalcError e = CalcError::None;
  return !app.value(v, d, &e) && e == CalcError::Math;
}
double est(const StatApp& app, StatVar v, double arg) {
  double d = -12345.678;
  app.estimate(v, arg, d);
  return d;
}

template <class App>
void press(App& app, DKey k, bool shift = false, bool alpha = false) {
  app.onKey(k, shift, alpha);
}

const DKey kDigits[] = {DKey::D0, DKey::D1, DKey::D2, DKey::D3, DKey::D4,
                        DKey::D5, DKey::D6, DKey::D7, DKey::D8, DKey::D9};

// Types a number with the keys ("-" = the (−) key) and presses =.
template <class App>
void enter(App& app, const char* s, bool eq = true) {
  for (const char* p = s; *p; ++p) {
    if (*p == '-') press(app, DKey::Neg);
    else if (*p == '.') press(app, DKey::Dot);
    else press(app, kDigits[*p - '0']);
  }
  if (eq) press(app, DKey::Eq);
}

int pixels(const Framebuffer& fb, int y0, int y1) {
  int n = 0;
  for (int y = y0; y <= y1; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) n += fb.get(x, y) ? 1 : 0;
  return n;
}

bool allGlyphs(const std::string& s) {
  for (uint32_t cp : decodeUtf8(s))
    if (cp != 0x0304 && cp != 0x0302 && !hasGlyph(cp)) return false;
  return true;
}

// Renders and returns the content pixels (rows 1-6).
template <class App>
int renderPixels(const App& app, Framebuffer* out = nullptr) {
  Framebuffer fb;
  fb.clear();
  app.render(fb);
  if (out) *out = fb;
  return pixels(fb, 8, 60);
}

void loadPairs(StatApp& app, StatType t, const std::vector<double>& xs, const std::vector<double>& ys) {
  app.setType(t);
  for (size_t i = 0; i < xs.size(); ++i) app.addRow(xs[i], ys[i]);
}

// --------------------------------------------------------------------------- helpers

void testHelpers() {
  // numberTokens reads back as the same double through calc_engine.
  const double vals[] = {0.1, -2.5, 1e-50, 6.02214076e23, 1.0 / 3.0, -9.99e99, 123456789012345.0, 7, -0.000123};
  for (double v : vals) {
    std::vector<Tok> t;
    SCHECK(numberTokens(v, t));
    Vars vars;
    const EvalResult r = evaluate(t, vars, AngleUnit::Deg, nullptr);
    SCHECK(r.error == CalcError::None);
    SCHECK(near(r.value.v, v, 1e-15));
  }
  std::vector<Tok> t;
  SCHECK(!numberTokens(1e100, t) && t.empty());
  SCHECK(!numberTokens(std::nan(""), t) && t.empty());
  SCHECK(numberTokens(0, t) && t.size() == 1 && t[0] == Tok::D0);
  t.clear();
  numberTokens(0.5, t);  // "(0.5)" -> ( 5 E - 1 ) is fine too; just check it is bracketed
  SCHECK(t.front() == Tok::Open && t.back() == Tok::Close);

  SCHECK(cellText(0, 6) == "0");
  SCHECK(cellText(1.0 / 3.0, 6) == "0.3333");
  SCHECK(cellText(123456, 6) == "123456");
  SCHECK(cellText(1234567, 6) == "1.23E6");
  SCHECK(cellText(-0.0000123, 6) == "-1E-5");
  SCHECK(cellText(2.5, 6) == "2.5");
  for (double v : {1e99, -1e-99, -123456789.0, 0.000001})
    SCHECK(static_cast<int>(cellText(v, 6).size()) <= 6);

  // keys: digits, SHIFT functions, ALPHA X
  Tok k;
  SCHECK(modeKeyToken(DKey::D7, false, false, k) && k == Tok::D7);
  SCHECK(modeKeyToken(DKey::Close, false, true, k) && k == Tok::VarX);
  SCHECK(modeKeyToken(DKey::Sq, true, false, k) && k == Tok::Cube);
  SCHECK(!modeKeyToken(DKey::D1, true, false, k));  // SHIFT 1 = STAT, not a token
  SCHECK(!modeKeyToken(DKey::Up, false, false, k));

  SCHECK(markedLength("x\xCC\x84") == 1);
  SCHECK(markedLength(statVarText(StatVar::XHat1)) == 2);
  Framebuffer fb;
  fb.clear();
  SCHECK(drawMarkedText(fb, 0, 1, statVarText(StatVar::MeanX)) == 1);
  SCHECK(fb.get(0, 8) && fb.get(3, 8));  // the bar over x
}

// --------------------------------------------------------------------------- 1-VAR

void testOneVar() {
  StatApp app;
  SCHECK(app.screen() == StatApp::Screen::TypeSelect);
  press(app, DKey::D1);
  SCHECK(app.screen() == StatApp::Screen::Editor && app.type() == StatType::OneVar && app.columns() == 1);
  // The textbook set 2 4 4 4 5 5 7 9: mean 5, population σ 2, sample s = √(32/7).
  for (const char* v : {"2", "4", "4", "4", "5", "5", "7", "9"}) enter(app, v);
  SCHECK(app.rows() == 8);
  SCHECK(app.curRow() == 8);
  SCHECK(near(stat(app, StatVar::N), 8));
  SCHECK(near(stat(app, StatVar::MeanX), 5));
  SCHECK(near(stat(app, StatVar::SigmaX), 2));
  SCHECK(near(stat(app, StatVar::SX), std::sqrt(32.0 / 7.0)));
  SCHECK(near(stat(app, StatVar::SX), 2.138089935, 1e-9));
  SCHECK(near(stat(app, StatVar::SumX), 40));
  SCHECK(near(stat(app, StatVar::SumX2), 232));
  SCHECK(near(stat(app, StatVar::MinX), 2));
  SCHECK(near(stat(app, StatVar::MaxX), 9));
  SCHECK(!app.available(StatVar::MeanY) && !app.available(StatVar::A) && statFails(app, StatVar::SumY));

  // AC -> the stat calculation screen; SHIFT 1 > 4:Var > 2:x̄, =
  press(app, DKey::AC);
  SCHECK(app.screen() == StatApp::Screen::Calc);
  press(app, DKey::D1, true);
  SCHECK(app.screen() == StatApp::Screen::Menu && app.menu() == StatApp::Menu::Stat);
  SCHECK(app.menuItems().size() == 5);  // Type Data Sum Var MinMax (no Reg for 1-VAR)
  SCHECK(app.menuItems()[4].text == "MinMax");
  press(app, DKey::D4);
  SCHECK(app.menu() == StatApp::Menu::Var && app.menuItems().size() == 4);
  press(app, DKey::D2);
  SCHECK(app.screen() == StatApp::Screen::Calc);
  SCHECK(app.expression().size() == 1 && app.expression()[0].var == StatVar::MeanX);
  SCHECK(app.expressionText() == "x\xCC\x84");
  press(app, DKey::Mul);
  press(app, DKey::D2);
  press(app, DKey::Eq);
  SCHECK(app.showingResult() && near(app.result().v, 10));
  SCHECK(app.resultText() == "10");
  // "+σx" continues from Ans: Ans+σx = 12
  press(app, DKey::Add);
  press(app, DKey::D1, true);
  press(app, DKey::D4);
  press(app, DKey::D3);
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 12));
  // implicit multiplication and a postfix: 3σx² = 12
  press(app, DKey::AC);
  press(app, DKey::D3);
  press(app, DKey::D1, true);
  press(app, DKey::D4);
  press(app, DKey::D3);
  press(app, DKey::Sq);
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 12));
  // S⇔D on a fraction: x̄÷3 = 5/3
  app.setExpression({StatItem::stat(StatVar::MeanX), StatItem::token(Tok::Div), StatItem::token(Tok::D3)});
  press(app, DKey::Eq);
  SCHECK(app.resultText() == "5\xE2\x8C\x9F" "3");
  press(app, DKey::SD);
  SCHECK(app.resultText() == "1.666666667");

  // Frequencies: 1×2, 2×3, 3×5 -> n 10, x̄ 2.3, σx² = 5.9 - 2.3² = 0.61
  StatApp f;
  f.setFrequency(true);
  SCHECK(f.frequency());
  f.setType(StatType::OneVar);
  SCHECK(f.columns() == 2 && f.column(1) == StatApp::Column::Freq && f.maxRows() == 40);
  f.addRow(1, 0, 2);
  f.addRow(2, 0, 3);
  f.addRow(3, 0, 5);
  SCHECK(near(stat(f, StatVar::N), 10));
  SCHECK(near(stat(f, StatVar::MeanX), 2.3));
  SCHECK(near(stat(f, StatVar::SumX2), 59));
  SCHECK(near(stat(f, StatVar::SigmaX), std::sqrt(0.61)));
  SCHECK(near(stat(f, StatVar::SX), std::sqrt(6.1 / 9)));
  // FREQ typed in the editor; a negative FREQ is a Math ERROR
  f.setType(StatType::OneVar);
  enter(f, "4");
  press(f, DKey::Up);
  press(f, DKey::Right);
  enter(f, "3");
  SCHECK(near(f.freq(0), 3) && near(stat(f, StatVar::N), 3));
  press(f, DKey::Up);
  enter(f, "-1");
  SCHECK(f.screen() == StatApp::Screen::Error && f.error() == CalcError::Math);
  press(f, DKey::AC);
  SCHECK(f.screen() == StatApp::Screen::Editor && !f.typing() && near(f.freq(0), 3));
  // changing the Frequency setting clears the data
  f.setFrequency(false);
  SCHECK(f.rows() == 0 && f.columns() == 1);

  // Errors: no data, n = 1
  StatApp e;
  e.setType(StatType::OneVar);
  SCHECK(statFails(e, StatVar::MeanX) && statFails(e, StatVar::MinX));
  SCHECK(near(stat(e, StatVar::SumX), 0) && near(stat(e, StatVar::N), 0));
  e.addRow(5);
  SCHECK(near(stat(e, StatVar::SigmaX), 0) && statFails(e, StatVar::SX));
  // x̄ with no data in the calculation screen: Math ERROR; AC clears, ◀ keeps
  e.clearData();
  press(e, DKey::AC);
  e.setExpression({StatItem::stat(StatVar::MeanX), StatItem::token(Tok::Add), StatItem::token(Tok::D1)});
  press(e, DKey::Eq);
  SCHECK(e.screen() == StatApp::Screen::Error && e.error() == CalcError::Math);
  press(e, DKey::Left);
  SCHECK(e.screen() == StatApp::Screen::Calc && e.expression().size() == 3);
  press(e, DKey::Eq);
  press(e, DKey::AC);
  SCHECK(e.screen() == StatApp::Screen::Calc && e.expression().empty());
}

// --------------------------------------------------------------------------- regressions

void testLinear() {
  StatApp app;
  press(app, DKey::D2);
  SCHECK(app.type() == StatType::Linear && app.columns() == 2 && app.maxRows() == 40);
  // x 1..5, y 2 4 5 4 5: y = 2.2 + 0.6x, r = 6/√60
  const char* ys[] = {"2", "4", "5", "4", "5"};
  for (int i = 0; i < 5; ++i) enter(app, std::to_string(i + 1).c_str());
  press(app, DKey::Right);
  for (int i = 0; i < 5; ++i) press(app, DKey::Up);
  SCHECK(app.curRow() == 0 && app.curCol() == 1);
  for (const char* y : ys) enter(app, y);
  SCHECK(app.rows() == 5);
  SCHECK(near(stat(app, StatVar::A), 2.2));
  SCHECK(near(stat(app, StatVar::B), 0.6));
  SCHECK(near(stat(app, StatVar::R), 6 / std::sqrt(60.0)));
  SCHECK(near(stat(app, StatVar::R), 0.7745966692, 1e-9));
  SCHECK(near(stat(app, StatVar::MeanY), 4));
  SCHECK(near(stat(app, StatVar::SumXY), 66));
  SCHECK(near(stat(app, StatVar::SumY2), 86));
  SCHECK(near(stat(app, StatVar::SumX3), 225));
  SCHECK(near(stat(app, StatVar::SumX2Y), 252));
  SCHECK(near(stat(app, StatVar::SumX4), 979));
  SCHECK(near(stat(app, StatVar::MinY), 2) && near(stat(app, StatVar::MaxY), 5));
  SCHECK(near(est(app, StatVar::YHat, 3), 4));
  SCHECK(near(est(app, StatVar::XHat, 4), 3));
  SCHECK(!app.available(StatVar::C) && statFails(app, StatVar::C));

  // 3ŷ = 4 through the keys: 3, SHIFT 1, 5:Reg, 5:ŷ, =
  press(app, DKey::AC);
  press(app, DKey::D3);
  press(app, DKey::D1, true);
  SCHECK(app.menuItems().size() == 6 && app.menuItems()[4].text == "Reg");
  press(app, DKey::D5);
  SCHECK(app.menu() == StatApp::Menu::Reg && app.menuItems().size() == 5);
  press(app, DKey::D5);
  press(app, DKey::Eq);
  SCHECK(app.showingResult() && near(app.result().v, 4));
  // (1+3)x̂ = 3 ... (y = 4 -> x = 3); and x̂ right after a result works on Ans
  app.setExpression({StatItem::token(Tok::Open), StatItem::token(Tok::D1), StatItem::token(Tok::Add),
                     StatItem::token(Tok::D3), StatItem::token(Tok::Close), StatItem::stat(StatVar::XHat)});
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 3));
  // a postfix with nothing before it is a Syntax ERROR
  app.setExpression({StatItem::stat(StatVar::YHat)});
  press(app, DKey::Eq);
  SCHECK(app.screen() == StatApp::Screen::Error && app.error() == CalcError::Syntax);
  press(app, DKey::AC);
  // sin(30)ŷ: an operand with a function: ŷ(0.5) = 2.5
  app.setExpression({StatItem::token(Tok::Sin), StatItem::token(Tok::D3), StatItem::token(Tok::D0),
                     StatItem::token(Tok::Close), StatItem::stat(StatVar::YHat)});
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 2.5));
  // 2^(2)ŷ: the power's base is part of the operand: ŷ(4) = 4.6
  app.setExpression({StatItem::token(Tok::D2), StatItem::token(Tok::Pow), StatItem::token(Tok::D2),
                     StatItem::token(Tok::Close), StatItem::stat(StatVar::YHat)});
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 4.6));
  // -2ŷ = -(ŷ(2)) = -3.4 (ŷ binds tighter than the sign, as on the Casio)
  app.setExpression({StatItem::token(Tok::Neg), StatItem::token(Tok::D2), StatItem::stat(StatVar::YHat)});
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, -3.4));

  // Anscombe's quartet, set I (published: y = 3.00 + 0.500x, r = 0.816)
  StatApp an;
  loadPairs(an, StatType::Linear, {10, 8, 13, 9, 11, 14, 6, 4, 12, 7, 5},
            {8.04, 6.95, 7.58, 8.81, 8.33, 9.96, 7.24, 4.26, 10.84, 4.82, 5.68});
  SCHECK(near(stat(an, StatVar::A), 3.000090909, 1e-8));
  SCHECK(near(stat(an, StatVar::B), 0.5000909091, 1e-8));
  SCHECK(near(stat(an, StatVar::R), 0.8164205163, 1e-8));
  SCHECK(near(stat(an, StatVar::MeanX), 9) && near(stat(an, StatVar::MeanY), 7.500909091, 1e-8));

  // no spread in x: Math ERROR; no spread in y: A, B fine, r undefined
  StatApp flat;
  loadPairs(flat, StatType::Linear, {2, 2, 2}, {1, 2, 3});
  SCHECK(statFails(flat, StatVar::A) && statFails(flat, StatVar::B));
  loadPairs(flat, StatType::Linear, {1, 2, 3}, {4, 4, 4});
  SCHECK(near(stat(flat, StatVar::A), 4) && near(stat(flat, StatVar::B), 0) && statFails(flat, StatVar::R));
  SCHECK(statFails(flat, StatVar::A) == false);
  double d;
  SCHECK(!flat.estimate(StatVar::XHat, 4, d));  // B = 0
}

void testQuad() {
  StatApp q;
  loadPairs(q, StatType::Quad, {0, 1, 2, 3, 4}, {1, 6, 17, 34, 57});  // y = 1 + 2x + 3x²
  SCHECK(near(stat(q, StatVar::A), 1) && near(stat(q, StatVar::B), 2) && near(stat(q, StatVar::C), 3));
  SCHECK(near(est(q, StatVar::YHat, 2), 17));
  SCHECK(near(est(q, StatVar::XHat1, 6), 1));         // 3x² + 2x - 5 = 0
  SCHECK(near(est(q, StatVar::XHat2, 6), -5.0 / 3));
  double d;
  SCHECK(!q.estimate(StatVar::XHat1, -100, d));       // no real root
  SCHECK(!q.available(StatVar::R) && !q.available(StatVar::XHat) && statFails(q, StatVar::R));
  q.setExpression({StatItem::token(Tok::D6), StatItem::stat(StatVar::XHat2)});
  press(q, DKey::Eq);
  SCHECK(near(q.result().v, -5.0 / 3));
  press(q, DKey::D1, true);
  press(q, DKey::D5);
  SCHECK(q.menuItems().size() == 6 && q.menuItems()[2].var == StatVar::C && q.menuItems()[4].var == StatVar::XHat2);
  press(q, DKey::AC);

  // Noisy data against the normal equations solved by Cramer's rule.
  const double xs[] = {1, 2, 3, 4, 5, 6}, ys[] = {2.1, 3.9, 9.2, 15.8, 26.1, 35.7};
  StatApp n;
  n.setType(StatType::Quad);
  double s0 = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, t0 = 0, t1 = 0, t2 = 0;
  for (int i = 0; i < 6; ++i) {
    n.addRow(xs[i], ys[i]);
    const double x = xs[i], y = ys[i];
    s0 += 1; s1 += x; s2 += x * x; s3 += x * x * x; s4 += x * x * x * x;
    t0 += y; t1 += x * y; t2 += x * x * y;
  }
  auto det3 = [](double a, double b, double c, double d2, double e, double f, double g, double h, double i) {
    return a * (e * i - f * h) - b * (d2 * i - f * g) + c * (d2 * h - e * g);
  };
  const double D = det3(s0, s1, s2, s1, s2, s3, s2, s3, s4);
  const double A = det3(t0, s1, s2, t1, s2, s3, t2, s3, s4) / D;
  const double B = det3(s0, t0, s2, s1, t1, s3, s2, t2, s4) / D;
  const double C = det3(s0, s1, t0, s1, s2, t1, s2, s3, t2) / D;
  SCHECK(near(stat(n, StatVar::A), A, 1e-9) && near(stat(n, StatVar::B), B, 1e-9) && near(stat(n, StatVar::C), C, 1e-9));
  SCHECK(near(stat(n, StatVar::SumX2Y), t2) && near(stat(n, StatVar::SumX4), s4));

  // two distinct x values can't fix a parabola
  StatApp two;
  loadPairs(two, StatType::Quad, {1, 1, 2, 2}, {1, 2, 3, 4});
  SCHECK(statFails(two, StatVar::C));
}

void testTransforms() {
  // ln X: y = 2 + 3 ln x
  StatApp a;
  {
    std::vector<double> xs = {1, 2, 3, 4}, ys;
    for (double x : xs) ys.push_back(2 + 3 * std::log(x));
    loadPairs(a, StatType::Log, xs, ys);
  }
  SCHECK(near(stat(a, StatVar::A), 2) && near(stat(a, StatVar::B), 3) && near(stat(a, StatVar::R), 1));
  SCHECK(near(est(a, StatVar::XHat, 2 + 3 * std::log(2.0)), 2) && near(est(a, StatVar::YHat, 5), 2 + 3 * std::log(5.0)));
  a.addRow(0, 1);  // ln 0
  SCHECK(statFails(a, StatVar::A));

  // e^X: y = 2 e^(0.5x)
  StatApp e;
  {
    std::vector<double> xs = {0, 1, 2, 3}, ys;
    for (double x : xs) ys.push_back(2 * std::exp(0.5 * x));
    loadPairs(e, StatType::Exp, xs, ys);
  }
  SCHECK(near(stat(e, StatVar::A), 2) && near(stat(e, StatVar::B), 0.5) && near(stat(e, StatVar::R), 1));
  SCHECK(near(est(e, StatVar::YHat, 4), 2 * std::exp(2.0)) && near(est(e, StatVar::XHat, 2 * std::exp(1.0)), 2));
  e.addRow(5, -1);  // ln of y <= 0
  SCHECK(statFails(e, StatVar::B));

  // A·B^X: y = 3·2^x
  StatApp ab;
  loadPairs(ab, StatType::ABExp, {0, 1, 2, 3}, {3, 6, 12, 24});
  SCHECK(near(stat(ab, StatVar::A), 3) && near(stat(ab, StatVar::B), 2) && near(stat(ab, StatVar::R), 1));
  SCHECK(near(est(ab, StatVar::XHat, 48), 4) && near(est(ab, StatVar::YHat, 5), 96));

  // A·X^B: y = 2 x^1.5
  StatApp p;
  {
    std::vector<double> xs = {1, 2, 3, 4}, ys;
    for (double x : xs) ys.push_back(2 * std::pow(x, 1.5));
    loadPairs(p, StatType::Power, xs, ys);
  }
  SCHECK(near(stat(p, StatVar::A), 2) && near(stat(p, StatVar::B), 1.5) && near(stat(p, StatVar::R), 1));
  SCHECK(near(est(p, StatVar::YHat, 9), 54) && near(est(p, StatVar::XHat, 16), 4));

  // 1/X: y = 1 + 4/x
  StatApp inv;
  loadPairs(inv, StatType::Inverse, {1, 2, 4}, {5, 3, 2});
  SCHECK(near(stat(inv, StatVar::A), 1) && near(stat(inv, StatVar::B), 4) && near(stat(inv, StatVar::R), 1));
  SCHECK(near(est(inv, StatVar::YHat, 8), 1.5) && near(est(inv, StatVar::XHat, 3), 2));
  double d;
  SCHECK(!inv.estimate(StatVar::YHat, 0, d));
  inv.addRow(0, 1);
  SCHECK(statFails(inv, StatVar::A));

  // A noisy exponential set checked against the transformed linear fit done here.
  const double xs[] = {1, 2, 3, 4, 5}, ys[] = {3.1, 4.4, 6.9, 10.2, 15.3};
  StatApp ne;
  ne.setType(StatType::Exp);
  double su = 0, sv = 0, suu = 0, suv = 0;
  for (int i = 0; i < 5; ++i) {
    ne.addRow(xs[i], ys[i]);
    const double v = std::log(ys[i]);
    su += xs[i]; sv += v; suu += xs[i] * xs[i]; suv += xs[i] * v;
  }
  const double b = (5 * suv - su * sv) / (5 * suu - su * su), a0 = (sv - b * su) / 5;
  SCHECK(near(stat(ne, StatVar::B), b, 1e-9) && near(stat(ne, StatVar::A), std::exp(a0), 1e-9));
}

// --------------------------------------------------------------------------- editor

void testEditor() {
  StatApp app;
  press(app, DKey::D2);  // A+BX
  // navigation on an empty editor: the cursor stays on the new-row line
  press(app, DKey::Down);
  press(app, DKey::Up);
  press(app, DKey::Left);
  SCHECK(app.curRow() == 0 && app.curCol() == 0);
  press(app, DKey::Right);
  press(app, DKey::Right);
  SCHECK(app.curCol() == 1);  // X, Y only
  press(app, DKey::Left);
  // an expression in a cell: 2+3 = 5
  press(app, DKey::D2);
  SCHECK(app.typing() && app.entry().size() == 1);
  press(app, DKey::Add);
  press(app, DKey::D3);
  press(app, DKey::Eq);
  SCHECK(!app.typing() && app.rows() == 1 && near(app.x(0), 5) && near(app.y(0), 0) && app.curRow() == 1);
  // DEL backspaces while typing; AC cancels
  press(app, DKey::D9);
  press(app, DKey::D8);
  press(app, DKey::Del);
  SCHECK(app.entry().size() == 1);
  press(app, DKey::AC);
  SCHECK(!app.typing() && app.rows() == 1 && app.screen() == StatApp::Screen::Editor);
  // 1÷0 -> Math ERROR; ◀ goes back to the typing, intact; AC drops it
  press(app, DKey::D1);
  press(app, DKey::Div);
  press(app, DKey::D0);
  press(app, DKey::Eq);
  SCHECK(app.screen() == StatApp::Screen::Error && app.error() == CalcError::Math);
  press(app, DKey::Left);
  SCHECK(app.screen() == StatApp::Screen::Editor && app.typing() && app.entry().size() == 3);
  press(app, DKey::Eq);
  press(app, DKey::AC);
  SCHECK(app.screen() == StatApp::Screen::Editor && !app.typing() && app.rows() == 1);
  // Syntax ERROR
  press(app, DKey::Mul);
  press(app, DKey::Eq);
  SCHECK(app.screen() == StatApp::Screen::Error && app.error() == CalcError::Syntax);
  press(app, DKey::AC);
  // variables: ALPHA ) = X, from the host's variables
  Vars host;
  host.x = Num::ofInt(7);
  app.setEnv(&host, AngleUnit::Deg, NormMode::Norm1, nullptr);
  press(app, DKey::Close, false, true);
  press(app, DKey::Eq);
  SCHECK(app.rows() == 2 && near(app.x(1), 7));
  // editing an existing cell replaces it; Y cell
  press(app, DKey::Up);
  press(app, DKey::Right);
  enter(app, "4");
  SCHECK(near(app.y(1), 4) && app.curRow() == 2 && app.curCol() == 1);
  // scrolling: 8 rows, the cursor at the new line 9 -> the top is row 6 (index 5)
  for (int i = 0; i < 6; ++i) enter(app, "1");
  SCHECK(app.rows() == 8 && app.curRow() == 8 && app.editorTop() == 5);
  Framebuffer fb;
  renderPixels(app, &fb);
  SCHECK(fb.icon(Icon::Up) && !fb.icon(Icon::Down));
  for (int i = 0; i < 8; ++i) press(app, DKey::Up);
  SCHECK(app.curRow() == 0 && app.editorTop() == 0);
  renderPixels(app, &fb);
  SCHECK(!fb.icon(Icon::Up) && fb.icon(Icon::Down));
  // DEL deletes the row under the cursor
  press(app, DKey::Del);
  SCHECK(app.rows() == 7 && near(app.x(0), 7) && near(app.y(0), 4));
  // SHIFT 1 > 3:Edit > 1:Ins inserts a 0 / 0 row; 2:Del-A deletes all
  press(app, DKey::D1, true);
  SCHECK(app.menuItems().size() == 7 && app.menuItems()[2].text == "Edit");
  press(app, DKey::D3);
  SCHECK(app.menu() == StatApp::Menu::Edit);
  press(app, DKey::D1);
  SCHECK(app.screen() == StatApp::Screen::Editor && app.rows() == 8 && near(app.x(0), 0) && near(app.x(1), 7));
  // a statistic picked from the editor's menu goes to the calculation screen
  press(app, DKey::D1, true);
  press(app, DKey::D5);  // Var
  press(app, DKey::D1);  // n
  SCHECK(app.screen() == StatApp::Screen::Calc && app.expression().size() == 1 &&
         app.expression()[0].var == StatVar::N);
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 8) && near(host.ans.v, 8));
  // 2:Data goes back to the editor
  press(app, DKey::D1, true);
  press(app, DKey::D2);
  SCHECK(app.screen() == StatApp::Screen::Editor);
  press(app, DKey::D1, true);
  press(app, DKey::D3);
  press(app, DKey::D2);
  SCHECK(app.rows() == 0 && app.curRow() == 0);
  // AC closes a menu
  press(app, DKey::D1, true);
  press(app, DKey::AC);
  SCHECK(app.screen() == StatApp::Screen::Editor);

  // 1:Type from the menu: AC goes back; a new type clears the data
  app.addRow(1, 2);
  press(app, DKey::D1, true);
  press(app, DKey::D1);
  SCHECK(app.screen() == StatApp::Screen::TypeSelect);
  press(app, DKey::AC);
  SCHECK(app.screen() == StatApp::Screen::Editor && app.rows() == 1 && !app.wantsExit());
  press(app, DKey::D1, true);
  press(app, DKey::D1);
  press(app, DKey::D3);
  SCHECK(app.type() == StatType::Quad && app.rows() == 0 && app.screen() == StatApp::Screen::Editor);

  // AC on the first type selection asks the host to leave
  StatApp fresh;
  press(fresh, DKey::AC);
  SCHECK(fresh.wantsExit());
  fresh.resume();
  SCHECK(!fresh.wantsExit());
  press(fresh, DKey::D9);  // not a type
  SCHECK(fresh.screen() == StatApp::Screen::TypeSelect);
}

void testRowLimits() {
  struct L { StatType t; bool freq; int max; };
  const L limits[] = {{StatType::OneVar, false, 80}, {StatType::OneVar, true, 40},
                      {StatType::Linear, false, 40}, {StatType::Power, true, 26}};
  for (const L& l : limits) {
    StatApp app;
    app.setFrequency(l.freq);
    app.setType(l.t);
    SCHECK(app.maxRows() == l.max);
    for (int i = 0; i < l.max; ++i) SCHECK(app.addRow(i + 1, 1, 1) || (std::printf("  row %d\n", i), false));
    SCHECK(!app.addRow(99) && app.rows() == l.max && !app.insertRow(0));
    // the cursor stops at the last row (no new-row line when full)
    for (int i = 0; i < l.max + 5; ++i) press(app, DKey::Down);
    SCHECK(app.curRow() == l.max - 1);
    SCHECK(app.editorTop() == l.max - StatApp::kEditorRows);
    // typing on the last row changes it; nothing is added
    enter(app, "5");
    SCHECK(app.rows() == l.max && near(app.x(l.max - 1), 5));
    SCHECK(near(stat(app, StatVar::N), l.max));
  }
  // keys fill a 1-VAR editor to its 80 rows
  StatApp k;
  press(k, DKey::D1);
  for (int i = 0; i < 85; ++i) enter(k, "3");
  SCHECK(k.rows() == 80 && near(stat(k, StatVar::SumX), 240));
}

// --------------------------------------------------------------------------- env and render

void testEnvAndRender() {
  // the host's variables: A + x̄, and = updates Ans / PreAns
  Vars host;
  host.a = Num::ofInt(10);
  host.ans = Num::ofInt(1);
  StatApp app;
  app.setEnv(&host, AngleUnit::Deg, NormMode::Norm1, nullptr);
  press(app, DKey::D1);
  enter(app, "4");
  enter(app, "6");
  press(app, DKey::AC);
  press(app, DKey::Neg, false, true);  // ALPHA (−) = A
  press(app, DKey::Add);
  press(app, DKey::D1, true);
  press(app, DKey::D4);
  press(app, DKey::D2);
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 15) && near(host.ans.v, 15) && near(host.preAns.v, 1));
  // Ans in the next calculation
  press(app, DKey::Ans);
  press(app, DKey::Mul);
  press(app, DKey::D2);
  press(app, DKey::Eq);
  SCHECK(near(app.result().v, 30));
  SCHECK(near(app.evaluate({StatItem::stat(StatVar::SigmaX)}).value.v, 1));

  // every screen renders, with every label in the font
  for (int t = 0; t < static_cast<int>(StatType::Count); ++t) SCHECK(allGlyphs(statTypeName(static_cast<StatType>(t))));
  for (int v = 1; v < static_cast<int>(StatVar::Count); ++v) SCHECK(allGlyphs(statVarText(static_cast<StatVar>(v))));
  StatApp r;
  r.setFrequency(true);
  SCHECK(renderPixels(r) > 0);  // type selection
  press(r, DKey::D2);
  SCHECK(renderPixels(r) > 0);  // empty editor (header)
  r.addRow(1.5, -2.25, 2);
  r.addRow(1234567, 0.000123, 1);
  SCHECK(r.columns() == 3);
  Framebuffer fb;
  SCHECK(renderPixels(r, &fb) > 0);
  press(r, DKey::D7);  // typing
  SCHECK(renderPixels(r) > 0);
  press(r, DKey::AC);
  press(r, DKey::D1, true);
  SCHECK(renderPixels(r) > 0);
  for (const auto& it : r.menuItems()) SCHECK(allGlyphs(it.text));
  for (int sub = 3; sub <= 7; ++sub) {  // Edit Sum Var Reg MinMax (from the editor)
    StatApp s = r;
    press(s, kDigits[sub]);
    SCHECK(s.screen() == StatApp::Screen::Menu);
    SCHECK(renderPixels(s) > 0);
    for (const auto& it : s.menuItems()) SCHECK(allGlyphs(it.text));
  }
  press(r, DKey::AC);
  press(r, DKey::AC);  // to the calculation screen
  SCHECK(r.screen() == StatApp::Screen::Calc);
  SCHECK(renderPixels(r) > 0);  // the cursor
  r.setExpression({StatItem::stat(StatVar::MeanX), StatItem::token(Tok::Add), StatItem::stat(StatVar::SY),
                   StatItem::token(Tok::Add), StatItem::token(Tok::D1), StatItem::stat(StatVar::YHat)});
  SCHECK(renderPixels(r) > 0);
  press(r, DKey::Eq);
  SCHECK(r.showingResult());
  const int big = renderPixels(r, &fb);
  SCHECK(big > 0 && pixels(fb, 46, 60) > 0);  // the answer at the bottom
  // a long expression wraps without crashing
  std::vector<StatItem> longExpr;
  for (int i = 0; i < 60; ++i) longExpr.push_back(StatItem::stat(StatVar::SumX2Y));
  r.setExpression(longExpr);
  SCHECK(renderPixels(r) > 0);
  press(r, DKey::Eq);  // a long product: a result or an error, either way it renders
  SCHECK(renderPixels(r) > 0);
  // the editor's bottom line shows the selected value in full
  StatApp full;
  full.setType(StatType::OneVar);
  full.addRow(1.0 / 3.0);
  press(full, DKey::Up);
  Framebuffer ref;
  renderPixels(full, &fb);
  ref.clear();
  ref.drawText(25 - 12, 6, "0.3333333333");
  bool same = true;
  for (int y = 48; y <= 55; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) same = same && fb.get(x, y) == ref.get(x, y);
  SCHECK(same);
  // error screen
  StatApp er;
  er.setType(StatType::OneVar);
  press(er, DKey::AC);
  er.setExpression({StatItem::stat(StatVar::MeanX)});
  press(er, DKey::Eq);
  SCHECK(er.screen() == StatApp::Screen::Error);
  SCHECK(renderPixels(er) > 0);
}

// --------------------------------------------------------------------------- TABLE

void testTable() {
  TableApp t;
  SCHECK(t.screen() == TableApp::Screen::Function && t.start() == 1 && t.end() == 5 && t.step() == 1);
  // f(X) = X²+1 with the keys; = = = keeps the defaults 1, 5, 1
  press(t, DKey::Close, false, true);
  press(t, DKey::Sq);
  press(t, DKey::Add);
  press(t, DKey::D1);
  SCHECK(t.functionText() == "X\xC2\xB2+1");
  press(t, DKey::Eq);
  SCHECK(t.screen() == TableApp::Screen::Start);
  SCHECK(renderPixels(t) > 0);
  press(t, DKey::Eq);
  SCHECK(t.screen() == TableApp::Screen::End);
  press(t, DKey::Eq);
  SCHECK(t.screen() == TableApp::Screen::Step);
  press(t, DKey::Eq);
  SCHECK(t.screen() == TableApp::Screen::Table && t.rows() == 5);
  const double want[] = {2, 5, 10, 17, 26};
  for (int i = 0; i < 5; ++i) SCHECK(t.rowOk(i) && near(t.rowX(i), i + 1) && near(t.rowY(i), want[i]));
  Framebuffer fb;
  SCHECK(renderPixels(t, &fb) > 0 && fb.icon(Icon::Down));
  // scrolling and columns
  press(t, DKey::Right);
  SCHECK(t.cursorCol() == 1);
  for (int i = 0; i < 7; ++i) press(t, DKey::Down);
  SCHECK(t.cursorRow() == 4 && t.top() == 1);
  renderPixels(t, &fb);
  SCHECK(fb.icon(Icon::Up) && !fb.icon(Icon::Down));
  press(t, DKey::Left);
  SCHECK(t.cursorCol() == 0);
  // AC back to f(X)= with the expression kept
  press(t, DKey::AC);
  SCHECK(t.screen() == TableApp::Screen::Function && t.function().size() == 4);

  // typed range: Start -2, End 2, Step 0.5 -> 9 rows
  press(t, DKey::Eq);
  enter(t, "-2");
  enter(t, "2");
  enter(t, "0.5");
  SCHECK(t.rows() == 9 && near(t.start(), -2) && near(t.step(), 0.5));
  SCHECK(near(t.rowX(8), 2) && near(t.rowY(1), 3.25));
  // the prompts show the current values; an expression works as a value
  press(t, DKey::AC);
  press(t, DKey::Eq);
  press(t, DKey::D1);
  press(t, DKey::Div);
  press(t, DKey::D2);
  press(t, DKey::Eq);
  SCHECK(near(t.start(), 0.5) && t.screen() == TableApp::Screen::End);
  press(t, DKey::AC);
  SCHECK(t.screen() == TableApp::Screen::Function);

  // 30 rows fit, 31 are "Insufficient MEM"
  SCHECK(TableApp::rowCount(1, 30, 1) == 30 && TableApp::rowCount(1, 31, 1) == 31);
  SCHECK(TableApp::rowCount(0, 1, 0.1) == 11);
  SCHECK(TableApp::rowCount(1, 5, 0) == 0 && TableApp::rowCount(1, 5, -1) == 0);
  SCHECK(TableApp::rowCount(5, 1, -1) == 5 && TableApp::rowCount(3, 3, 1) == 1);
  t.setRange(1, 30, 1);
  SCHECK(t.build() && t.rows() == 30 && near(t.rowY(29), 901));
  for (int i = 0; i < 40; ++i) press(t, DKey::Down);
  SCHECK(t.cursorRow() == 29 && t.top() == 26);
  t.setRange(1, 31, 1);
  SCHECK(!t.build() && t.screen() == TableApp::Screen::Error);
  SCHECK(std::strcmp(t.errorMessage(), "Insufficient MEM") == 0);
  SCHECK(renderPixels(t) > 0);
  press(t, DKey::AC);
  SCHECK(t.screen() == TableApp::Screen::Start);
  // a negative step with Start < End: Math ERROR (via the keys)
  enter(t, "1");
  enter(t, "5");
  enter(t, "-1");
  SCHECK(t.screen() == TableApp::Screen::Error && std::strcmp(t.errorMessage(), "Math ERROR") == 0);
  press(t, DKey::Right);
  SCHECK(t.screen() == TableApp::Screen::Start);
  // a negative step counting down works
  t.setRange(5, 1, -1);
  SCHECK(t.build() && t.rows() == 5 && near(t.rowX(0), 5) && near(t.rowX(4), 1));
  // step 0
  t.setRange(1, 5, 0);
  SCHECK(!t.build());
  press(t, DKey::AC);
  // an error in a prompt: 1÷0 at Start
  press(t, DKey::D1);
  press(t, DKey::Div);
  press(t, DKey::D0);
  press(t, DKey::Eq);
  SCHECK(t.screen() == TableApp::Screen::Error && std::strcmp(t.errorMessage(), "Math ERROR") == 0);
  press(t, DKey::Left);
  SCHECK(t.screen() == TableApp::Screen::Start && t.entry().size() == 3);

  // rows where f(X) fails show ERROR
  TableApp e;
  SCHECK(e.setFunctionText("1/(X-2)"));
  e.setRange(0, 4, 1);
  SCHECK(e.build());
  SCHECK(e.rowOk(0) && near(e.rowY(0), -0.5) && !e.rowOk(2) && e.rowError(2) == CalcError::Math && e.rowOk(3));
  for (int i = 0; i < 2; ++i) press(e, DKey::Down);
  press(e, DKey::Right);
  SCHECK(renderPixels(e, &fb) > 0);
  // the bottom line says ERROR: compare with a drawn "ERROR"
  Framebuffer ref;
  ref.clear();
  ref.drawText(20, 6, "ERROR");
  bool same = true;
  for (int y = 48; y <= 55; ++y)
    for (int x = 100; x < 125; ++x) same = same && fb.get(x, y) == ref.get(x, y);
  SCHECK(same);

  // setFunctionText: X / x, exp( keeps its x, "ans" stays Ans
  SCHECK(e.setFunctionText("exp(x)") && e.function().size() == 3 && e.function()[0] == Tok::Exp &&
         e.function()[1] == Tok::VarX);
  SCHECK(e.setFunctionText("2X") && e.function().size() == 2 && e.function()[1] == Tok::VarX);
  SCHECK(e.setFunctionText("Ans+x") && e.function()[0] == Tok::Ans && e.function()[2] == Tok::VarX);
  SCHECK(e.setFunctionText("X-1") && e.function()[1] == Tok::Sub);
  SCHECK(e.setFunctionText("2^X+1"));
  e.setRange(3, 3, 1);
  SCHECK(e.build() && near(e.rowY(0), 9));
  SCHECK(!e.setFunctionText("X$") && e.function().size() == 6);
  SCHECK(e.setFunctionText("sin(X)"));
  e.setRange(0, 90, 30);
  SCHECK(e.build() && near(e.rowY(1), 0.5) && near(e.rowY(3), 1));
  // the host's X is untouched; its other variables are used
  Vars host;
  host.x = Num::ofInt(42);
  host.b = Num::ofInt(3);
  TableApp h;
  h.setEnv(&host, AngleUnit::Rad, NormMode::Norm1, nullptr);
  SCHECK(h.setFunctionText("X+B"));
  h.setRange(1, 2, 1);
  SCHECK(h.build() && near(h.rowY(1), 5) && near(host.x.v, 42));

  // AC on f(X)=: clears, then asks to leave
  TableApp x;
  press(x, DKey::D1);
  press(x, DKey::Eq);  // f(X)=1 -> Start
  press(x, DKey::AC);
  press(x, DKey::AC);
  SCHECK(x.function().empty() && !x.wantsExit());
  press(x, DKey::AC);
  SCHECK(x.wantsExit());
  press(x, DKey::Eq);  // nothing to tabulate
  SCHECK(x.screen() == TableApp::Screen::Function);
  SCHECK(renderPixels(x) > 0);  // "f(X)="
  // a long function wraps
  for (int i = 0; i < 60; ++i) press(x, DKey::Close, false, true);
  SCHECK(renderPixels(x) > 0 && static_cast<int>(x.function().size()) == 60);
}

// --------------------------------------------------------------------------- bug hunt 2026-10-11

void testBugHunt() {
  // R-4: small negatives fit a 6-wide cell in E form ("######" for −0.0001 before).
  SCHECK(cellText(-0.000152, 6) == "-2E-4");
  SCHECK(cellText(-0.000152, 7) == "-1.5E-4");
  SCHECK(cellText(-0.0001, 6) == "-1E-4");
  SCHECK(cellText(-0.00099, 6) == "-0.001");
  SCHECK(cellText(0.000152, 6) == "1.5E-4");  // 2 digits beat "0.0002"
  SCHECK(cellText(0.000152, 7) == "1.52E-4");
  SCHECK(cellText(-1.5e-7, 6) == "-1E-7");  // -1.5E-7 is just below 1.5 in binary
  SCHECK(cellText(123456.7, 6) == "123457" && cellText(-123456.7, 6) == "-1.2E5");
  SCHECK(cellText(1e-100, 6) == "1E-100");
  for (double v : {-0.000999, -0.0005, -0.00012345, -9.99e-5, -1e-9, -1.23e-45})
    SCHECK(cellText(v, 6).find('#') == std::string::npos && cellText(v, 6).size() <= 6);
  // R-5: identical values have no spread (0.1 × 3 gave σx = 1.4E-17); identical x give no
  // regression (Math ERROR) and identical y no r.
  {
    StatApp a;
    a.setType(StatType::OneVar);
    for (int i = 0; i < 3; ++i) a.addRow(0.1);
    SCHECK(stat(a, StatVar::SigmaX) == 0 && stat(a, StatVar::SX) == 0 && near(stat(a, StatVar::MeanX), 0.1));
    StatApp f;
    f.setFrequency(true);
    f.setType(StatType::OneVar);
    f.addRow(0.1, 0, 3);
    SCHECK(stat(f, StatVar::SigmaX) == 0 && stat(f, StatVar::SX) == 0);
    StatApp g;
    g.setType(StatType::OneVar);
    for (int i = 0; i < 7; ++i) g.addRow(1.1);
    SCHECK(stat(g, StatVar::SigmaX) == 0);
    StatApp h;  // huge values with frequencies: σ from rounding was 1E25
    h.setFrequency(true);
    h.setType(StatType::OneVar);
    h.addRow(7.123456789e40, 0, 3);
    h.addRow(7.123456789e40, 0, 7);
    SCHECK(stat(h, StatVar::SigmaX) == 0);
    StatApp k;  // a real spread on a big offset is still seen
    k.setType(StatType::OneVar);
    for (double v : {1e9 + 0.25, 1e9 + 0.5, 1e9 + 0.75}) k.addRow(v);
    SCHECK(near(stat(k, StatVar::SigmaX), std::sqrt(0.125 / 3), 1e-6));
  }
  for (StatType t : {StatType::Linear, StatType::Log, StatType::Inverse, StatType::Power, StatType::Exp, StatType::ABExp}) {
    StatApp a;
    loadPairs(a, t, {0.1, 0.1, 0.1}, {1, 2, 3});
    SCHECK(statFails(a, StatVar::A) && statFails(a, StatVar::B) && statFails(a, StatVar::R));
    StatApp b;
    loadPairs(b, t, {2, 2, 2}, {1, 2, 3});
    SCHECK(statFails(b, StatVar::A) && statFails(b, StatVar::B));
  }
  {
    StatApp q;
    loadPairs(q, StatType::Quad, {0.1, 0.1, 0.1, 0.1}, {1, 2, 3, 4});
    SCHECK(statFails(q, StatVar::A) && statFails(q, StatVar::B) && statFails(q, StatVar::C));
    StatApp q2;  // a real quadratic on an offset still fits
    loadPairs(q2, StatType::Quad, {1000, 1001, 1002, 1003}, {1, 2, 5, 10});
    SCHECK(near(stat(q2, StatVar::C), 1, 1e-6) && near(est(q2, StatVar::YHat, 1004), 17, 1e-6));
    StatApp l;
    loadPairs(l, StatType::Linear, {1, 2, 4}, {0.1, 0.1, 0.1});
    SCHECK(stat(l, StatVar::B) == 0 && statFails(l, StatVar::R));
    StatApp e;
    loadPairs(e, StatType::Exp, {1, 2, 4}, {3, 3, 3});
    SCHECK(stat(e, StatVar::B) == 0 && statFails(e, StatVar::R));
  }
  // R-9: A·B^X keeps ln B for x̂ / ŷ (B = 1.000000001 lost its digits through e^b, ln B).
  {
    StatApp a;
    a.setType(StatType::ABExp);
    for (int i = 0; i < 5; ++i) a.addRow(i * 1000.0, 2 * std::exp(1e-9 * i * 1000.0));
    SCHECK(near(est(a, StatVar::XHat, 2 * std::exp(1e-9 * 2500)), 2500, 1e-7));
    SCHECK(near(est(a, StatVar::YHat, 2500), 2 * std::exp(2.5e-6), 1e-12));
    StatApp b;
    loadPairs(b, StatType::ABExp, {0, 1, 2}, {3, 6, 12});
    SCHECK(near(stat(b, StatVar::B), 2) && near(est(b, StatVar::XHat, 24), 3) && near(est(b, StatVar::YHat, 3), 24));
  }
  // R-6: TABLE's X without binary noise: −0.3 + 3 × 0.1 is 0, −0.3 + 2 × 0.1 is −0.2.
  {
    TableApp t;
    SCHECK(t.setFunctionText("X^2"));
    t.setRange(-0.3, 0.3, 0.1);
    SCHECK(t.build() && t.rows() == 7);
    for (int i = 0; i < 7; ++i) SCHECK(t.rowX(i) == (i - 3) / 10.0);
    SCHECK(t.rowX(3) == 0 && cellText(t.rowX(3), 9) == "0" && t.rowY(3) == 0);
    SCHECK(TableApp::rowValue(0, 1e-20, 5) == 5e-20);  // tiny steps keep their own scale
    SCHECK(TableApp::rowValue(1e13, 1, 3) == 1e13 + 3);
  }
}

}  // namespace

int runStatTableTests() {
  testHelpers();
  testOneVar();
  testLinear();
  testQuad();
  testTransforms();
  testEditor();
  testRowLimits();
  testEnvAndRender();
  testTable();
  testBugHunt();
  std::printf("stat/table: %d checks, %d failed\n", g_checks, g_fail);
  return g_fail;
}
