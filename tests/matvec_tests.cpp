// Unit tests for MATRIX (MODE 8) and VECTOR (MODE 9) (core/matvec.*): the evaluator (the fx-115ES
// PLUS guide's examples, det / inverse / transpose / powers, Abs, the dot and cross products,
// scalars, Dim / Math / Syntax / Stack ERRORs and their positions), the element display, the mode's
// screens (Dim's lists, the editors, the MatAns screen, SHIFT 4 / SHIFT 5's menus, STO copies) and
// the Device integration (MODE menu, the MAT / VCT tag, results, replay, memories, exam mode,
// leaving, saveState()). runMatVecTests() returns the number of failures; tests.cpp counts them.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include "../core/device.h"
#include "../core/font.h"
#include "../core/framebuffer.h"
#include "../core/matvec.h"
#include "../core/text.h"

using namespace calc;

namespace {

int g_fail = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
  ++g_checks;
  if (!ok) {
    ++g_fail;
    std::printf("FAIL matvec_tests.cpp:%d: %s\n", line, what);
  }
}
#define MCHECK(cond) check((cond), #cond, __LINE__)
#define MEQ(got, want)                                                                                     \
  do {                                                                                                     \
    const std::string g_ = (got), w_ = (want);                                                             \
    if (g_ != w_) std::printf("  matvec_tests.cpp:%d: got \"%s\", want \"%s\"\n", __LINE__, g_.c_str(), w_.c_str()); \
    check(g_ == w_, #got " == " #want, __LINE__);                                                          \
  } while (0)

const std::string M = "\xE2\x88\x92";  // −
const std::string FR = "\xE2\x8C\x9F";  // ⌟

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
  MCHECK(same);
}

// Test-only text → tokens: digits . + - * / ( ) , the words MatA MatB MatC MatAns VctA ... VctAns
// det( Trn( Abs( sin( cos-1( Ans, and @ the dot product, ~ (−), ^ x^(, q x², c x³, i x⁻¹, _ ⌟.
std::vector<Tok> tk(const std::string& s) {
  struct W {
    const char* w;
    Tok t;
  };
  static const W words[] = {{"MatAns", Tok::MatAns}, {"MatA", Tok::MatA}, {"MatB", Tok::MatB}, {"MatC", Tok::MatC},
                            {"VctAns", Tok::VctAns}, {"VctA", Tok::VctA}, {"VctB", Tok::VctB}, {"VctC", Tok::VctC},
                            {"det(", Tok::Det},      {"Trn(", Tok::Trn},  {"Abs(", Tok::Abs},  {"sin(", Tok::Sin},
                            {"cos-1(", Tok::Acos},   {"Ans", Tok::Ans},   {"@", Tok::DotP},    {"~", Tok::Neg},
                            {"^", Tok::Pow},         {"q", Tok::Sq},      {"c", Tok::Cube},    {"i", Tok::Inv},
                            {"_", Tok::Frac},        {"+", Tok::Add},     {"-", Tok::Sub},     {"*", Tok::Mul},
                            {"/", Tok::Div},         {"(", Tok::Open},    {")", Tok::Close},   {",", Tok::Comma},
                            {".", Tok::Dot},         {"!", Tok::Fact},    {"=", Tok::Eq},      {"I", Tok::Integ}};
  std::vector<Tok> out;
  for (size_t i = 0; i < s.size();) {
    const char c = s[i];
    if (c >= '0' && c <= '9') {
      out.push_back(static_cast<Tok>(c - '0'));
      ++i;
      continue;
    }
    bool hit = false;
    for (const W& w : words) {
      const size_t n = std::char_traits<char>::length(w.w);
      if (s.compare(i, n, w.w) == 0) {
        out.push_back(w.t);
        i += n;
        hit = true;
        break;
      }
    }
    if (!hit) {
      std::printf("matvec tk: can't read '%s' at %zu\n", s.c_str(), i);
      ++g_fail;
      return out;
    }
  }
  return out;
}

Mat mk(int rows, int cols, std::initializer_list<double> v) {
  Mat m = Mat::zero(rows, cols);
  int i = 0;
  for (double x : v) {
    m.at(i / cols, i % cols) = x == std::floor(x) && std::fabs(x) < 1e15 ? Num::ofInt(static_cast<long long>(x)) : Num::ofDouble(x);
    ++i;
  }
  return m;
}

// The guide's examples: MatA = [[2,1],[1,1]], MatB = [[2,−1],[−1,2]], MatC = [[1,0,−1],[0,−1,1]];
// VctA = (1,2), VctB = (3,4), VctC = (2,−1,2).
struct Env {
  Mat mats[4], vcts[4];
  Vars vars;
  Env() {
    mats[0] = mk(2, 2, {2, 1, 1, 1});
    mats[1] = mk(2, 2, {2, -1, -1, 2});
    mats[2] = mk(2, 3, {1, 0, -1, 0, -1, 1});
    vcts[0] = mk(1, 2, {1, 2});
    vcts[1] = mk(1, 2, {3, 4});
    vcts[2] = mk(1, 3, {2, -1, 2});
  }
};

double fixedRandom() { return 0.25; }

MvResult ev(Env& e, const std::string& s, bool vec = false, AngleUnit u = AngleUnit::Deg) {
  return evaluateMatVec(tk(s), e.vars, vec ? e.vcts : e.mats, vec, u, fixedRandom);
}

