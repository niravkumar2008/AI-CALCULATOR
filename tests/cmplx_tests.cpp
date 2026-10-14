// Unit tests for CMPLX (core/cmplx.*, MODE 6): the complex evaluator (arithmetic, i² = −1, the
// SHIFT 2 functions, polar entry and display, the Math / Syntax / Stack ERRORs), the formats, and
// the Device integration (MODE menu, keys, Ans and memories, R⇔I, SETUP, exam mode, leaving
// CMPLX, saveState()). runCmplxTests() returns the number of failures; tests.cpp counts them.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include "../core/cmplx.h"
#include "../core/device.h"
#include "../core/font.h"
#include "../core/framebuffer.h"
#include "../core/text.h"

using namespace calc;

namespace {

int g_fail = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
  ++g_checks;
  if (!ok) {
    ++g_fail;
    std::printf("FAIL cmplx_tests.cpp:%d: %s\n", line, what);
  }
}
#define XCHECK(cond) check((cond), #cond, __LINE__)
// A text result, printed when it differs.
#define XEQ(got, want)                                                                                   \
  do {                                                                                                   \
    const std::string g_ = (got), w_ = (want);                                                           \
    if (g_ != w_) std::printf("  cmplx_tests.cpp:%d: got \"%s\", want \"%s\"\n", __LINE__, g_.c_str(), w_.c_str()); \
    check(g_ == w_, #got " == " #want, __LINE__);                                                         \
  } while (0)

const std::string M = "\xE2\x88\x92";            // −
const std::string ANG = "\xE2\x88\xA0";          // ∠
const std::string FR = "\xE2\x8C\x9F";           // ⌟
const std::string POL = "\xE2\x96\xB6r\xE2\x88\xA0\xCE\xB8";  // ▶r∠θ
const std::string RECT = "\xE2\x96\xB6" "a+bi";  // ▶a+bi

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
  XCHECK(same);
}

// Test-only text → tokens: longest match over the tokens' own text, plus * / ~ (the (−) sign),
// "-" (minus or sign by context), "<" for ∠, "pi". 'E' after a digit is ×10ˣ, else the variable E.
std::vector<Tok> tk(const std::string& s) {
  struct Cand {
    std::string text;
    Tok tok;
  };
  std::vector<Cand> c;
  for (int i = 0; i < static_cast<int>(Tok::Count); ++i) {
    const Tok t = static_cast<Tok>(i);
    if (t == Tok::Exp10 || t == Tok::VarE || t == Tok::NCr || t == Tok::NPr || t == Tok::Neg) continue;
    c.push_back({tokText(t), t});
  }
  c.push_back({"*", Tok::Mul});
  c.push_back({"/", Tok::Div});
  c.push_back({"<", Tok::Angle});
  c.push_back({"pi", Tok::Pi});
  c.push_back({"~", Tok::Neg});
  auto endsOperand = [](Tok t) {
    return t <= Tok::D9 || t == Tok::Dot || t == Tok::Close || (t >= Tok::Pi && t <= Tok::VarM) || t == Tok::Sq ||
           t == Tok::Cube || t == Tok::Inv || t == Tok::Fact || t == Tok::Pct || t == Tok::ImagI;
  };
  std::vector<Tok> out;
  size_t i = 0;
  while (i < s.size()) {
    if (s[i] == ' ') {
      ++i;
      continue;
    }
    if (s[i] == '-') {
      out.push_back(!out.empty() && endsOperand(out.back()) ? Tok::Sub : Tok::Neg);
      ++i;
      continue;
    }
    if (s[i] == 'E') {
      out.push_back(!out.empty() && (out.back() <= Tok::D9 || out.back() == Tok::Dot) ? Tok::Exp10 : Tok::VarE);
      ++i;
      continue;
    }
    size_t best = 0;
    Tok bt = Tok::Count;
    for (const Cand& k : c)
      if (k.text.size() > best && s.compare(i, k.text.size(), k.text) == 0) {
        best = k.text.size();
        bt = k.tok;
      }
    if (best == 0) {
      std::printf("cmplx tk: can't read '%s' at %zu\n", s.c_str(), i);
      ++g_fail;
      return out;
    }
    out.push_back(bt);
    i += best;
  }
  return out;
}

