// Unit tests for BASE-N (core/basen.*, MODE 7): the integer evaluator (conversions, two's
// complement, the logic operators' truth tables, the prefixes, priorities, Math / Syntax / Stack
// ERRORs), the zero-padded display, and the Device integration (MODE menu, the DEC HEX BIN OCT
// keys, A-F, SHIFT 3's menu, Ans and memories, the replay, exam mode, leaving BASE-N,
// saveState()). runBaseNTests() returns the number of failures; tests.cpp counts them.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include "../core/basen.h"
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
    std::printf("FAIL basen_tests.cpp:%d: %s\n", line, what);
  }
}
#define BCHECK(cond) check((cond), #cond, __LINE__)
#define BEQ(got, want)                                                                                    \
  do {                                                                                                    \
    const std::string g_ = (got), w_ = (want);                                                            \
    if (g_ != w_) std::printf("  basen_tests.cpp:%d: got \"%s\", want \"%s\"\n", __LINE__, g_.c_str(), w_.c_str()); \
    check(g_ == w_, #got " == " #want, __LINE__);                                                         \
  } while (0)

const std::string M = "\xE2\x88\x92";  // −

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
  BCHECK(same);
}

// Test-only text → tokens: 0-9, A-F the hex digits, + - * / ( ), the words and or xor xnor
// Not( Neg( Ans, the prefixes d h b o, and $A ... $M the memories.
std::vector<Tok> tk(const std::string& s) {
  struct W {
    const char* w;
    Tok t;
  };
  static const W words[] = {{"xnor", Tok::Xnor}, {"xor", Tok::Xor}, {"and", Tok::And}, {"or", Tok::Or},
                            {"Not(", Tok::Not},  {"Neg(", Tok::BNeg}, {"Ans", Tok::Ans}, {"d", Tok::PreD},
                            {"h", Tok::PreH},    {"b", Tok::PreB},   {"o", Tok::PreO}, {"+", Tok::Add},
                            {"-", Tok::Sub},     {"*", Tok::Mul},    {"/", Tok::Div},  {"(", Tok::Open},
                            {")", Tok::Close}};
  std::vector<Tok> out;
  for (size_t i = 0; i < s.size();) {
    const char c = s[i];
    if (c >= '0' && c <= '9') {
      out.push_back(static_cast<Tok>(c - '0'));
      ++i;
      continue;
    }
    if (c >= 'A' && c <= 'F' && s.compare(i, 3, "Ans") != 0) {
      out.push_back(static_cast<Tok>(static_cast<int>(Tok::HexA) + (c - 'A')));
      ++i;
      continue;
    }
    if (c == '$' && i + 1 < s.size()) {
      static const char vars[] = "ABCDEFXYM";
      for (int k = 0; vars[k]; ++k)
        if (vars[k] == s[i + 1]) out.push_back(static_cast<Tok>(static_cast<int>(Tok::VarA) + k));
      i += 2;
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
      std::printf("basen tk: can't read '%s' at %zu\n", s.c_str(), i);
      ++g_fail;
      return out;
    }
  }
  return out;
}

BaseResult ev(const std::string& s, Base b = Base::Dec, const Vars* vars = nullptr) {
  Vars v;
  return evaluateBaseN(tk(s), vars ? *vars : v, b);
}
// The result as shown in base `b`, or the error message.
std::string show(const std::string& s, Base b = Base::Dec, const Vars* vars = nullptr) {
  const BaseResult r = ev(s, b, vars);
  if (r.error != CalcError::None) return errorText(r.error);
  return baseText(r.value, b);
}
int32_t val(const std::string& s, Base b = Base::Dec) { return ev(s, b).value; }