// A result as text: "[[3,0],[1,1]]" (fractions as the editor shows them: "3⌟2"), a scalar as
// COMP shows it, or the error message.
std::string matText(const Mat& m) {
  std::string o = m.rows == 1 ? "(" : "[";
  for (int r = 0; r < m.rows; ++r) {
    if (m.rows > 1) o += r ? ",[" : "[";
    for (int c = 0; c < m.cols; ++c) o += (c ? "," : "") + formatResult(m.at(r, c), true, NormMode::Norm1);
    if (m.rows > 1) o += "]";
  }
  return o + (m.rows == 1 ? ")" : "]");
}
std::string show(Env& e, const std::string& s, bool vec = false, AngleUnit u = AngleUnit::Deg) {
  const MvResult r = ev(e, s, vec, u);
  if (r.error != CalcError::None) return errorText(r.error);
  if (r.kind == MvKind::Scalar) return formatResult(r.scalar, true, NormMode::Norm1);
  if ((r.kind == MvKind::Vector) != vec) return "WRONG KIND";
  return matText(r.m);
}

// ---------------------------------------------------------------- the evaluator
void testMatrixMaths() {
  Env e;
  // The guide's MATRIX examples 1-8.
  MEQ(show(e, "MatA*MatB"), "[[3,0],[1,1]]");
  MEQ(show(e, "MatA+MatB"), "[[4,0],[0,3]]");
  MEQ(show(e, "3*MatA"), "[[6,3],[3,3]]");
  MEQ(show(e, "3MatA"), "[[6,3],[3,3]]");  // implicit ×, as the guide keys it
  MEQ(show(e, "det(MatA)"), "1");
  MEQ(show(e, "Trn(MatC)"), "[[1,0],[0," + M + "1],[" + M + "1,1]]");
  MEQ(show(e, "MatAi"), "[[1," + M + "1],[" + M + "1,2]]");
  MEQ(show(e, "Abs(MatB)"), "[[2,1],[1,2]]");
  MEQ(show(e, "MatAq"), "[[5,3],[3,2]]");
  MEQ(show(e, "MatAc"), "[[13,8],[8,5]]");
  // More: −, k × and × k, ÷ k, (−), products of other sizes, MatAns.
  MEQ(show(e, "MatA-MatB"), "[[0,2],[2," + M + "1]]");
  MEQ(show(e, "MatA*2"), "[[4,2],[2,2]]");
  MEQ(show(e, "MatA/2"), "[[1,1" + FR + "2],[1" + FR + "2,1" + FR + "2]]");
  MEQ(show(e, "~MatA"), "[[" + M + "2," + M + "1],[" + M + "1," + M + "1]]");
  MEQ(show(e, "MatA*MatC"), "[[2," + M + "1," + M + "1],[1," + M + "1,0]]");
  MEQ(show(e, "MatC*Trn(MatC)"), "[[2," + M + "1],[" + M + "1,2]]");
  MEQ(show(e, "Trn(MatC)MatC"), "[[1,0," + M + "1],[0,1," + M + "1],[" + M + "1," + M + "1,2]]");
  MEQ(show(e, "(MatA+MatB)(MatA-MatB)"), "[[0,8],[6," + M + "3]]");
  MEQ(show(e, "MatA*MatAi"), "[[1,0],[0,1]]");
  MEQ(show(e, "2*3*MatA/6"), "[[2,1],[1,1]]");
  e.mats[3] = mk(2, 2, {1, 2, 3, 4});
  MEQ(show(e, "MatAns+MatA"), "[[3,3],[4,5]]");
  // Inverse: exact fractions (the Casio shows 3⌟2 and −1⌟2).
  MEQ(show(e, "MatAnsi"), "[[" + M + "2,1],[3" + FR + "2," + M + "1" + FR + "2]]");
  MEQ(show(e, "det(MatAns)"), M + "2");
  // 3 × 3: det, inverse, A·A⁻¹ = I exactly; a singular one: det 0 and Math ERROR.
  e.mats[0] = mk(3, 3, {2, -1, 0, 1, 3, 2, 0, 1, 4});
  MEQ(show(e, "det(MatA)"), "24");
  MEQ(show(e, "MatA*MatAi"), "[[1,0,0],[0,1,0],[0,0,1]]");
  MEQ(show(e, "MatAi*MatA"), "[[1,0,0],[0,1,0],[0,0,1]]");
  MEQ(show(e, "det(MatAi)"), "1" + FR + "24");
  MEQ(show(e, "Trn(Trn(MatA))-MatA"), "[[0,0,0],[0,0,0],[0,0,0]]");
  MEQ(show(e, "det(Trn(MatA))"), "24");
  e.mats[1] = mk(3, 3, {1, 2, 3, 4, 5, 6, 7, 8, 9});
  MEQ(show(e, "det(MatB)"), "0");
  MEQ(show(e, "MatBi"), "Math ERROR");
  // Decimals: the inverse within rounding, a near-singular determinant's noise is 0.
  e.mats[1] = mk(3, 3, {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9});
  MEQ(show(e, "det(MatB)"), "0");
  MEQ(show(e, "MatBi"), "Math ERROR");
  e.mats[1] = mk(3, 3, {0.5, 1.25, 3, 2.2, 0.1, 7, 1, 1, 1.5});
  {
    const MvResult r = ev(e, "MatB*MatBi");
    MCHECK(r.error == CalcError::None && r.kind == MvKind::Matrix);
    double worst = 0;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) worst = std::fmax(worst, std::fabs(r.m.at(i, j).v - (i == j ? 1 : 0)));
    MCHECK(worst < 1e-12);
  }
  // 1 × 1 and the helpers on their own.
  {
    Num d;
    Mat inv;
    CalcError err = CalcError::None;
    MCHECK(matDet(mk(1, 1, {-4}), d, err) && d.v == -4);
    MCHECK(matInverse(mk(1, 1, {4}), inv, err) && formatResult(inv.at(0, 0), true, NormMode::Norm1) == "1" + FR + "4");
    MCHECK(!matDet(mk(2, 3, {1, 2, 3, 4, 5, 6}), d, err) && err == CalcError::Dim);
    MCHECK(!matInverse(mk(2, 2, {1, 2, 2, 4}), inv, err) && err == CalcError::Math);
    MCHECK(!matInverse(mk(3, 2, {1, 2, 2, 4, 5, 6}), inv, err) && err == CalcError::Dim);
  }
  // Scalars still calculate as in COMP (and read Ans and the memories).
  e.vars.ans = Num::ofInt(5);
  MEQ(show(e, "1/3+1/6"), "1" + FR + "2");
  MEQ(show(e, "Ans*2+sin(30)"), "21" + FR + "2");
  MEQ(show(e, "det(MatC*Trn(MatC))+Ans"), "8");
}