double fixedRandom() { return 0.25; }

CxResult run(const std::string& s, AngleUnit u = AngleUnit::Deg, Vars* vars = nullptr, const ImagVars* im = nullptr) {
  Vars v;
  ImagVars iv;
  return evaluateComplex(tk(s), vars ? *vars : v, im ? *im : iv, u, fixedRandom);
}

// The result as the Device shows it: an error's text, or the value in its form (a forced
// ▶r∠θ / ▶a+bi, else `form` for a non-real value; a real one as in COMP).
std::string cx(const std::string& s, AngleUnit u = AngleUnit::Deg, CxForm form = CxForm::Rect, bool frac = true) {
  const CxResult r = run(s, u);
  if (r.error != CalcError::None) return errorText(r.error);
  if (r.forced >= 0) form = r.forced == 1 ? CxForm::Polar : CxForm::Rect;
  else if (r.value.real()) return formatResult(r.value.re, frac, NormMode::Norm1);
  return complexText(r.value, form, frac, NormMode::Norm1, u);
}

// The real engine on the same text (COMP).
std::string real(const std::string& s, AngleUnit u = AngleUnit::Deg) {
  Vars v;
  const EvalResult r = evaluate(tk(s), v, u, fixedRandom);
  if (r.error != CalcError::None) return errorText(r.error);
  return formatResult(r.value, true, NormMode::Norm1);
}

// ---------------------------------------------------------------- arithmetic

void testArithmetic() {
  XEQ(cx("i"), "i");
  XEQ(cx("i\xC2\xB2"), M + "1");                    // i² = −1
  XEQ(cx("i*i"), M + "1");
  XEQ(cx("i\xC2\xB3"), M + "i");                    // i³ = −i
  XEQ(cx("(3+4i)(1-2i)"), "11" + M + "2i");          // the classic
  XEQ(cx("(3+4i)*(1-2i)"), "11" + M + "2i");
  XEQ(cx("(3+4i)/(1-2i)"), M + "1+2i");
  XEQ(cx("(3+4i)+(1-2i)"), "4+2i");
  XEQ(cx("(3+4i)-(3+4i)"), "0");                     // a + (−a) = 0
  XEQ(cx("(2+i)-(2-i)"), "2i");
  XEQ(cx("1/(1+i)"), "1" + FR + "2" + M + "1" + FR + "2i");  // exact parts, as fractions
  XEQ(cx("1/(1+i)", AngleUnit::Deg, CxForm::Rect, false), "0.5" + M + "0.5i");  // S⇔D
  XEQ(cx("(1+i)/2"), "1" + FR + "2+1" + FR + "2i");
  XEQ(cx("(1+i)\xE2\x81\xBB\xC2\xB9"), "1" + FR + "2" + M + "1" + FR + "2i");    // x⁻¹
  XEQ(cx("2i"), "2i");                                // implicit ×
  XEQ(cx("3i2"), "6i");
  XEQ(cx("i(1+i)"), M + "1+i");
  XEQ(cx("-i"), M + "i");
  XEQ(cx("~(1+i)"), M + "1" + M + "i");
  XEQ(cx("-i\xC2\xB2"), "1");                        // (−) looser than x²: −(i²)
  // Identities: z·conj(z) = |z|², z/z = 1, (a+b)(a−b) = a² − b².
  XEQ(cx("(2+3i)Conjg(2+3i)"), "13");
  XEQ(cx("(2+3i)/(2+3i)"), "1");
  XEQ(cx("(5-7i)(5+7i)"), "74");
  XEQ(cx("(1+2i)(3+4i)-(3+4i)(1+2i)"), "0");          // commutative
  XEQ(cx("(1+2i)((3+4i)+(5+6i))-((1+2i)(3+4i)+(1+2i)(5+6i))"), "0");  // distributive
  // Powers: integer exponents.
  XEQ(cx("i^(4)"), "1");
  XEQ(cx("i^(-1)"), M + "i");
  XEQ(cx("(1+i)^(8)"), "16");
  XEQ(cx("(1+i)^(2)"), "2i");
  XEQ(cx("(1+i)^(0)"), "1");
  XEQ(cx("(1+i)^(-2)"), M + "1" + FR + "2i");
  XEQ(cx("(1+i)^(0.5)"), "Math ERROR");               // not an integer
  XEQ(cx("i^(i)"), "Math ERROR");                     // a complex exponent
  XEQ(cx("2^(i)"), "Math ERROR");
  XEQ(cx("2^(0.5)"), real("2^(0.5)"));                // real ^ real: the real engine
  XEQ(cx("(-8)^(1" + FR + "3)"), M + "2");
  // Rounding noise is dropped: (1∠30)³ is exactly i on the Casio.
  XEQ(cx("(1<30)\xC2\xB3"), "i");
  XEQ(cx("(1<45)^(8)"), "1");
  // Division by zero.
  XEQ(cx("1/0"), "Math ERROR");
  XEQ(cx("i/0"), "Math ERROR");
  XEQ(cx("(1+i)/(0i)"), "Math ERROR");
  // Big parts: Smith's division where c² + d² would overflow.
  XEQ(cx("(1E60+1E60i)/(1E60+1E60i)"), "1");
  XEQ(cx("1E99i*10"), "Math ERROR");                  // out of range
}