// ---------------------------------------------------------------- the evaluator
void testConversions() {
  // The fx-ES guide's example: 15 × 37 = 555 in each base, zero-padded as the Casio shows it.
  BEQ(show("15*37"), "555");
  BEQ(baseText(555, Base::Hex), "0000022B");
  BEQ(baseText(555, Base::Bin), "0000001000101011");
  BEQ(baseText(555, Base::Oct), "00000001053");
  // Its other examples: 11₂ + 1₂, 1F₁₆ + 1₁₆, 7₈ + 1₈, d10 + h10 + b10 + o10.
  BEQ(show("11+1", Base::Bin), "0000000000000100");
  BEQ(show("1F+1", Base::Hex), "00000020");
  BEQ(show("7+1", Base::Oct), "00000000010");
  BEQ(show("d10+h10+b10+o10"), "36");
  BEQ(show("d10+h10+b10+o10", Base::Hex), "00000024");
  // Widths: HEX 8, BIN 16, OCT 11 digits; DEC as COMP shows integers.
  BEQ(baseText(0, Base::Hex), "00000000");
  BEQ(baseText(0, Base::Bin), "0000000000000000");
  BEQ(baseText(0, Base::Oct), "00000000000");
  BEQ(baseText(0, Base::Dec), "0");
  BEQ(baseText(2147483647, Base::Dec), "2147483647");
  BEQ(baseText(-2147483647 - 1, Base::Dec), M + "2147483648");
  BEQ(baseText(2147483647, Base::Hex), "7FFFFFFF");
  BEQ(baseText(2147483647, Base::Oct), "17777777777");
  BEQ(baseText(32767, Base::Bin), "0111111111111111");
  // Every digit and letter reads in its base.
  BCHECK(val("ABCDEF", Base::Hex) == 0xABCDEF);
  BCHECK(val("1234567", Base::Oct) == 01234567);
  BCHECK(val("1011", Base::Bin) == 11);
  BCHECK(val("hFF") == 255 && val("o777") == 511 && val("b1111") == 15 && val("d99", Base::Bin) == 99);
  BCHECK(val("hFF", Base::Bin) == 255);
}

void testTwosComplement() {
  // Negatives: two's complement in HEX / OCT (32 bits) and BIN (16 bits); a minus sign in DEC.
  BEQ(show("0-1", Base::Hex), "FFFFFFFF");
  BEQ(show("0-1", Base::Oct), "37777777777");
  BEQ(show("0-1", Base::Bin), "1111111111111111");
  BEQ(show("0-1"), M + "1");
  BCHECK(val("FFFFFFFF", Base::Hex) == -1);
  BCHECK(val("80000000", Base::Hex) == -2147483647 - 1);
  BCHECK(val("7FFFFFFF", Base::Hex) == 2147483647);
  BCHECK(val("37777777777", Base::Oct) == -1);
  BCHECK(val("20000000000", Base::Oct) == -2147483647 - 1);
  BCHECK(val("1111111111111111", Base::Bin) == -1);
  BCHECK(val("1000000000000000", Base::Bin) == -32768);
  BCHECK(val("hFFFFFFFF") == -1 && val("b1111111111111111") == -1 && val("o37777777777") == -1);
  // Arithmetic wraps nowhere: FFFFFFFF + 1 is −1 + 1.
  BEQ(show("FFFFFFFF+1", Base::Hex), "00000000");
  BEQ(show("FFFFFFFF*FFFFFFFF", Base::Hex), "00000001");
  // − / + where a value belongs: its sign ((−) types A in BASE-N).
  BEQ(show("-5"), M + "5");
  BEQ(show("--5"), "5");
  BEQ(show("+5"), "5");
  BEQ(show("2*-3"), M + "6");
}

void testLogic() {
  // The fx-ES guide's examples (BIN).
  BEQ(show("1010and1100", Base::Bin), "0000000000001000");
  BEQ(show("1011or11010", Base::Bin), "0000000000011011");
  BEQ(show("1010xor1100", Base::Bin), "0000000000000110");
  BEQ(show("1111xnor101", Base::Bin), "1111111111110101");
  BEQ(show("Not(1010)", Base::Bin), "1111111111110101");
  BEQ(show("Neg(101101)", Base::Bin), "1111111111010011");
  // Truth tables, bit by bit (the low bit; xnor and Not set every other bit).
  for (int a = 0; a <= 1; ++a)
    for (int b = 0; b <= 1; ++b) {
      const std::string A = std::to_string(a), B = std::to_string(b);
      BCHECK(val(A + "and" + B, Base::Bin) == (a & b));
      BCHECK(val(A + "or" + B, Base::Bin) == (a | b));
      BCHECK(val(A + "xor" + B, Base::Bin) == (a ^ b));
      BCHECK(val(A + "xnor" + B, Base::Bin) == ~(a ^ b));
      BCHECK((val(A + "xnor" + B, Base::Bin) & 1) == ((a == b) ? 1 : 0));
    }
  BCHECK(val("Not(0)") == -1 && val("Not(" + std::string("0-1") + ")") == 0);
  BCHECK(val("Not(5)") == -6 && val("Neg(5)") == -5 && val("Neg(Neg(5))") == 5);
  BEQ(show("Not(0)", Base::Hex), "FFFFFFFF");
  BEQ(show("FFFF0000andFF00FF00", Base::Hex), "FF000000");
  BEQ(show("F0orF", Base::Hex), "000000FF");
  // Priorities: and binds tighter than or / xor / xnor, + − tighter than and.
  BCHECK(val("1or2and3") == 3);   // 1 or (2 and 3)
  BCHECK(val("6xor3and1") == 7);  // 6 xor (3 and 1)
  BCHECK(val("2+3and1") == 1);    // (2+3) and 1
  BCHECK(val("12and10or1") == 9); // (12 and 10) or 1
  BCHECK(val("5xor3xnor1") == ~(6 ^ 1));  // left to right
  // Auto-closed bracket, implicit ×, Ans continuing.
  BCHECK(val("Not(5") == -6 && val("2(3)") == 6 && val("2(3)(4") == 24 && val("(2)3") == 6);
}