void testMatrixErrors() {
  Env e;
  // Dim ERROR: sizes that don't agree; non-square det / inverse / powers.
  MEQ(show(e, "MatA+MatC"), "Dim ERROR");
  MEQ(show(e, "MatC-MatA"), "Dim ERROR");
  MEQ(show(e, "MatC*MatC"), "Dim ERROR");
  MEQ(show(e, "det(MatC)"), "Dim ERROR");
  MEQ(show(e, "MatCi"), "Dim ERROR");
  MEQ(show(e, "MatCq"), "Dim ERROR");
  MEQ(show(e, "MatCc"), "Dim ERROR");
  // A variable without a size: Dim ERROR, at its token (Goto).
  e.mats[3] = Mat();
  {
    const MvResult r = ev(e, "1+MatAns");
    MCHECK(r.error == CalcError::Dim && r.errorPos == 2);
  }
  {
    Env f;
    f.mats[1] = Mat();
    MEQ(show(f, "MatA*MatB"), "Dim ERROR");
  }
  // Syntax ERROR: a matrix where only a scalar goes (the guide's ^ note, functions, ÷ M ...).
  MEQ(show(e, "3+MatA"), "Syntax ERROR");
  MEQ(show(e, "MatA-1"), "Syntax ERROR");
  MEQ(show(e, "MatA^(2)"), "Syntax ERROR");
  MEQ(show(e, "2^(MatA)"), "Syntax ERROR");
  MEQ(show(e, "sin(MatA)"), "Syntax ERROR");
  MEQ(show(e, "MatA/MatB"), "Syntax ERROR");
  MEQ(show(e, "1/MatA"), "Syntax ERROR");
  MEQ(show(e, "MatA_2"), "Syntax ERROR");
  MEQ(show(e, "MatA!"), "Syntax ERROR");
  MEQ(show(e, "det(3)"), "Syntax ERROR");
  MEQ(show(e, "Trn(3)"), "Syntax ERROR");
  MEQ(show(e, "MatA@MatB"), "Syntax ERROR");  // · is VECTOR's
  MEQ(show(e, "VctA"), "Syntax ERROR");       // VECTOR's variables in MATRIX
  MEQ(show(e, "I(1,2,3)"), "Syntax ERROR");   // ∫ is COMP's
  MEQ(show(e, ""), "Syntax ERROR");
  MEQ(show(e, "MatA+"), "Syntax ERROR");
  MEQ(show(e, "1=2"), "Syntax ERROR");
  // Math ERROR: an element out of range; ÷ 0.
  e.mats[0] = mk(2, 2, {1e60, 1, 1, 1});
  MEQ(show(e, "MatAq"), "Math ERROR");
  MEQ(show(e, "MatA/0"), "Math ERROR");
  // The shared nesting budget: auto-closed brackets, one level too many = Stack ERROR.
  {
    Env f;
    std::string deep;
    for (int i = 0; i < kMaxNesting; ++i) deep += "(";
    MEQ(show(f, deep + "MatA"), "[[2,1],[1,1]]");
    MEQ(show(f, "(" + deep + "MatA"), "Stack ERROR");
  }
  // The matrix stack: kMvSlots matrices held at once; one more = Stack ERROR.
  {
    Env f;
    std::string s, close;
    for (int i = 1; i < kMvSlots; ++i) s += "MatA+(";
    MEQ(show(f, s + "MatA"), "[[16,8],[8,8]]");
    MEQ(show(f, s + "MatA+(MatA"), "Stack ERROR");
    // ... but a long chain reuses its slots.
    std::string chain = "MatA";
    for (int i = 0; i < 30; ++i) chain += "+MatA";
    MEQ(show(f, chain), "[[62,31],[31,31]]");
  }
  // Error positions for Goto: the operator's right operand (Dim) / the token (Syntax).
  {
    const MvResult r = ev(e, "MatA+MatC");
    MCHECK(r.error == CalcError::Dim && r.errorPos >= 2 && r.errorPos <= 3);
    const MvResult s = ev(e, "1+)");
    MCHECK(s.error == CalcError::Syntax && s.errorPos == 2);
  }
}