// ---------------------------------------------------------------- real values: as in COMP
void testRealSameAsComp() {
  for (const char* s : {"1/3", "1" "\xE2\x8C\x9F" "2+1" "\xE2\x8C\x9F" "3", "sin(30)", "2+3*4", "-2\xC2\xB2", "2^(3)\xC2\xB2",
                        "\xE2\x88\x9A(8)", "log(1000)", "5!", "10C3", "1/0", "\xE2\x88\x9A(-1)", "ln(0)", "(", "1+",
                        "Abs(-3)", "50%", "GCD(12,18)", "Rnd(1/3)", "pi", "e^(1)", "sin\xE2\x81\xBB\xC2\xB9(2)"}) {
    const CxResult r = run(s);
    const std::string got = r.error != CalcError::None ? errorText(r.error) : formatResult(r.value.re, true, NormMode::Norm1);
    if (got != real(s)) std::printf("  real in CMPLX: %s: %s vs %s\n", s, got.c_str(), real(s).c_str());
    XCHECK(got == real(s));
    XCHECK(r.error != CalcError::None || r.value.real());
  }
  for (AngleUnit u : {AngleUnit::Deg, AngleUnit::Rad, AngleUnit::Gra}) XEQ(cx("cos(60)+tan(45)", u), real("cos(60)+tan(45)", u));
  // The real engine refuses the CMPLX tokens (COMP never evaluates them).
  for (const char* s : {"i", "1+i", "2<30", "arg(1)", "Conjg(1)", "1"}) {
    std::vector<Tok> t = tk(s);
    if (std::string(s) == "1") t.push_back(Tok::ToPolar);
    Vars v;
    XCHECK(evaluate(t, v, AngleUnit::Deg, fixedRandom).error == CalcError::Syntax);
  }
}

