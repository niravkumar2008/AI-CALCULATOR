// Regression tests for calc_engine fixes found by the v15 review:
//   F-3  a postfix function after a closed ^( ) or ⁿ√( ) was a Syntax ERROR
//   F-11 0^0 gave 1 (the Casio says Math ERROR)
//   F-14 1E2E3 gave 100000 (a second ×10ˣ in one number is a Syntax ERROR)
// Hooked into tests.cpp by calling runEngineRegressTests(); returns the number of failures.
#include <cstdio>
#include <string>
#include <vector>

#include "../core/calc_engine.h"

using namespace calc;

namespace {

int g_regFail = 0, g_regPass = 0;

#define REG_CHECK(cond)                                                              \
  do {                                                                               \
    if (cond) ++g_regPass;                                                           \
    else { ++g_regFail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
  } while (0)

double regRandom() { return 0.5; }

std::string show(const EvalResult& r, bool frac) {
  if (r.error != CalcError::None) return errorText(r.error);
  return formatResult(r.value, frac, NormMode::Norm1);
}

// As keyed on the calculator, token by token.
std::string keyed(const std::vector<Tok>& t, bool frac = false) {
  Vars v;
  return show(evaluate(t, v, AngleUnit::Deg, regRandom), frac);
}

// As typed (Claude's expression text).
std::string typed(const std::string& text, bool frac = false) {
  std::vector<Tok> t;
  if (!parseExpression(text, t)) return "PARSE";
  Vars v;
  return show(evaluate(t, v, AngleUnit::Deg, regRandom), frac);
}

const std::string kMinus = "\xE2\x88\x92";  // −

}  // namespace

int runEngineRegressTests() {
  g_regFail = g_regPass = 0;

  // F-3: ^( ) and ⁿ√( ) then a postfix function, left to right.
  REG_CHECK(keyed({Tok::D2, Tok::Pow, Tok::D3, Tok::Close, Tok::Sq}) == "64");           // 2^(3)² = 8²
  REG_CHECK(keyed({Tok::D2, Tok::Pow, Tok::D3, Tok::Close, Tok::Fact}) == "40320");      // 2^(3)! = 8!
  REG_CHECK(keyed({Tok::D2, Tok::Pow, Tok::D2, Tok::Close, Tok::Inv}) == "0.25");        // 2^(2)⁻¹ = 4⁻¹
  REG_CHECK(keyed({Tok::D1, Tok::D0, Tok::Pow, Tok::D2, Tok::Close, Tok::Pct}) == "1");  // 10^(2)% = 100%
  REG_CHECK(keyed({Tok::D2, Tok::Pow, Tok::D2, Tok::Close, Tok::Cube, Tok::Sq}) == "4096");  // (4³)²
  REG_CHECK(keyed({Tok::D3, Tok::XRoot, Tok::D8, Tok::Close, Tok::Sq}) == "4");          // ³√(8)² = 2²
  REG_CHECK(typed("2^(3)\xC2\xB2") == "64");
  REG_CHECK(typed("2^(3)!") == "40320");
  REG_CHECK(typed("2^(3)\xC2\xB2+1") == "65");     // the postfix doesn't swallow what follows
  REG_CHECK(typed("2^(3)\xC2\xB2" "2") == "128");  // implicit multiplication still applies after it
  // ... and the existing priorities are unchanged.
  REG_CHECK(typed("2^3^2") == "64");               // left to right
  REG_CHECK(typed("2\xC2\xB2^(3)") == "64");       // 2²^(3) = 4³
  REG_CHECK(typed("-2\xC2\xB2") == kMinus + "4");  // −2² = −(2²)
  REG_CHECK(typed("(-2)\xC2\xB2") == "4");
  REG_CHECK(typed("2^(-1)") == "0.5");
  REG_CHECK(typed("3^(2)2") == "18");              // 9 × 2
  REG_CHECK(typed("2(1+2)") == "6");
  REG_CHECK(typed("1/2pi") == typed("1/(2pi)"));   // implicit × binds tighter than ÷
  REG_CHECK(keyed({Tok::D2, Tok::Pow, Tok::D3}) == "8");  // auto-closed at the end

  // F-11: 0^0 is a Math ERROR; other powers of 0 and to the 0 are fine.
  REG_CHECK(typed("0^0") == "Math ERROR");
  REG_CHECK(typed("0^(0)") == "Math ERROR");
  REG_CHECK(typed("0.0^(1-1)") == "Math ERROR");
  REG_CHECK(typed("0^2") == "0");
  REG_CHECK(typed("5^0") == "1");
  REG_CHECK(typed("0^(-1)") == "Math ERROR");

  // F-14: one ×10ˣ per number, with a whole-number exponent.
  REG_CHECK(keyed({Tok::D1, Tok::Exp10, Tok::D2, Tok::Exp10, Tok::D3}) == "Syntax ERROR");
  REG_CHECK(typed("1E2E3") == "Syntax ERROR");
  REG_CHECK(keyed({Tok::D1, Tok::Exp10, Tok::D2, Tok::Dot, Tok::D5}) == "Syntax ERROR");
  REG_CHECK(keyed({Tok::Exp10, Tok::D2, Tok::Exp10, Tok::D3}) == "Syntax ERROR");
  REG_CHECK(typed("1E2") == "100");
  REG_CHECK(typed("1.5E-1") == "0.15");
  REG_CHECK(keyed({Tok::Exp10, Tok::D3}) == "1000");  // "E3" alone is 1E3
  REG_CHECK(typed("2E3+1E2") == "2100");
  REG_CHECK(typed("2E3pi") == typed("2000pi"));        // a value after the exponent still multiplies

  std::printf("engine regression: %d passed, %d failed\n", g_regPass, g_regFail);
  return g_regFail;
}