void testVectorMaths() {
  Env e;
  // The guide's VECTOR examples 1-7.
  MEQ(show(e, "VctA+VctB", true), "(4,6)");
  MEQ(show(e, "3*VctA", true), "(3,6)");
  e.vcts[3] = mk(1, 2, {3, 6});
  MEQ(show(e, "VctAns-VctB", true), "(0,2)");
  MEQ(show(e, "VctA@VctB", true), "11");
  MEQ(show(e, "Abs(VctC)", true), "3");
  {
    const MvResult r = ev(e, "cos-1(VctA@VctB/(Abs(VctA)Abs(VctB)))", true);
    MCHECK(r.error == CalcError::None && r.kind == MvKind::Scalar && std::fabs(r.scalar.v - 10.30484647) < 1e-7);
  }
  // Cross product (3D only), its properties; 2D × 2D: Dim ERROR (no cross product there).
  e.vcts[0] = mk(1, 3, {1, 0, 0});
  e.vcts[1] = mk(1, 3, {0, 1, 0});
  MEQ(show(e, "VctA*VctB", true), "(0,0,1)");
  MEQ(show(e, "VctB*VctA", true), "(0,0," + M + "1)");
  e.vcts[0] = mk(1, 3, {1, 2, 3});
  e.vcts[1] = mk(1, 3, {4, 5, 6});
  MEQ(show(e, "VctA*VctB", true), "(" + M + "3,6," + M + "3)");
  MEQ(show(e, "(VctA*VctB)@VctA", true), "0");
  MEQ(show(e, "VctA*VctA", true), "(0,0,0)");
  MEQ(show(e, "VctA@VctB*2", true), "64");  // · and × left to right: (A·B)×2
  MEQ(show(e, "Abs(VctA)q", true), "14");
  MEQ(show(e, "VctA/2", true), "(1" + FR + "2,1,3" + FR + "2)");
  MEQ(show(e, "~VctA", true), "(" + M + "1," + M + "2," + M + "3)");
  e.vcts[0] = mk(1, 2, {1, 2});
  e.vcts[1] = mk(1, 2, {3, 4});
  MEQ(show(e, "VctA*VctB", true), "Dim ERROR");
  MEQ(show(e, "VctA@VctC", true), "Dim ERROR");
  MEQ(show(e, "VctA+VctC", true), "Dim ERROR");
  MEQ(show(e, "VctA*VctC", true), "Dim ERROR");
  // Syntax: a scalar added, powers, functions, MATRIX's tokens.
  MEQ(show(e, "VctA+1", true), "Syntax ERROR");
  MEQ(show(e, "VctAq", true), "Syntax ERROR");
  MEQ(show(e, "VctAi", true), "Syntax ERROR");
  MEQ(show(e, "det(VctA)", true), "Syntax ERROR");
  MEQ(show(e, "Trn(VctA)", true), "Syntax ERROR");
  MEQ(show(e, "MatA", true), "Syntax ERROR");
  MEQ(show(e, "2@3", true), "Syntax ERROR");
  MEQ(show(e, "VctA/VctB", true), "Syntax ERROR");
  e.vcts[2] = Mat();
  MEQ(show(e, "VctC", true), "Dim ERROR");
  // Abs of a vector: exact when it is, else 10 digits.
  e.vcts[0] = mk(1, 2, {1, 1});
  MEQ(show(e, "Abs(VctA)", true), "1.414213562");
  // The element display: a fraction when it fits, else as many digits as fit.
  MEQ(mvCellText(Num::ofDouble(0.5), 7), "1" + FR + "2");
  MEQ(mvCellText(Num::ofInt(-12), 7), "-12");
  MEQ(mvCellText(Num::ofDouble(1.0 / 7), 7), "1" + FR + "7");
  MEQ(mvCellText(Num::ofDouble(3.14159265358979), 7), "3.14159");
  MEQ(mvCellText(Num::ofDouble(123456789012.0), 7), "1.23E11");
  // Every glyph the modes draw is in the font.
  for (const std::string s : {std::string("MatA MatB MatC MatAns VctA VctB VctC VctAns det( Trn( \xC2\xB7 [ ] MAT VCT"),
                              std::string("8:MATRIX 9:VECTOR Matrix? Vector? m\xC3\x97n? Dim ERROR")})
    for (uint32_t cp : decodeUtf8(s)) MCHECK(hasGlyph(cp));
  MEQ(tokText(Tok::DotP), "\xC2\xB7");
  MCHECK(tokOpensBracket(Tok::Det) && tokOpensBracket(Tok::Trn) && !tokOpensBracket(Tok::MatA));
  // COMP's evaluate() never takes the new tokens.
  Vars v;
  MCHECK(evaluate(tk("MatA"), v, AngleUnit::Deg, fixedRandom).error == CalcError::Syntax);
  MCHECK(evaluate(tk("det(2)"), v, AngleUnit::Deg, fixedRandom).error == CalcError::Syntax);
  MEQ(errorText(CalcError::Dim), "Dim ERROR");
}

// ---------------------------------------------------------------- Device
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
std::string cell(const Device& d, int i, int r, int c) {
  return formatResult(d.matVec().var(i).at(r, c), true, NormMode::Norm1);
}