// ---------------------------------------------------------------- SHIFT 2 functions, Abs, polar
void testFunctions() {
  XEQ(cx("Conjg(3+4i)"), "3" + M + "4i");
  XEQ(cx("Conjg(-i)"), "i");
  XEQ(cx("Conjg(5)"), "5");
  XEQ(cx("Abs(3+4i)"), "5");                          // exact
  XEQ(cx("Abs(1+i)"), "1.414213562");
  XEQ(cx("Abs(-2i)"), "2");
  XEQ(cx("Abs(-7)"), "7");
  XEQ(cx("arg(1+i)"), "45");
  XEQ(cx("arg(1+i)", AngleUnit::Rad), "0.7853981634");
  XEQ(cx("arg(i)", AngleUnit::Gra), "100");
  XEQ(cx("arg(-1)"), "180");
  XEQ(cx("arg(-1)", AngleUnit::Rad), "3.141592654");
  XEQ(cx("arg(-i)"), M + "90");
  XEQ(cx("arg(0)"), "0");
  XEQ(cx("arg(-1-i)"), M + "135");                   // (−180°, 180°]
  XEQ(cx("arg(1+\xE2\x88\x9A(3)i)"), "60");
  // Polar entry r∠θ, θ in the angle unit.
  XEQ(cx("2<90"), "2i");
  XEQ(cx("1<180"), M + "1");
  XEQ(cx("2<30"), "1.732050808+i");
  XEQ(cx("2<-30"), "1.732050808" + M + "i");
  XEQ(cx("1<pi", AngleUnit::Rad), M + "1");
  XEQ(cx("1<100", AngleUnit::Gra), "i");
  XEQ(cx("2<30*2"), "3.464101615+2i");                // ∠ before ×: (2∠30)×2
  XEQ(cx("1+2<90"), "1+2i");
  XEQ(cx("i<30"), "Math ERROR");                       // r and θ must be real
  XEQ(cx("2<i"), "Math ERROR");
  // Display forms: ▶r∠θ / ▶a+bi at the end, else the SETUP choice.
  XEQ(cx("3+4i" + POL), "5" + ANG + "53.13010235");
  XEQ(cx("1+i" + POL), "1.414213562" + ANG + "45");
  XEQ(cx("1+i" + POL, AngleUnit::Rad), "1.414213562" + ANG + "0.7853981634");
  XEQ(cx("-1" + POL), "1" + ANG + "180");             // a real one too, when asked
  XEQ(cx("0" + POL), "0");
  XEQ(cx("3+4i", AngleUnit::Deg, CxForm::Polar), "5" + ANG + "53.13010235");
  XEQ(cx("2<30" + RECT, AngleUnit::Deg, CxForm::Polar), "1.732050808+i");
  XEQ(cx("5", AngleUnit::Deg, CxForm::Polar), "5");    // a real result stays real
  XEQ(cx(POL), "Syntax ERROR");
  XEQ(cx("1" + POL + "+1"), "Syntax ERROR");           // only at the very end
  XEQ(cx("1" + POL + POL), "Syntax ERROR");
  // cxAbs / cxArg directly.
  Cx z;
  z.re = Num::ofInt(-3);
  z.im = Num::ofInt(4);
  Num r, t;
  XCHECK(cxAbs(z, r) && r.exact && r.num == 5 && r.den == 1);
  XCHECK(cxArg(z, AngleUnit::Deg, t) && std::fabs(t.v - 126.8698976458) < 1e-9);
}

// ---------------------------------------------------------------- unsupported: Math ERROR
void testErrors() {
  for (const char* s : {"sin(i)", "cos(1+i)", "tan(i)", "log(i)", "ln(i)", "e^(i)", "10^(i)", "\xE2\x88\x9A(i)",
                        "\xC2\xB3\xE2\x88\x9A(i)", "\xE2\x88\x9A(-1)", "(1+i)!", "i%", "Int(i)", "Intg(i)", "Rnd(i)",
                        "log(2,i)", "GCD(i,2)", "sinh(i)", "sin\xE2\x81\xBB\xC2\xB9(i)", "2\xE2\x81\xBF\xE2\x88\x9A(i)",
                        "1\xE2\x8C\x9Fi"})
    XEQ(cx(s), "Math ERROR");
  {  // nPr / nCr: real only
    Vars v;
    ImagVars im;
    XCHECK(evaluateComplex({Tok::ImagI, Tok::NCr, Tok::D2}, v, im, AngleUnit::Deg, fixedRandom).error == CalcError::Math);
    XCHECK(evaluateComplex({Tok::D5, Tok::NPr, Tok::ImagI}, v, im, AngleUnit::Deg, fixedRandom).error == CalcError::Math);
    const CxResult ok = evaluateComplex({Tok::D5, Tok::NPr, Tok::D2}, v, im, AngleUnit::Deg, fixedRandom);
    XCHECK(ok.error == CalcError::None && ok.value.re.v == 20 && ok.value.real());
  }
  for (const char* s : {"1+", "i+*2", "\xE2\x88\xAB(X,0,1)", "d/dx(X,1)", "i=1", ")", "arg(1,2)", "Conjg()"})
    XEQ(cx(s), "Syntax ERROR");
  // The shared nesting budget: 24 levels, then Stack ERROR (as in COMP).
  std::string deep(24, '('), deeper(25, '(');
  XEQ(cx(deep + "i"), "i");
  XEQ(cx(deeper + "i"), "Stack ERROR");
  XEQ(real(deeper + "1"), "Stack ERROR");
  std::string fdeep;
  for (int k = 0; k < 24; ++k) fdeep += "Conjg(";
  XEQ(cx(fdeep + "i"), "i");  // 24 conjugates: i again
  XEQ(cx("Conjg(" + fdeep + "i"), "Stack ERROR");
  // Error positions (Goto): a Math ERROR at the token before, a Syntax ERROR at the token.
  CxResult r = run("1+sin(i)");
  XCHECK(r.error == CalcError::Math && r.errorPos == 4);  // the ")" closing sin(
  r = run("1+*2");
  XCHECK(r.error == CalcError::Syntax && r.errorPos == 2);
}