void testErrors() {
  const std::string MATH = "Math ERROR", SYN = "Syntax ERROR";
  // Out of the 32-bit range (a number, a step, the result) or BIN's 16 bits.
  BEQ(show("2147483647+1"), MATH);
  BEQ(show("2147483648"), MATH);
  BEQ(show("0-2147483647-1"), M + "2147483648");
  BEQ(show("0-2147483647-2"), MATH);
  BEQ(show("65536*65536"), MATH);
  BEQ(show("100000000", Base::Hex), MATH);
  BEQ(show("7FFFFFFF+1", Base::Hex), MATH);
  BEQ(show("40000000000", Base::Oct), MATH);
  BEQ(show("10000000000000000", Base::Bin), MATH);  // 17 bits
  BEQ(show("111111111111111+1", Base::Bin), MATH);  // 32767 + 1
  BEQ(show("d40000", Base::Bin), MATH);             // the result doesn't fit BIN
  BEQ(show("d40000-d39999", Base::Bin), "0000000000000001");  // a step may (32 bits)
  BEQ(show("Neg(h80000000)"), MATH);
  BEQ(show("5/0"), MATH);
  BEQ(show("h80000000/FFFFFFFF", Base::Hex), MATH);  // −2147483648 ÷ −1
  // Division cuts the fraction off (toward 0).
  BCHECK(val("7/2") == 3 && val("-7/2") == -3 && val("0-7/2") == -3 && val("1/3") == 0);
  // A digit the base doesn't have, a prefix without a number, a missing operand: Syntax.
  BEQ(show("2", Base::Bin), SYN);
  BEQ(show("8", Base::Oct), SYN);
  BEQ(show("A"), SYN);
  BEQ(show("b12"), SYN);
  BEQ(show("h"), SYN);
  BEQ(show("h+1"), SYN);
  BEQ(show("1+"), SYN);
  BEQ(show("and1"), SYN);
  BEQ(show("1)"), SYN);
  BEQ(show(""), SYN);
  {  // the COMP tokens are not BASE-N's
    Vars v;
    for (Tok t : {Tok::Dot, Tok::Exp10, Tok::Sin, Tok::Pi, Tok::E, Tok::ImagI, Tok::Sq, Tok::Pow, Tok::Ran})
      BCHECK(evaluateBaseN({Tok::D1, t, Tok::D2}, v, Base::Dec).error == CalcError::Syntax);
  }
  // The error position for Goto: the bad digit.
  {
    const BaseResult r = ev("11+12", Base::Bin);
    BCHECK(r.error == CalcError::Syntax && r.errorPos == 4);
  }
  // The shared nesting budget: kMaxNesting brackets are fine, one more is a Stack ERROR.
  {
    std::vector<Tok> t(kMaxNesting, Tok::Open);
    t.push_back(Tok::D1);
    Vars v;
    BCHECK(evaluateBaseN(t, v, Base::Dec).error == CalcError::None);
    t.insert(t.begin(), Tok::Not);
    BCHECK(evaluateBaseN(t, v, Base::Dec).error == CalcError::Stack);
  }
  // COMP refuses the BASE-N tokens (and its replay never meets them).
  {
    Vars v;
    for (Tok t : {Tok::And, Tok::HexA, Tok::PreH, Tok::Not})
      BCHECK(evaluate({Tok::D1, t, Tok::D1}, v, AngleUnit::Deg, nullptr).error != CalcError::None);
  }
}