void testMatrixDevice() {
  using K = DKey;
  using S = MatVecApp::Screen;
  Device d;
  d.tick(0);
  keys(d, {K::Mode});
  snapshot("matvec_menu_mode", d);
  MCHECK(shows(d, 0, 4, "7:BASE-N") && shows(d, 11, 4, "8:MATRIX") && shows(d, 0, 5, "9:VECTOR"));
  // MODE 8: Dim's "Matrix?" list first (as the Casio), MAT in the status bar.
  keys(d, {K::D8});
  MCHECK(d.mode() == Mode::Matrix && d.view() == View::MatVec && d.matVec().screen() == S::Select);
  MCHECK(shows(d, 11, 0, "MAT") && shows(d, 0, 1, "Matrix?"));
  snapshot("matvec_select", d);
  keys(d, {K::D1});  // MatA: its sizes, two pages
  MCHECK(d.matVec().screen() == S::Dim && shows(d, 24, 0, "\xE2\x96\xBC"));
  snapshot("matvec_dim", d);
  keys(d, {K::Down});
  MCHECK(d.matVec().dimPage() == 1 && shows(d, 23, 0, "\xE2\x96\xB2"));
  snapshot("matvec_dim2", d);
  keys(d, {K::Up, K::D5});  // 2×2
  MCHECK(d.matVec().screen() == S::Editor && d.matVec().editing() == 0);
  MCHECK(d.matVec().var(0).rows == 2 && d.matVec().var(0).cols == 2);
  type(d, "2=1=1=1=");  // the guide's MatA, row by row
  MEQ(cell(d, 0, 0, 0), "2");
  MEQ(cell(d, 0, 1, 1), "1");
  MCHECK(d.matVec().curRow() == 1 && d.matVec().curCol() == 1);  // stays on the last element
  keys(d, {K::Shift, K::Up});  // SHIFT ▲ is the platform's (brightness): the cursor stays
  MCHECK(d.matVec().curRow() == 1);
  snapshot("matvec_editor", d);
  // Editing: arrows stop at the edges; an expression's value (exact); DEL zeroes; errors.
  keys(d, {K::Up, K::Up, K::Left, K::Left, K::Left});
  MCHECK(d.matVec().curRow() == 0 && d.matVec().curCol() == 0);
  type(d, "1/3");
  MCHECK(d.matVec().typing());
  snapshot("matvec_editor_typing", d);
  type(d, "=");
  MEQ(cell(d, 0, 0, 0), "1" + FR + "3");
  MCHECK(d.matVec().curCol() == 1);
  keys(d, {K::Left, K::Del});
  MEQ(cell(d, 0, 0, 0), "0");
  type(d, "1/0=");
  MCHECK(d.matVec().screen() == S::Error && shows(d, 2, 1, "Math ERROR"));
  keys(d, {K::AC});
  MCHECK(d.matVec().screen() == S::Editor && !d.matVec().typing());
  type(d, "2=");
  // AC: the calculation screen.
  keys(d, {K::AC});
  MCHECK(d.view() == View::Calc && d.expression().empty());
  // SHIFT 4: the MATRIX menu. Data on MatB (no size yet): its sizes first.
  keys(d, {K::Shift, K::D4});
  MCHECK(d.view() == View::MatVec && d.matVec().screen() == S::Menu);
  snapshot("matvec_menu", d);
  keys(d, {K::D2, K::D2});
  MCHECK(d.matVec().screen() == S::Dim);
  keys(d, {K::D5});
  type(d, "2=~1=~1=2=");
  MEQ(cell(d, 1, 0, 1), M + "1");
  keys(d, {K::AC});
  // MatA × MatB: the MatAns screen with the guide's result.
  keys(d, {K::Shift, K::D4, K::D3, K::Mul, K::Shift, K::D4, K::D4});
  MCHECK(d.view() == View::Calc && d.expression().size() == 3 && d.expression()[0] == Tok::MatA &&
         d.expression()[2] == Tok::MatB);
  snapshot("matvec_calc_typing", d);
  type(d, "=");
  MCHECK(d.view() == View::MatVec && d.matVec().screen() == S::Editor && d.matVec().editing() == 3);
  MEQ(cell(d, 3, 0, 0), "3");
  MEQ(cell(d, 3, 0, 1), "0");
  MEQ(cell(d, 3, 1, 0), "1");
  MEQ(cell(d, 3, 1, 1), "1");
  MCHECK(shows(d, 0, 1, "MatAns(2\xC3\x97" "2)"));
  snapshot("matvec_matans", d);
  // The Ans screen is read only; arrows look round it.
  keys(d, {K::D7, K::Del, K::Right});
  MEQ(cell(d, 3, 0, 0), "3");
  MCHECK(d.matVec().curCol() == 1);
  // AC: the calculation screen, the expression with no value; = shows MatAns again.
  keys(d, {K::AC});
  MCHECK(d.view() == View::Calc && d.showingResult() && d.matrixResultShown() && d.resultText().empty());
  snapshot("matvec_calc_after", d);
  type(d, "=");
  MCHECK(d.view() == View::MatVec && d.matVec().editing() == 3);
  // + on the MatAns screen: "MatAns+" on the calculation screen.
  keys(d, {K::Add});
  MCHECK(d.view() == View::Calc && d.expression().size() == 2 && d.expression()[0] == Tok::MatAns &&
         d.expression()[1] == Tok::Add && !d.showingResult());
  keys(d, {K::Shift, K::D4, K::D3});  // MatA
  type(d, "=");
  MEQ(cell(d, 3, 0, 0), "5");
  MEQ(cell(d, 3, 1, 1), "2");
  // x² and x⁻¹ on the MatAns screen.
  keys(d, {K::Sq});
  MCHECK(d.expression().size() == 2 && d.expression()[1] == Tok::Sq);
  keys(d, {K::AC});
  // From the calculation screen after a matrix result, an operator carries on from MatAns.
  keys(d, {K::Shift, K::D4, K::D3});  // a fresh "MatA"
  type(d, "=");
  keys(d, {K::AC, K::Mul});
  MCHECK(d.expression().size() == 2 && d.expression()[0] == Tok::MatAns && d.expression()[1] == Tok::Mul);
  keys(d, {K::AC});
  // det: a scalar result, as in COMP, to Ans.
  keys(d, {K::Shift, K::D4, K::D7, K::Shift, K::D4, K::D3});
  type(d, "=");
  MCHECK(d.view() == View::Calc && !d.matrixResultShown());
  MEQ(d.resultText(), "1");  // det of the guide's MatA
  MCHECK(d.vars().ans.v == 1);
  // Inverse with x⁻¹; Trn; Dim ERROR on screen, with Goto.
  keys(d, {K::Shift, K::D4, K::D3, K::Inv});
  type(d, "=");
  MCHECK(d.view() == View::MatVec && d.matVec().editing() == 3);
  keys(d, {K::AC, K::AC});
  keys(d, {K::Shift, K::D4, K::D1, K::D3, K::D4});  // Dim MatC 2×3
  type(d, "1=0=~1=0=~1=1=");
  keys(d, {K::AC, K::Shift, K::D4, K::D3, K::Add, K::Shift, K::D4, K::D5});
  type(d, "=");
  MCHECK(d.view() == View::Error && d.errorMessage() == "Dim ERROR");
  snapshot("matvec_dim_error", d);
  keys(d, {K::Left});
  MCHECK(d.view() == View::Calc && !d.showingResult());
  keys(d, {K::AC, K::Shift, K::D4, K::D8, K::Shift, K::D4, K::D5});
  type(d, "=");
  MCHECK(d.view() == View::MatVec && d.matVec().var(3).rows == 3 && d.matVec().var(3).cols == 2);
  keys(d, {K::AC});
  // Dim on a variable that has data: a new size, every element 0 (the Casio's Dim).
  keys(d, {K::Shift, K::D4, K::D1, K::D1, K::D1});  // MatA 3×3
  MCHECK(d.matVec().var(0).rows == 3 && cell(d, 0, 0, 0) == "0" && cell(d, 0, 2, 2) == "0");
  keys(d, {K::Shift, K::D4, K::AC});  // the menu from an editor: AC goes back to it
  MCHECK(d.view() == View::MatVec && d.matVec().screen() == S::Editor && d.matVec().editing() == 0);
  // STO A / B / C in an editor copies what it shows (MatC → MatB here).
  keys(d, {K::AC, K::Shift, K::D4, K::D2, K::D3});  // Data MatC
  MCHECK(d.matVec().editing() == 2);
  keys(d, {K::Shift, K::Rcl});
  MCHECK(d.matVec().storing() && shows(d, 22, 1, "STO"));
  keys(d, {K::Dms});  // B
  MCHECK(d.matVec().editing() == 1 && d.matVec().var(1).rows == 2 && d.matVec().var(1).cols == 3);
  MEQ(cell(d, 1, 0, 2), M + "1");
  keys(d, {K::AC});
  // STO with a matrix result shown: Syntax ERROR (a memory holds a number).
  keys(d, {K::Shift, K::D4, K::D4});
  type(d, "=");
  keys(d, {K::AC, K::Shift, K::Rcl, K::Neg});
  MCHECK(d.view() == View::Error && d.errorMessage() == "Syntax ERROR");
  keys(d, {K::AC});
  // STO of a scalar works: det(MatB*Trn(MatB)) → A.
  keys(d, {K::Shift, K::D4, K::D7, K::Shift, K::D4, K::D4, K::Shift, K::D4, K::D8, K::Shift, K::D4, K::D4});
  keys(d, {K::Shift, K::Rcl, K::Neg});
  MCHECK(d.view() == View::Calc && d.vars().a.v == 3);  // [[1,0,−1],[0,−1,1]]·Trn = [[2,−1],[−1,2]]
  // ∫dx, d/dx, CALC and SOLVE do nothing; SHIFT 5 (VECTOR's) does nothing in MATRIX.
  keys(d, {K::AC, K::Integral, K::Calc, K::Shift, K::Calc, K::Shift, K::D5});
  MCHECK(d.view() == View::Calc && d.expression().empty());
  // The replay: a matrix item comes back without a value; = shows it again.
  keys(d, {K::Up});
  MCHECK(d.showingResult());
  for (int i = 0; i < 10 && !d.matrixResultShown(); ++i) keys(d, {K::Up});
  MCHECK(d.matrixResultShown() && d.resultText().empty());
  type(d, "=");
  MCHECK(d.view() == View::MatVec && d.matVec().editing() == 3);
  // ON: the calculation screen, cleared; MODE / AC comes back to the mode's screen.
  keys(d, {K::On});
  MCHECK(d.view() == View::Calc && d.expression().empty() && d.mode() == Mode::Matrix);
  keys(d, {K::Shift, K::D4, K::Mode});
  MCHECK(d.view() == View::ModeMenu);
  keys(d, {K::AC});
  MCHECK(d.view() == View::MatVec && d.matVec().screen() == S::Menu);
  keys(d, {K::AC});
  MCHECK(d.view() == View::Calc);
  // Leaving MATRIX: the variables and the replay are gone; Ans stays; COMP's keys again.
  const double ans = d.vars().ans.v;
  keys(d, {K::Mode, K::D1});
  MCHECK(d.mode() == Mode::Comp && !d.matVec().var(0).defined() && d.vars().ans.v == ans);
  MCHECK(!shows(d, 11, 0, "MAT"));
  keys(d, {K::Up});
  MCHECK(!d.showingResult());
  keys(d, {K::Shift, K::D4});
  MCHECK(d.view() == View::Calc);
  // Entering again: Dim's list, nothing kept.
  keys(d, {K::Mode, K::D8});
  MCHECK(d.view() == View::MatVec && d.matVec().screen() == S::Select && !d.matVec().var(2).defined());
  keys(d, {K::AC});
  MCHECK(d.view() == View::Calc);
  // A variable without a size on the calculation screen: Dim ERROR.
  keys(d, {K::Shift, K::D4, K::D6});
  type(d, "=");
  MCHECK(d.view() == View::Error && d.errorMessage() == "Dim ERROR");
}