// ---------------------------------------------------------------- memories and formats
void testMemoriesAndFormats() {
  Vars v;
  ImagVars im;
  v.a = Num::ofInt(1);
  im.a = Num::ofInt(2);
  v.ans = Num::ofInt(3);
  im.ans = Num::ofInt(-1);
  v.preAns = Num::ofInt(0);
  im.preAns = Num::ofInt(5);
  CxResult r = run("A", AngleUnit::Deg, &v, &im);
  XEQ(complexText(r.value, CxForm::Rect, true, NormMode::Norm1, AngleUnit::Deg), "1+2i");
  r = run("2A+Ans", AngleUnit::Deg, &v, &im);
  XEQ(complexText(r.value, CxForm::Rect, true, NormMode::Norm1, AngleUnit::Deg), "5+3i");
  r = run("PreAns", AngleUnit::Deg, &v, &im);
  XEQ(complexText(r.value, CxForm::Rect, true, NormMode::Norm1, AngleUnit::Deg), "5i");
  XCHECK(im.any());
  XCHECK(!ImagVars().any());
  XCHECK(&im.get(Tok::VarM) == &im.m && &im.get(Tok::PreAns) == &im.preAns && &im.get(Tok::Ans) == &im.ans);
  // Formats: a long value splits into halves of at most a row each.
  Cx z;
  z.re = Num::ofDouble(1.0 / 3e20);
  z.im = Num::ofDouble(-2.0 / 3);
  const std::string full = complexText(z, CxForm::Rect, false, NormMode::Norm1, AngleUnit::Deg);
  XEQ(full, "3.333333333\xC3\x97" "10\xE2\x81\xBB\xC2\xB2\xC2\xB9" + M + "0.6666666667i");
  XCHECK(textLength(full) > Framebuffer::kCols);
  std::string a, b;
  complexParts(z, CxForm::Rect, false, NormMode::Norm1, AngleUnit::Deg, a, b);
  XEQ(a, "3.333333333\xC3\x97" "10\xE2\x81\xBB\xC2\xB2\xC2\xB9");
  XEQ(b, M + "0.6666666667i");
  complexParts(z, CxForm::Polar, false, NormMode::Norm1, AngleUnit::Deg, a, b);
  XEQ(a, "0.6666666667");
  XEQ(b, ANG + M + "90");
  // Norm2 and an imaginary part of 1 / −1.
  z.re = Num::ofDouble(0.001);
  z.im = Num::ofInt(-1);
  XEQ(complexText(z, CxForm::Rect, false, NormMode::Norm1, AngleUnit::Deg), "1\xC3\x97" "10\xE2\x81\xBB\xC2\xB3" + M + "i");
  XEQ(complexText(z, CxForm::Rect, false, NormMode::Norm2, AngleUnit::Deg), "0.001" + M + "i");
  // Every glyph the mode draws is in the font.
  for (const std::string& s : {ANG, POL, RECT, std::string("\xE2\x87\x94"), std::string("CMPLX Conjg( arg( i")})
    for (uint32_t cp : decodeUtf8(s)) XCHECK(hasGlyph(cp));
}

// ---------------------------------------------------------------- Device
void keys(Device& d, std::initializer_list<DKey> ks) {
  for (DKey k : ks) d.onKey(k);
}
void type(Device& d, const char* s) {  // keysForChar() characters (j = ENG = i, @ = SHIFT (−) = ∠)
  for (; *s; ++s) {
    DKey k[2];
    int n = 0;
    if (calc::keysForChar(*s, k, n))
      for (int i = 0; i < n; ++i) d.onKey(k[i]);
  }
}
// Does the screen show `s` at character cell (col, row)? (Drawn alone and compared pixel by pixel.)
bool shows(const Device& d, int col, int row, const std::string& s) {
  static Framebuffer fb, ref;
  fb.clear();
  d.render(fb);
  ref.clear();
  ref.drawText(col, row, s);
  for (int x = col * 5; x < (col + textLength(s)) * 5 && x < Framebuffer::kWidth; ++x)
    for (int y = row * 8; y < row * 8 + 8; ++y)
      if (fb.get(x, y) != ref.get(x, y)) return false;
  return true;
}

