// The scientific calculator: what the keys type (tokens), how an expression is
// evaluated, and how a number is shown. Behaves like a Casio fx-300ES PLUS in
// COMP mode (linear input): same operator priorities, implicit multiplication,
// auto-closed brackets, exact fractions, Math ERROR / Syntax ERROR.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace calc {

// One key press = one token, so DEL removes "sin(" in one go, as on the Casio.
enum class Tok : uint8_t {
  // digits and number parts
  D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, Dot, Exp10,  // Exp10 = ×10ˣ
  // operators
  Add, Sub, Mul, Div, Neg, Frac, Pow, XRoot, NPr, NCr, Comma,
  Open, Close,
  // postfix
  Sq, Cube, Inv, Fact, Pct,
  // functions (each includes its opening bracket)
  Sin, Cos, Tan, Asin, Acos, Atan, Sinh, Cosh, Tanh, Asinh, Acosh, Atanh,
  Log, Ln, Pow10, Exp, Sqrt, Cbrt, Abs, Int, Intg, Rnd, Gcd, Lcm, RanInt,
  // values
  Pi, E, Ans, PreAns, Ran,
  VarA, VarB, VarC, VarD, VarE, VarF, VarX, VarY, VarM,
  Count
};

const char* tokText(Tok t);          // how the token is drawn, e.g. "sin(" or "×"
bool tokOpensBracket(Tok t);         // functions and "(": need a ")" (auto-closed at the end)
std::string exprText(const std::vector<Tok>& toks);  // whole expression as drawn

// Turns typed text (Claude's "expression", e.g. "2*sqrt(3)+sin(30)^2") into
// key tokens, so a scanned problem appears on the display as if keyed in.
// Returns false (and leaves `out` empty) if the text uses anything the
// keypad can't type. At most kMaxTokens tokens, like the Casio's 99-step limit.
constexpr int kMaxTokens = 99;
bool parseExpression(const std::string& text, std::vector<Tok>& out);

// A number that stays an exact fraction as long as it can (1÷3 is 1/3, not
// 0.3333333333), like the Casio's Math mode.
struct Num {
  double v = 0;
  bool exact = true;  // num/den hold the exact value
  long long num = 0, den = 1;
  static Num ofInt(long long n);
  static Num ofDouble(double d);
};

enum class AngleUnit : uint8_t { Deg, Rad, Gra };
enum class NormMode : uint8_t { Norm1, Norm2 };

struct Vars {
  Num a, b, c, d, e, f, x, y, m;
  Num ans, preAns;
  Num& get(Tok t);
};

enum class CalcError : uint8_t { None, Syntax, Math, Stack, Argument };

struct EvalResult {
  CalcError error = CalcError::None;
  int errorPos = 0;  // token index to put the cursor at (Goto)
  Num value;
};

// Evaluates a token list. `random` returns a fresh value in [0, 1) for Ran#
// and RanInt# (kept outside so tests stay repeatable).
EvalResult evaluate(const std::vector<Tok>& toks, Vars& vars, AngleUnit unit, double (*random)());

// Formatting, as on the Casio: up to 10 significant digits, scientific
// notation outside the Norm range, fractions shown as "a/b" when asked.
std::string formatDecimal(double v, NormMode norm);
bool fractionText(const Num& n, std::string& out);  // false if not shown as a fraction
std::string formatResult(const Num& n, bool preferFraction, NormMode norm);
const char* errorText(CalcError e);  // "Math ERROR" ...

}  // namespace calc