void testVectorDevice() {
  using K = DKey;
  using S = MatVecApp::Screen;
  Device d;
  d.tick(0);
  keys(d, {K::Mode, K::D9});
  MCHECK(d.mode() == Mode::Vector && d.view() == View::MatVec && d.matVec().vectors());
  MCHECK(shows(d, 11, 0, "VCT") && shows(d, 0, 1, "Vector?"));
  snapshot("vector_select", d);
  keys(d, {K::D1});
  snapshot("vector_dim", d);
  keys(d, {K::D2});  // VctA, 2 dimensions
  type(d, "1=2=");
  MCHECK(d.matVec().var(0).rows == 1 && d.matVec().var(0).cols == 2);
  snapshot("vector_editor", d);
  keys(d, {K::AC, K::Shift, K::D5});
  MCHECK(d.matVec().screen() == S::Menu);
  snapshot("vector_menu", d);
  keys(d, {K::D1, K::D2, K::D2});  // Dim VctB 2
  type(d, "3=4=");
  keys(d, {K::AC, K::Shift, K::D5, K::D1, K::D3, K::D1});  // Dim VctC 3
  type(d, "2=~1=2=");
  keys(d, {K::AC});
  // VctA + VctB: the VctAns screen.
  keys(d, {K::Shift, K::D5, K::D3, K::Add, K::Shift, K::D5, K::D4});
  type(d, "=");
  MCHECK(d.view() == View::MatVec && d.matVec().editing() == 3);
  MEQ(cell(d, 3, 0, 0), "4");
  MEQ(cell(d, 3, 0, 1), "6");
  snapshot("vector_vctans", d);
  // x² is no VctAns key (read only); × is.
  keys(d, {K::Sq});
  MCHECK(d.view() == View::MatVec);
  keys(d, {K::Mul});
  MCHECK(d.view() == View::Calc && d.expression().size() == 2 && d.expression()[0] == Tok::VctAns);
  keys(d, {K::AC});
  // The dot product (SHIFT 5 7): 11, to Ans.
  keys(d, {K::Shift, K::D5, K::D3, K::Shift, K::D5, K::D7, K::Shift, K::D5, K::D4});
  MCHECK(d.expression().size() == 3 && d.expression()[1] == Tok::DotP);
  snapshot("vector_dot", d);
  type(d, "=");
  MEQ(d.resultText(), "11");
  MCHECK(d.vars().ans.v == 11);
  // Abs( (SHIFT hyp): the magnitude.
  keys(d, {K::Shift, K::Hyp, K::Shift, K::D5, K::D5});
  type(d, "=");
  MEQ(d.resultText(), "3");
  // 2D × 2D: Dim ERROR; VctC × VctC (3D): (0,0,0).
  keys(d, {K::Shift, K::D5, K::D3, K::Mul, K::Shift, K::D5, K::D4});
  type(d, "=");
  MCHECK(d.view() == View::Error && d.errorMessage() == "Dim ERROR");
  keys(d, {K::AC, K::Shift, K::D5, K::D5, K::Mul, K::Shift, K::D5, K::D5});
  type(d, "=");
  MCHECK(d.view() == View::MatVec && d.matVec().var(3).cols == 3);
  MEQ(cell(d, 3, 0, 2), "0");
  keys(d, {K::AC});
  // SHIFT 4 does nothing in VECTOR; the menu has no 8.
  keys(d, {K::AC, K::Shift, K::D4});
  MCHECK(d.view() == View::Calc);
  keys(d, {K::Shift, K::D5, K::D8});
  MCHECK(d.view() == View::MatVec && d.matVec().screen() == S::Menu);
  keys(d, {K::AC});
  // MODE 8 from VECTOR: MATRIX, nothing of VECTOR's kept.
  keys(d, {K::Mode, K::D8});
  MCHECK(d.mode() == Mode::Matrix && !d.matVec().vectors() && !d.matVec().var(0).defined());
}