void testDevice() {
  using K = DKey;
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode});
    snapshot("cmplx_menu_mode", d);
    keys(d, {K::D6});
    XCHECK(d.view() == View::Calc && d.mode() == Mode::Cmplx);
    type(d, "(3+4j)(1-2j)");
    snapshot("cmplx_typing", d);
    type(d, "=");
    XEQ(d.resultText(), "11" + M + "2i");
    XCHECK(d.vars().ans.v == 11 && d.imagVars().ans.v == -2 && d.resultImag().v == -2);
    snapshot("cmplx_result", d);
    // Ans carries on: "Ans×i".
    type(d, "*j=");
    XEQ(d.resultText(), "2+11i");
    type(d, "a+1=");  // Ans + 1
    XEQ(d.resultText(), "3+11i");
    // The replay keeps complex values.
    keys(d, {K::Up});
    XEQ(d.resultText(), "2+11i");
    keys(d, {K::Up});
    XEQ(d.resultText(), "11" + M + "2i");
    keys(d, {K::AC});
    // SHIFT 2: the CMPLX menu.
    keys(d, {K::Shift, K::D2});
    XCHECK(d.view() == View::CmplxMenu);
    snapshot("cmplx_shift2_menu", d);
    keys(d, {K::D2});
    XCHECK(d.view() == View::Calc && d.expression().size() == 1 && d.expression()[0] == Tok::Conjg);
    type(d, "3+4j)=");
    XEQ(d.resultText(), "3" + M + "4i");
    keys(d, {K::Shift, K::D2, K::D1});
    type(d, "j)=");
    XEQ(d.resultText(), "90");
    keys(d, {K::Shift, K::Hyp});  // Abs
    type(d, "3+4j=");
    XEQ(d.resultText(), "5");
    keys(d, {K::Shift, K::D2, K::AC});
    XCHECK(d.view() == View::Calc);
    // ∠ (SHIFT (−)) and the conversions.
    type(d, "2@90=");
    XEQ(d.resultText(), "2i");
    type(d, "3+4j");
    keys(d, {K::Shift, K::D2, K::D3});
    type(d, "=");
    XEQ(d.resultText(), "5" + ANG + "53.13010235");
    snapshot("cmplx_polar_result", d);
    keys(d, {K::Shift, K::D2, K::D4});  // after a result: "Ans▶a+bi"
    XCHECK(d.expression().size() == 2 && d.expression()[0] == Tok::Ans && d.expression()[1] == Tok::ToRect);
    type(d, "=");
    XEQ(d.resultText(), "3+4i");
    // STO / RCL / M+ with complex values.
    type(d, "1+j");
    keys(d, {K::Shift, K::Rcl, K::Neg});  // STO A
    XCHECK(d.vars().a.v == 1 && d.imagVars().a.v == 1);
    XEQ(d.resultText(), "1+i");
    type(d, "$2]~=");  // 2A
    XEQ(d.resultText(), "2+2i");
    keys(d, {K::Rcl, K::Neg});
    XEQ(d.resultText(), "1+i");
    type(d, "$j");
    keys(d, {K::MPlus});
    XCHECK(d.vars().m.v == 0 && d.imagVars().m.v == 1);
    XCHECK(shows(d, 3, 0, "M"));
    // The keys COMP gives other jobs: ∫dx, d/dx, CALC, SOLVE do nothing in CMPLX; SHIFT ENG either.
    type(d, "$1+jIDCS");
    XCHECK(d.view() == View::Calc && d.expression().size() == 3);
    keys(d, {K::Shift, K::Eng});
    XCHECK(d.expression().size() == 3);
    // √(−1) is a Math ERROR, as on the Casio.
    type(d, "$r~1=");
    XCHECK(d.view() == View::Error && d.errorMessage() == "Math ERROR");
    keys(d, {K::AC});
    type(d, "s1+j)=");
    XCHECK(d.view() == View::Error && d.errorMessage() == "Math ERROR");
    keys(d, {K::Left});  // Goto
    XCHECK(d.view() == View::Calc);
    // The status bar says CMPLX.
    keys(d, {K::AC});
    XCHECK(shows(d, 11, 0, "CMPLX"));
    // Leaving CMPLX: only the real parts stay, the replay is cleared.
    type(d, "5+6j=");
    keys(d, {K::Mode, K::D1});
    XCHECK(d.mode() == Mode::Comp && d.vars().ans.v == 5 && !d.imagVars().any());
    XCHECK(d.vars().a.v == 1);  // A keeps its real part
    keys(d, {K::Up});
    XCHECK(!d.showingResult());  // no replay
    XCHECK(!shows(d, 11, 0, "CMPLX") && shows(d, 11, 0, "     "));
    // In COMP, ENG is ENG again and SHIFT (−) / SHIFT 2 type nothing.
    type(d, "1000=");
    keys(d, {K::Eng});
    XEQ(d.resultText(), "1\xC3\x97" "10\xC2\xB3");
    keys(d, {K::AC, K::Shift, K::Neg, K::Shift, K::D2});
    XCHECK(d.expression().empty() && d.view() == View::Calc);
  }
  {  // SETUP ▼ 2:CMPLX: r∠θ; long results split with R⇔I and SHIFT =.
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::Mode, K::Down});
    snapshot("cmplx_setup_page2", d);
    keys(d, {K::D2});
    XCHECK(d.view() == View::SetupCmplx);
    snapshot("cmplx_setup", d);
    keys(d, {K::D2});
    XCHECK(d.complexPolar() && d.view() == View::Calc);
    keys(d, {K::Mode, K::D6});
    type(d, "3+4j=");
    XEQ(d.resultText(), "5" + ANG + "53.13010235");
    type(d, "2+3=");
    XEQ(d.resultText(), "5");                         // a real result shows as a real
    keys(d, {K::Shift, K::Mode, K::Down, K::D2, K::D1});
    XCHECK(!d.complexPolar());
    type(d, "1E~20/3-2j/3=");
    // The real part (an inexact 3.3E-21) is below 1E-14 of the imaginary one: dropped as noise.
    XEQ(d.resultText(), M + "2" + FR + "3i");
    type(d, "1E~5/3-2j/3=");
    keys(d, {K::SD});  // decimals: 3.333333333×10⁻⁶−0.6666666667i, 30 characters
    XCHECK(d.complexSplit() && !d.complexImagShown());
    XEQ(d.resultText(), "3.333333333\xC3\x97" "10\xE2\x81\xBB\xE2\x81\xB6");
    snapshot("cmplx_split_real", d);
    keys(d, {K::Shift, K::Eq});  // SHIFT = : Re⇔Im
    XCHECK(d.complexImagShown());
    XEQ(d.resultText(), M + "0.6666666667i");
    snapshot("cmplx_split_imag", d);
    keys(d, {K::Shift, K::Eq});
    XCHECK(!d.complexImagShown());
    keys(d, {K::SD});  // fractions: 1⌟300000−2⌟3i fits one row
    XCHECK(!d.complexSplit());
    XEQ(d.resultText(), "1" + FR + "300000" + M + "2" + FR + "3i");
  }
  {  // exam mode allows CMPLX (a calculator mode)
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::Mode, K::D8, K::Eq});
    XCHECK(d.examActive());
    keys(d, {K::AC, K::Mode, K::D6});
    XCHECK(d.mode() == Mode::Cmplx && d.view() == View::Calc);
    type(d, "(1+j)q=");
    XEQ(d.resultText(), "2i");
    snapshot("cmplx_exam", d);
    keys(d, {K::Mode, K::D4});  // the apps stay off
    XCHECK(d.view() == View::Notice);
    keys(d, {K::AC});
    XCHECK(d.view() == View::Calc && d.mode() == Mode::Cmplx);
  }
  {  // MODE 4 (Apps) and back: CMPLX and its complex Ans stay under the apps.
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D6});
    type(d, "j=");
    keys(d, {K::Mode, K::D4});
    XCHECK(d.view() == View::Apps);
    keys(d, {K::AC});
    XCHECK(d.view() == View::Calc && d.mode() == Mode::Cmplx && d.imagVars().ans.v == 1);
    keys(d, {K::Mode, K::D5});  // EQN: leaving CMPLX
    XCHECK(d.mode() == Mode::Eqn && !d.imagVars().any());
  }
  {  // saveState(): the mode, the r∠θ choice, complex memories and the replay survive.
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::Mode, K::Down, K::D2, K::D2});
    keys(d, {K::Mode, K::D6});
    type(d, "1+2j");
    keys(d, {K::Shift, K::Rcl, K::Dms});  // STO B
    type(d, "3+4j=");
    type(d, "j");
    keys(d, {K::Shift, K::D2, K::D4});
    type(d, "=");
    const std::string s = d.saveState();
    XCHECK(s.size() <= 2048);
    Device e;
    e.tick(0);
    XCHECK(e.restoreState(s) && e.mode() == Mode::Cmplx && e.view() == View::Calc && e.complexPolar());
    XCHECK(e.vars().b.v == 1 && e.imagVars().b.v == 2 && e.vars().ans.v == 0 && e.imagVars().ans.v == 1);
    XCHECK(e.imagVars().preAns.v == 4);
    keys(e, {K::Up});
    XEQ(e.resultText(), "i");                          // ▶a+bi kept with the replay item
    keys(e, {K::Up});
    XEQ(e.resultText(), "5" + ANG + "53.13010235");   // the r∠θ choice
    keys(e, {K::Up});
    XEQ(e.resultText(), "5" + ANG + "53.13010235");   // the oldest: stays (STO adds no replay)
    // The r∠θ choice also survives in COMP (a SETUP flag).
    Device c;
    c.tick(0);
    keys(c, {K::Shift, K::Mode, K::Down, K::D2, K::D2});
    Device c2;
    c2.tick(0);
    XCHECK(c2.restoreState(c.saveState()) && c2.complexPolar() && c2.mode() == Mode::Comp);
    // Still within 2 KB with a long complex replay.
    Device h;
    h.tick(0);
    keys(h, {K::Mode, K::D6});
    for (int i = 0; i < 40; ++i) type(h, "123456789+987654321j=");
    const std::string hs = h.saveState();
    Device h2;
    h2.tick(0);
    XCHECK(hs.size() <= 2048 && h2.restoreState(hs) && h2.mode() == Mode::Cmplx && h2.imagVars().ans.v == 987654321);
    keys(h2, {K::Up});
    XEQ(h2.resultText(), "123456789+987654321i");
    // Damaged 'C' blocks are refused (re-signed so the checksum passes).
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
      // Layout of s's tail: 'C', 11 × 25 bytes, the count (2), 2 × (25 + 1), the checksum.
      const size_t tag = s.size() - 4 - 2 * 26 - 1 - 11 * 25 - 1;
      XCHECK(s[tag] == 'C' && s[tag + 1 + 11 * 25] == 2);
      Device x;
      x.tick(0);
      std::string bad = s;
      bad[tag + 1 + 11 * 25] = 1;  // a count that isn't the replay's
      XCHECK(!x.restoreState(resign(bad)) && x.mode() == Mode::Comp);
      bad = s;
      bad[s.size() - 4 - 1] = 7;  // a form byte that is neither 0, 1 nor 0xFF
      XCHECK(!x.restoreState(resign(bad)));
      bad = s;
      for (int i = 0; i < 8; ++i) bad[tag + 1 + i] = static_cast<char>(0xFF);  // A's imaginary part a NaN
      XCHECK(!x.restoreState(resign(bad)));
      bad = s;
      bad[tag] = 'E';  // another mode's block under mode CMPLX
      XCHECK(!x.restoreState(resign(bad)));
      bad = s;
      bad[1] = 6;  // mode byte BASE-N with CMPLX's block (basen_tests: a mode byte beyond BaseN)
      XCHECK(!x.restoreState(resign(bad)));
      XCHECK(x.restoreState(resign(s)));
    }
  }
}

}  // namespace

int runCmplxTests() {
  testArithmetic();
  testRealSameAsComp();
  testFunctions();
  testErrors();
  testMemoriesAndFormats();
  testDevice();
  std::printf("cmplx: %d checks, %d failed\n", g_checks, g_fail);
  return g_fail;
}