void testMemories() {
  // A memory or Ans from COMP: its integer part (toward 0); outside 32 bits: Math ERROR.
  Vars v;
  v.a = Num::ofDouble(2.9);
  v.b = Num::ofDouble(-2.9);
  v.c = Num::ofDouble(1e12);
  v.ans = Num::ofInt(40);
  v.m = Num::ofInt(65535);
  BCHECK(ev("$A", Base::Dec, &v).value == 2 && ev("$B", Base::Dec, &v).value == -2);
  BCHECK(ev("$C", Base::Dec, &v).error == CalcError::Math);
  BCHECK(ev("Ans+2", Base::Dec, &v).value == 42 && ev("2$A", Base::Dec, &v).value == 4);
  BCHECK(ev("$M", Base::Bin, &v).error == CalcError::Math);  // 65535 needs 17 bits signed
  BEQ(show("$M", Base::Hex, &v), "0000FFFF");
  // baseFits / names.
  BCHECK(baseFits(32767, Base::Bin) && !baseFits(32768, Base::Bin) && baseFits(-32768, Base::Bin) &&
         !baseFits(-32769, Base::Bin));
  BCHECK(baseFits(2147483647LL, Base::Hex) && !baseFits(2147483648LL, Base::Dec) && !baseFits(-2147483649LL, Base::Oct));
  BEQ(baseName(Base::Dec), "Dec");
  BEQ(baseName(Base::Hex), "Hex");
  BEQ(baseName(Base::Bin), "Bin");
  BEQ(baseName(Base::Oct), "Oct");
  // Every glyph the mode draws is in the font.
  for (const std::string s : {"Dec Hex Bin Oct 0123456789ABCDEF", "and or xnor xor Not( Neg( d h b o", "7:BASE-N"})
    for (uint32_t cp : decodeUtf8(s)) BCHECK(hasGlyph(cp));
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

void testDevice() {
  using K = DKey;
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode});
    snapshot("basen_menu_mode", d);
    BCHECK(shows(d, 0, 4, "7:BASE-N"));
    keys(d, {K::D7});
    BCHECK(d.view() == View::Calc && d.mode() == Mode::BaseN && d.numberBase() == Base::Dec);
    BCHECK(shows(d, 11, 0, "Dec"));
    type(d, "15*37=");
    BEQ(d.resultText(), "555");
    BCHECK(d.vars().ans.v == 555);
    // x² DEC, x^□ HEX, log BIN, ln OCT: the shown result converted, the status bar follows.
    keys(d, {K::Pow});
    BCHECK(d.numberBase() == Base::Hex && shows(d, 11, 0, "Hex"));
    BEQ(d.resultText(), "0000022B");
    snapshot("basen_result_hex", d);
    keys(d, {K::Log});
    BEQ(d.resultText(), "0000001000101011");
    BCHECK(shows(d, 11, 0, "Bin"));
    snapshot("basen_result_bin", d);
    keys(d, {K::Ln});
    BEQ(d.resultText(), "00000001053");
    BCHECK(shows(d, 11, 0, "Oct"));
    keys(d, {K::Sq});
    BEQ(d.resultText(), "555");
    // A-F on (−) °'" hyp sin cos tan (HEX).
    keys(d, {K::Pow, K::Neg, K::Dms, K::Hyp, K::Sin, K::Cos, K::Tan});
    BCHECK(d.expression().size() == 6 && d.expression()[0] == Tok::HexA && d.expression()[5] == Tok::HexF);
    snapshot("basen_typing_hex", d);
    type(d, "=");
    BEQ(d.resultText(), "00ABCDEF");
    BCHECK(d.vars().ans.v == 0xABCDEF);
    // SHIFT 3: the BASE menu, ▼ the prefixes (▲▼ in the status bar).
    keys(d, {K::AC, K::D1, K::D2, K::Shift, K::D3});
    BCHECK(d.view() == View::BaseMenu && d.baseMenuPage() == 0);
    BCHECK(shows(d, 24, 0, "\xE2\x96\xBC") && !shows(d, 23, 0, "\xE2\x96\xB2"));
    snapshot("basen_base_menu", d);
    keys(d, {K::Down});
    BCHECK(d.baseMenuPage() == 1 && shows(d, 23, 0, "\xE2\x96\xB2"));
    snapshot("basen_base_menu2", d);
    keys(d, {K::Up});
    BCHECK(d.baseMenuPage() == 0);
    keys(d, {K::D1});  // and
    BCHECK(d.view() == View::Calc && d.expression().back() == Tok::And);
    keys(d, {K::D1, K::D5});  // 12 and 15 (HEX)
    keys(d, {K::Shift, K::D3, K::Down, K::D1});  // d
    BCHECK(d.expression().back() == Tok::PreD);
    keys(d, {K::AC, K::Shift, K::D3, K::AC});
    BCHECK(d.view() == View::Calc && d.expression().empty());
    // HEX: 12 and 15 = 10 (h12 & h15 = h10).
    type(d, "12");
    keys(d, {K::Shift, K::D3, K::D1});
    type(d, "15=");
    BEQ(d.resultText(), "00000010");
    // After a result, a logic operator carries on from Ans; Not( starts afresh.
    keys(d, {K::Shift, K::D3, K::D2});
    BCHECK(d.expression().size() == 2 && d.expression()[0] == Tok::Ans && d.expression()[1] == Tok::Or);
    type(d, "1=");
    BEQ(d.resultText(), "00000011");
    keys(d, {K::Shift, K::D3, K::D5});
    BCHECK(d.expression().size() == 1 && d.expression()[0] == Tok::Not);
    type(d, "0=");
    BEQ(d.resultText(), "FFFFFFFF");
    // The replay.
    keys(d, {K::Up});
    BEQ(d.resultText(), "00000011");
    keys(d, {K::Down});
    BEQ(d.resultText(), "FFFFFFFF");
    // DEC 100000 can't show in BIN: Math ERROR (as the Casio, out of BIN's range).
    keys(d, {K::Sq});
    type(d, "100000=");
    keys(d, {K::Log});
    BCHECK(d.view() == View::Error && d.errorMessage() == "Math ERROR" && d.numberBase() == Base::Bin);
    keys(d, {K::AC});
    // And the replay of it in BIN, likewise.
    keys(d, {K::Up});
    BCHECK(d.view() == View::Error);
    keys(d, {K::AC});
    // A digit BIN doesn't have: Syntax ERROR; Goto puts the cursor on it.
    type(d, "12=");
    BCHECK(d.view() == View::Error && d.errorMessage() == "Syntax ERROR");
    keys(d, {K::Right});
    BCHECK(d.view() == View::Calc && d.cursor() == 1);
    // Keys BASE-N has no use for type nothing (√ . ×10ˣ x⁻¹ SHIFT sin π, ∫dx, CALC, SOLVE, ENG, S⇔D).
    keys(d, {K::AC, K::Sqrt, K::Dot, K::Exp10, K::Inv, K::Frac, K::Shift, K::Sin, K::Shift, K::Exp10, K::Integral,
             K::Calc, K::Shift, K::Calc, K::Eng, K::SD, K::Alpha, K::Exp10, K::Shift, K::D0, K::LogAB});
    BCHECK(d.view() == View::Calc && d.expression().empty());
    // ALPHA (−) is still the memory A.
    keys(d, {K::Alpha, K::Neg});
    BCHECK(d.expression().size() == 1 && d.expression()[0] == Tok::VarA);
    // STO / RCL / M+ with integers.
    keys(d, {K::AC, K::Sq});
    type(d, "7");
    keys(d, {K::Shift, K::Rcl, K::Dms});  // STO B
    BCHECK(d.vars().b.v == 7);
    BEQ(d.resultText(), "7");
    keys(d, {K::Log, K::Rcl, K::Dms});  // RCL B in BIN
    BEQ(d.resultText(), "0000000000000111");
    keys(d, {K::Sq, K::AC});
    type(d, "5");
    keys(d, {K::MPlus});
    type(d, "3");
    keys(d, {K::Shift, K::MPlus});  // M−
    BCHECK(d.vars().m.v == 2 && shows(d, 3, 0, "M"));
    // Leaving BASE-N: the replay is cleared, Ans stays, the keys are COMP's again.
    type(d, "6=");
    keys(d, {K::Mode, K::D1});
    BCHECK(d.mode() == Mode::Comp && d.vars().ans.v == 6);
    BCHECK(!shows(d, 11, 0, "Dec") && shows(d, 11, 0, "   "));
    keys(d, {K::Up});
    BCHECK(!d.showingResult());
    type(d, "q");  // x² is x² again
    BCHECK(d.expression().empty() || d.expression()[0] == Tok::Sq);
    keys(d, {K::AC});
    type(d, "1.5*2=");
    BEQ(d.resultText(), "3");
    // A COMP fraction in BASE-N: cut to its integer part; entering BASE-N is DEC again.
    type(d, "7/2=");
    keys(d, {K::Mode, K::D7});
    BCHECK(d.numberBase() == Base::Dec);
    type(d, "a=");
    BEQ(d.resultText(), "3");
  }
  {  // exam mode allows BASE-N (a calculator mode); the apps stay off
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::Mode, K::D8, K::Eq});
    BCHECK(d.examActive());
    keys(d, {K::AC, K::Mode});
    snapshot("basen_menu_exam", d);
    keys(d, {K::D7});
    BCHECK(d.mode() == Mode::BaseN && d.view() == View::Calc);
    keys(d, {K::Pow, K::Tan, K::Tan});  // HEX, F F
    type(d, "=");
    BEQ(d.resultText(), "000000FF");
    snapshot("basen_exam", d);
    keys(d, {K::Mode, K::D4});
    BCHECK(d.view() == View::Notice);
    keys(d, {K::AC});
    BCHECK(d.view() == View::Calc && d.mode() == Mode::BaseN);
  }
  {  // MODE 4 (Apps) and back: BASE-N, its base and Ans stay under the apps.
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D7, K::Ln});
    type(d, "17=");
    keys(d, {K::Mode, K::D4});
    BCHECK(d.view() == View::Apps);
    keys(d, {K::AC});
    BCHECK(d.view() == View::Calc && d.mode() == Mode::BaseN && d.numberBase() == Base::Oct && d.vars().ans.v == 15);
    keys(d, {K::Mode, K::D6});  // CMPLX: leaving BASE-N
    BCHECK(d.mode() == Mode::Cmplx && d.numberBase() == Base::Dec);
  }
  {  // saveState(): the mode, the base, integer memories and the replay survive.
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D7, K::Pow});
    type(d, "10=");
    keys(d, {K::Neg});  // A
    type(d, "+1=");
    const std::string s = d.saveState();
    BCHECK(s.size() <= 2048);
    Device e;
    e.tick(0);
    BCHECK(e.restoreState(s) && e.mode() == Mode::BaseN && e.view() == View::Calc && e.numberBase() == Base::Hex);
    BCHECK(e.vars().ans.v == 11 && e.vars().preAns.v == 16);
    BCHECK(shows(e, 11, 0, "Hex"));
    keys(e, {K::Up});
    BEQ(e.resultText(), "0000000B");
    keys(e, {K::Up});
    BEQ(e.resultText(), "00000010");
    // Damaged 'N' blocks are refused (re-signed so the checksum passes).
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
    const size_t tag = s.size() - 4 - 2;  // 'N', the base, the checksum
    BCHECK(s[tag] == 'N' && s[tag + 1] == 1 && s[1] == 6);
    Device x;
    x.tick(0);
    std::string bad = s;
    bad[tag + 1] = 4;  // no such base
    BCHECK(!x.restoreState(resign(bad)) && x.mode() == Mode::Comp);
    bad = s;
    bad[tag] = 'C';  // another mode's block under mode BASE-N
    BCHECK(!x.restoreState(resign(bad)));
    bad = s;
    bad[1] = 7;  // MATRIX with BASE-N's block
    BCHECK(!x.restoreState(resign(bad)));
    bad = s;
    bad[1] = 5;  // CMPLX with BASE-N's block
    BCHECK(!x.restoreState(resign(bad)));
    // Mode byte BASE-N without its block (as an older layout would end): DEC.
    bad = s;
    bad.erase(tag, 2);
    BCHECK(x.restoreState(resign(bad)) && x.mode() == Mode::BaseN && x.numberBase() == Base::Dec);
    BCHECK(x.restoreState(resign(s)) && x.numberBase() == Base::Hex);
    // A replay value no BASE-N calculation makes (a fraction) is refused under BASE-N.
    Device c;
    c.tick(0);
    type(c, "1/2=");
    std::string cs = c.saveState();
    BCHECK(cs[1] == 0);
    cs[1] = 6;
    BCHECK(!x.restoreState(resign(cs)));
    // COMP's state still restores as COMP (the base is DEC there).
    Device c2;
    c2.tick(0);
    BCHECK(c2.restoreState(c.saveState()) && c2.mode() == Mode::Comp && c2.numberBase() == Base::Dec);
  }
}

}  // namespace

int runBaseNTests() {
  testConversions();
  testTwosComplement();
  testLogic();
  testErrors();
  testMemories();
  testDevice();
  std::printf("basen: %d checks, %d failed\n", g_checks, g_fail);
  return g_fail;
}