void testExamAndApps() {
  using K = DKey;
  {  // exam mode allows MATRIX and VECTOR (calculator modes); the apps stay off
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::Mode, K::D8, K::Eq});
    MCHECK(d.examActive());
    keys(d, {K::AC, K::Mode});
    snapshot("matvec_menu_mode_exam", d);
    keys(d, {K::D8});
    MCHECK(d.mode() == Mode::Matrix && d.view() == View::MatVec);
    keys(d, {K::D1, K::D9});  // ▼ page 1 is not shown: 9 picks nothing
    MCHECK(d.matVec().screen() == MatVecApp::Screen::Dim);
    keys(d, {K::Down, K::D3});  // 1×1
    type(d, "4=");
    keys(d, {K::AC, K::Shift, K::D4, K::D3, K::Inv});
    type(d, "=");
    MCHECK(d.view() == View::MatVec && formatResult(d.matVec().var(3).at(0, 0), true, NormMode::Norm1) == "1" + FR + "4");
    snapshot("matvec_exam", d);
    keys(d, {K::AC, K::Mode, K::D4});
    MCHECK(d.view() == View::Notice);
    keys(d, {K::AC});
    MCHECK(d.mode() == Mode::Matrix);
    keys(d, {K::Mode, K::D9});
    MCHECK(d.mode() == Mode::Vector && d.view() == View::MatVec);
  }
  {  // MODE 4 (Apps) and back: MATRIX and its variables stay under the apps.
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D8, K::D1, K::D5});
    type(d, "7=");
    keys(d, {K::Mode, K::D4});
    MCHECK(d.view() == View::Apps);
    keys(d, {K::AC});
    MCHECK(d.view() == View::Calc && d.mode() == Mode::Matrix && cell(d, 0, 0, 0) == "7");
    // CLR 2 (Memory): the variables too.
    keys(d, {K::Shift, K::D9, K::D2, K::Eq});
    MCHECK(!d.matVec().var(0).defined());
  }
}

