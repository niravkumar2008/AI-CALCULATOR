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
  // fx-115ES templates and "=" (core/numeric.h): ∫( f , a , b [, tol] ), d/dx( f , a [, tol] ),
  // and "=" for CALC ("Y=X+1") / SOLVE equations (a Syntax ERROR in a plain evaluation).
  Eq, Integ, Deriv,
  // CMPLX mode (MODE 6, core/cmplx.h): i (the ENG key), ∠ (SHIFT (−)), and SHIFT 2's menu:
  // arg( Conjg( ▶r∠θ ▶a+bi. Added last so saved token values keep their numbers; outside
  // CMPLX they never evaluate (Syntax ERROR, and the keys don't type them there).
  ImagI, Angle, Arg, Conjg, ToPolar, ToRect,
  // BASE-N mode (MODE 7, core/basen.h): SHIFT 3's logic operators and functions (and or xnor
  // xor Not( Neg(), its ▼ page's base prefixes (d h b o), and the hex digits A-F (the (−) °'"
  // hyp sin cos tan keys in BASE-N). Added last, as CMPLX's; outside BASE-N: Syntax ERROR.
  And, Or, Xnor, Xor, Not, BNeg, PreD, PreH, PreB, PreO, HexA, HexB, HexC, HexD, HexE, HexF,
  // MATRIX / VECTOR modes (MODE 8 / 9, core/matvec.h): the variables (SHIFT 4 / SHIFT 5's menus),
  // det( Trn( and the dot product "·". Added last, as BASE-N's; outside those modes: Syntax ERROR.
  MatA, MatB, MatC, MatAns, VctA, VctB, VctC, VctAns, Det, Trn, DotP,
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

// Dim: MATRIX / VECTOR only ("Dim ERROR": sizes that don't allow the calculation, or a matrix /
// vector used before its dimension was set). Added last.
enum class CalcError : uint8_t { None, Syntax, Math, Stack, Argument, Dim };

struct EvalResult {
  CalcError error = CalcError::None;
  int errorPos = 0;  // token index to put the cursor at (Goto)
  Num value;
};

// Evaluates a token list. `random` returns a fresh value in [0, 1) for Ran#
// and RanInt# (kept outside so tests stay repeatable).
EvalResult evaluate(const std::vector<Tok>& toks, Vars& vars, AngleUnit unit, double (*random)());

// ---- Limits shared by an evaluation and the evaluations it starts (∫ and d/dx evaluate their
// f again and again; SOLVE its equation). One "run" = the outermost evaluate() or
// numeric::solveExpr() (an EvalRun scope); everything inside it shares these budgets.
//
// Nesting: brackets, functions, ^( and templates open at the same time, counted across an ∫ /
// d/dx and its f (an ∫ costs kTemplateDepth more: its numeric frames, ~1.8 KB, are worth 4
// levels). One more = Stack ERROR, as on the Casio, before the stack can run out: a level
// costs at most 560 bytes on the ESP32 (a function; a bracket 432), so the deepest accepted
// expression needs ~19 KB of the 32 KB loop task (Xtensa -fstack-usage sums, PORT_NOTES.md
// 2026-10-11).
constexpr int kMaxNesting = 24;
constexpr int kTemplateDepth = 4;
// f evaluations in one run (∫, d/dx, SOLVE, and SOLVE over an ∫). Spent: ∫ / d/dx give Math
// ERROR and SOLVE "Can't Solve" (the fx-ES has no "Time Out" message). ~5 s on the ESP32 for a
// 60-step f (~1.2 µs an evaluation on a PC, ×150 for soft doubles); an ordinary ∫ needs 60-500.
constexpr long kMaxFunctionEvals = 25000;
// The platform's "stop" key: polled at a run's first f evaluation and every 64th after it;
// true = stop the run now (same result as a spent budget). nullptr = none (the default).
void setCancelHook(bool (*cancelled)());
class EvalRun {  // RAII: the outermost one starts a fresh budget
 public:
  EvalRun();
  ~EvalRun();
  EvalRun(const EvalRun&) = delete;
  EvalRun& operator=(const EvalRun&) = delete;
};
bool evalRunStep();     // counts one f evaluation; false once the run must stop (budget or cancel)
bool evalRunStopped();  // the current run was stopped

// Formatting, as on the Casio: up to 10 significant digits, scientific
// notation outside the Norm range, fractions shown as "a/b" when asked.
std::string formatDecimal(double v, NormMode norm);
bool fractionText(const Num& n, std::string& out);  // false if not shown as a fraction
std::string formatResult(const Num& n, bool preferFraction, NormMode norm);
const char* errorText(CalcError e);  // "Math ERROR" ...

// ---- The real engine's own operations on values already computed, for the CMPLX evaluator
// (core/cmplx.cpp): it hands every all-real step back here, so a real value in CMPLX gets
// exactly what evaluate() gives it (exact fractions, the angle unit, the Math ERROR cases).
// `op`: a binary operator (Add Sub Mul Div Frac NPr NCr Pow XRoot; l op r = a op b), a postfix
// one (Sq Cube Inv Fact Pct; b unused), Neg, or a function token Sin..RanInt (b = its second
// argument when `two`). False (out unchanged) with err = Math / Syntax on an error.
bool realOp(Tok op, const Num& a, const Num& b, bool two, AngleUnit unit, double (*random)(), Num& out,
            CalcError& err);
// The number starting at toks[i] (digits, ".", ×10ˣ with its (−)), as evaluate() reads it;
// i moves past it. False with err (Syntax: a second "." ...) and errPos on an error.
bool realNumber(const std::vector<Tok>& toks, size_t& i, Num& out, CalcError& err, int& errPos);

}  // namespace calc