std::string resign(std::string body) {
  body.resize(body.size() - 4);
  uint32_t a = 1, b = 0;
  for (unsigned char ch : body) {
    a = (a + ch) % 65521;
    b = (b + a) % 65521;
  }
  const uint32_t sum = (b << 16) | a;
  for (int i = 0; i < 4; ++i) body += static_cast<char>((sum >> (8 * i)) & 0xFF);
  return body;
}

void testSaveRestore() {
  using K = DKey;
  {  // MATRIX: sizes, exact elements, MatAns and the replay's matrix items survive deep sleep.
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D8, K::D1, K::D5});
    type(d, "1=2=3=4=");
    keys(d, {K::AC, K::Shift, K::D4, K::D3, K::Inv});
    type(d, "=");
    keys(d, {K::AC, K::Shift, K::D4, K::D7, K::Shift, K::D4, K::D3});
    type(d, "=");
    const std::string s = d.saveState();
    MCHECK(s.size() <= 2048);
    MCHECK(s[1] == 7);
    Device e;
    e.tick(0);
    MCHECK(e.restoreState(s) && e.mode() == Mode::Matrix && e.view() == View::Calc && !e.matVec().vectors());
    MEQ(cell(e, 0, 1, 1), "4");
    MEQ(cell(e, 3, 1, 0), "3" + FR + "2");
    MCHECK(e.vars().ans.v == -2 && shows(e, 11, 0, "MAT"));
    keys(e, {K::Up});
    MEQ(e.resultText(), M + "2");
    keys(e, {K::Up});
    MCHECK(e.matrixResultShown());
    type(e, "=");
    MCHECK(e.view() == View::MatVec && e.matVec().editing() == 3);
    // Damaged 'X' blocks are refused (re-signed so the checksum passes).
    // From the end: the checksum, 2 form bytes, the count, MatAns (2×2), MatC, MatB (no size), MatA (2×2).
    const size_t tag = s.size() - 4 - 2 - 1 - (1 + 4 * 25) - 1 - 1 - (1 + 4 * 25) - 1;
    MCHECK(tag != std::string::npos && s[tag + 1] == 0x22);  // MatA 2×2
    Device x;
    x.tick(0);
    std::string bad = s;
    bad[tag + 1] = 0x44;  // 4×4
    MCHECK(!x.restoreState(resign(bad)) && x.mode() == Mode::Comp);
    bad = s;
    bad[tag + 1] = 0x02;  // no rows but columns
    MCHECK(!x.restoreState(resign(bad)));
    bad = s;
    bad[tag] = 'V';  // VECTOR's block under MATRIX
    MCHECK(!x.restoreState(resign(bad)));
    bad = s;
    bad[s.size() - 5] = 7;  // a form byte that is neither 0xFF nor 2
    MCHECK(!x.restoreState(resign(bad)));
    bad = s;
    bad[1] = 9;  // a mode byte beyond Vector
    MCHECK(!x.restoreState(resign(bad)));
    MCHECK(x.restoreState(s) && x.mode() == Mode::Matrix);
  }
  {  // VECTOR: a 2D vector in a 3D slot is refused; a sound one restores.
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D9, K::D1, K::D1});
    type(d, "1=2=3=");
    keys(d, {K::AC, K::Shift, K::D5, K::D3});
    type(d, "=");
    const std::string s = d.saveState();
    MCHECK(s[1] == 8);
    Device e;
    e.tick(0);
    MCHECK(e.restoreState(s) && e.mode() == Mode::Vector && e.matVec().vectors() && e.matVec().var(0).cols == 3);
    const size_t tag = s.size() - 4 - 1 - 1 - (1 + 3 * 25) - 1 - 1 - (1 + 3 * 25) - 1;
    MCHECK(s[tag] == 'V');
    MCHECK(tag != std::string::npos && s[tag + 1] == 0x13);
    std::string bad = s;
    bad[tag + 1] = 0x23;  // 2 rows: not a vector
    MCHECK(!e.restoreState(resign(bad)));
    bad[tag + 1] = 0x11;  // 1 dimension: no such vector
    MCHECK(!e.restoreState(resign(bad)));
  }
  {  // Worst case: four 3×3 matrices of inexact values and 20 replay items: within the 2 KB.
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D8});
    for (int v = 0; v < 3; ++v) {
      keys(d, {K::AC, K::Shift, K::D4, K::D1, static_cast<K>(static_cast<int>(K::D1) + v), K::D1});
      for (int i = 0; i < 9; ++i) type(d, "1/7+1.234567=");
    }
    keys(d, {K::AC});
    for (int i = 0; i < 20; ++i) {
      keys(d, {K::Shift, K::D4, K::D3, K::Mul, K::Shift, K::D4, K::D4, K::Add, K::Shift, K::D4, K::D5});
      type(d, "=");
      keys(d, {K::AC, K::AC});
    }
    const std::string s = d.saveState();
    std::printf("matvec: saveState() with 4 full 3x3 matrices and 20 replay items: %zu bytes\n", s.size());
    MCHECK(s.size() <= 2048 && s.size() > 1100);
    Device e;
    e.tick(0);
    MCHECK(e.restoreState(s) && e.matVec().var(3).rows == 3);
    MCHECK(std::fabs(e.matVec().var(2).at(2, 2).v - (1.0 / 7 + 1.234567)) < 1e-12);
  }
}

}  // namespace

int runMatVecTests() {
  testMatrixMaths();
  testMatrixErrors();
  testVectorMaths();
  testMatrixDevice();
  testVectorDevice();
  testExamAndApps();
  testSaveRestore();
  std::printf("matvec: %d checks, %d failed\n", g_checks, g_fail);
  return g_fail;
}
