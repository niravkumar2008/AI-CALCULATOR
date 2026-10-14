// Casio fx-115ES style numeric calculus and equation tools: CALC, SOLVE,
// ∫dx (definite integral) and d/dx (derivative at a point).
//
// How they appear in the token stream (calc_engine.h):
//   Tok::Integ "∫("     ∫( f , a , b [, tol] )   integral of f over X from a to b
//   Tok::Deriv "d/dx("  d/dx( f , a [, tol] )    derivative of f with respect to X at X = a
//   Tok::Eq    "="      only for CALC ("Y=X+1": assigns) and SOLVE ("X²=2"). A plain = evaluation
//                       of an expression containing "=" is a Syntax ERROR, as on the Casio.
// f always uses X as its variable, like the Casio. The closing ")" is optional at the very end,
// as for every other function. ∫ and d/dx may not be nested inside another one's f (Syntax
// ERROR); anywhere else is fine (2∫(X,0,1)+1, in a SOLVE equation, in a CALC expression).
// calc::evaluate() handles both templates by calling evalTemplate() below, so Device::equals()
// needs nothing new for them.
//
// CalcRunner / SolveRunner are the CALC and SHIFT CALC (SOLVE) screens: small state machines the
// Device feeds keys into while one is active (see "Integration" at the end of this file).
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "calc_engine.h"
#include "framebuffer.h"
#include "keys.h"

namespace calc {
namespace numeric {

// ------------------------------------------------------------------ pure numerics

// f(x): false when f is undefined at x (Math ERROR there).
using RealFn = std::function<bool(double x, double& y)>;
// Both sides of an equation: SOLVE drives lhs - rhs to 0 and reports L−R.
using EquationFn = std::function<bool(double x, double& lhs, double& rhs)>;

enum class Status : uint8_t {
  Ok,
  Domain,      // f was undefined where it had to be evaluated (Math ERROR)
  NoConverge,  // ∫: did not reach the tolerance (newer Casios say "Time Out"; Math ERROR here).
               // A run out of its kMaxFunctionEvals budget (calc_engine.h) or cancelled with
               // AC: Math ERROR for ∫ / d/dx, "Can't Solve" for SOLVE.
               // d/dx: no stable limit / not differentiable there (Math ERROR)
               // SOLVE: "Can't Solve"
};

constexpr double kIntegralTol = 1e-5;  // fx-ES default tol for ∫ (relative to the result)
constexpr double kDerivTol = 1e-10;    // fx-ES default tol for d/dx
constexpr int kMaxIntervals = 400;     // ∫: adaptive subintervals (15 points each) before giving up
constexpr int kSolveIterations = 150;  // SOLVE: iterations before "Can't Solve"

// ∫ from a to b (a > b gives the negated integral, a == b gives 0). Global adaptive
// Gauss–Kronrod (7-point Gauss / 15-point Kronrod, QUADPACK error estimate), always splitting
// the worst subinterval. Never evaluates f at a or b, so integrable endpoint singularities
// (1/√X on 0..1) work. Starts from 4 equal pieces (plus pieces graded by powers of ten from 0
// over a range wider than 1000), never from one piece's estimate. Where f is undefined at a
// node inside, the piece is split at that point (at most 8 such points: ln(Abs X) over −1..1).
// Done when the estimated error <= tol·|∫| (with a round-off floor). tol <= 0 = kIntegralTol.
Status integrate(const RealFn& f, double a, double b, double tol, double& out, double* errEstimate = nullptr);

// f'(x0): Ridders' central differences with Richardson extrapolation, from a first step of
// 1E-3·max(1, |x0|), smaller (÷8, up to 10 times) while the error estimate is above tol,
// keeping the best. Domain when f(x0) is undefined. NoConverge when the extrapolation does not
// settle (∛X at 0) or the one-sided slopes disagree however small the step (a corner:
// d/dx(Abs(X),0)). The real fx-ES returns the bare central difference at a corner (0 for Abs
// at 0); a Math ERROR here is a deliberate, safer difference. tol <= 0 = kDerivTol; the result
// is accepted when its error estimate is below max(tol, 1E-6)·max(1, |f'|), and is 0 when no
// bigger than twice that estimate.
Status derivative(const RealFn& f, double x0, double tol, double& out);

// Root of lhs(x) = rhs(x) from the guess x0: Newton's method (central-difference slope) with a
// secant fallback, step limits, backtracking (a damped step must shrink |L−R|), and bisection
// once a sign change is bracketed. Steps and convergence are relative to |x| (floor 1E-12 of the
// guess's size), so a root at 1E-15 is found. Domain when the equation is undefined at the
// guess itself. On Ok: x = the root, lr = L−R there.
Status solve(const EquationFn& eq, double x0, double& x, double& lr, int* iterations = nullptr);

// ------------------------------------------------------------------ token helpers

// Evaluates `f` with variable `var` (Tok::VarA..VarM) set to x; other variables from `vars`.
EvalResult evalWith(const std::vector<Tok>& f, Tok var, double x, const Vars& vars, AngleUnit unit, double (*rnd)());

// The variables (A-F X Y M) used in toks, each once, in order of first appearance. X inside
// the f of a ∫( or d/dx( is that template's own variable, so it is not listed for it.
std::vector<Tok> variablesIn(const std::vector<Tok>& toks);

// Splits at a top-level "=". False (Syntax ERROR at errorPos) for a second "=" or an empty
// side. No "=": hasEq = false, lhs = toks, rhs empty. eqPos = the "=" token's index (or -1).
bool splitEquation(const std::vector<Tok>& toks, std::vector<Tok>& lhs, std::vector<Tok>& rhs, bool& hasEq,
                   int& eqPos, int& errorPos);

// Token-level ∫ / d/dx. EvalResult.error: Syntax (errorPos inside f) or Math.
EvalResult integrateExpr(const std::vector<Tok>& f, double a, double b, double tol, const Vars& vars,
                         AngleUnit unit, double (*rnd)());
EvalResult derivativeExpr(const std::vector<Tok>& f, double x0, double tol, const Vars& vars, AngleUnit unit,
                          double (*rnd)());

struct SolveOutcome {
  enum Kind : uint8_t { Solved, CantSolve, Error } kind = Error;
  CalcError error = CalcError::None;  // Kind Error: Syntax / Math; errorPos = token in the equation
  int errorPos = 0;
  double x = 0, lr = 0;  // Solved: the root and L−R
  int iterations = 0;
};
// Solves `equation` ("lhs=rhs", or an expression meaning "= 0") for `var` from `guess`, with
// the other variables from vars (vars is not changed).
SolveOutcome solveExpr(const std::vector<Tok>& equation, Tok var, double guess, const Vars& vars, AngleUnit unit,
                       double (*rnd)());

// Called by calc::evaluate() for Tok::Integ / Tok::Deriv once a, b, tol are evaluated (b unused
// for d/dx; tol <= 0 = default). Not normally called directly.
EvalResult evalTemplate(Tok kind, const std::vector<Tok>& f, double a, double b, double tol, const Vars& vars,
                        AngleUnit unit, double (*rnd)());

const char* varName(Tok v);  // "X" for Tok::VarX ... "" for anything else

// ------------------------------------------------------------------ CALC / SOLVE screens

// How a runner finished (wantsExit()).
enum class Exit : uint8_t {
  None,  // still running
  Done,  // AC: back to editing the expression, cursor at the end
  Goto,  // ◀ / ▶ on an error or a result: edit the expression with the cursor at gotoPos()
};

// The screen as text (for tests, and platforms that draw their own way); render() draws it.
struct Screen {
  std::string rows[6];  // text rows 1-6 (row 0, the status bar, is the Device's)
  int cursorRow = -1;   // 1-6: an insert-bar cursor before character cursorCol of that row
  int cursorCol = 0;
  std::string big;      // right-aligned 2x result at the bottom ("" = none); 1x on row 6
                        // when too wide, exactly like Device::drawCalc
};

// What CalcRunner and SolveRunner share: the "X?" prompt with its input line, error screens.
class RunnerBase {
 public:
  virtual ~RunnerBase() = default;
  // A key that types a token (the Device's own key → token mapping, with SHIFT / ALPHA) goes
  // here; every other key to onKey(). Both ignore keys that mean nothing on the screen shown.
  void insert(Tok t);
  bool active() const { return started_ && exit_ == Exit::None; }
  bool wantsExit() const { return exit_ != Exit::None; }
  Exit exitKind() const { return exit_; }
  int gotoPos() const { return gotoPos_; }  // cursor position in the original expression
  virtual Screen screen() const = 0;
  void render(Framebuffer& fb) const;      // draws screen() on rows 1-6; no clear, no status bar
  // Results also go to Ans (Ans → PreAns first) when true, the default.
  void setUpdatesAns(bool on) { updatesAns_ = on; }

 protected:
  enum class Phase : uint8_t { Prompt, Result, Error };
  bool startCommon(const std::vector<Tok>& expr, Vars& vars, AngleUnit unit, NormMode norm, double (*rnd)());
  void beginPrompt(Tok var);
  bool acceptPrompt();  // = on a prompt: stores the input (if any) into the variable
  void fail(CalcError e, int pos, bool inPrompt, const char* text = nullptr);
  bool keyCommon(DKey k);  // DEL ◀ ▶ AC on prompts and error screens; true when handled
  void promptScreen(Screen& s, const std::string& title, const std::string& subtitle) const;
  void errorScreen(Screen& s) const;
  void exprRows(Screen& s, int maxRows) const;
  std::string fmt(const Num& n) const;
  std::string fmt(double v) const;
  void storeAns(const Num& v);

  std::vector<Tok> expr_;
  Vars* vars_ = nullptr;
  AngleUnit unit_ = AngleUnit::Deg;
  NormMode norm_ = NormMode::Norm1;
  double (*rnd_)() = nullptr;
  bool started_ = false, updatesAns_ = true;
  Phase phase_ = Phase::Prompt;
  Exit exit_ = Exit::None;
  int gotoPos_ = 0;
  Tok promptVar_ = Tok::VarX;
  std::vector<Tok> input_;
  int inputCursor_ = 0;
  std::string errorText_;
  bool errorInPrompt_ = false;
  int errorPos_ = 0;
};

// CALC: prompts each variable of the expression in order of appearance ("X?" with its current
// value; = keeps it or takes what was typed), then shows the result. = (or CALC) on the result
// prompts again (the Casio loop); AC ends. "Y=X+1" (one variable, "=", an expression): prompts
// the right side's variables and also stores the result into Y.
class CalcRunner : public RunnerBase {
 public:
  // False (nothing starts) for an empty expression. vars must outlive the run (the Device's).
  // An expression the runner can't take ("X+1=3") starts on its Syntax ERROR screen.
  bool start(const std::vector<Tok>& expr, Vars& vars, AngleUnit unit, NormMode norm, double (*rnd)());
  void onKey(DKey k, bool shift = false);  // = CALC AC DEL ◀ ▶
  Screen screen() const override;
  bool showingResult() const { return phase_ == Phase::Result; }
  const Num& result() const { return result_; }
  const std::vector<Tok>& prompts() const { return list_; }

 private:
  void next();  // the next prompt, or the result
  std::vector<Tok> list_;
  size_t idx_ = 0;
  std::vector<Tok> body_;  // what is evaluated (the right side of "Y=")
  int bodyOffset_ = 0;     // body_[0]'s index in expr_
  Tok assignTo_ = Tok::Count;
  Num result_;
};

// SOLVE (SHIFT CALC): "lhs=rhs" (no "=": the expression = 0). Solves for X when X is in it
// (outside a ∫/d/dx f), else for its first variable. Prompts the other variables first, then
// the solve variable ("X?" / Solve for X) whose value is the initial guess; = solves. The
// result screen shows "X=" and "L−R=" and stores the root in X. = solves again (prompting
// again, the root as the new guess); AC ends. No convergence: "Can't Solve".
class SolveRunner : public RunnerBase {
 public:
  bool start(const std::vector<Tok>& expr, Vars& vars, AngleUnit unit, NormMode norm, double (*rnd)());
  void onKey(DKey k, bool shift = false);  // = CALC AC DEL ◀ ▶
  Screen screen() const override;
  bool showingResult() const { return phase_ == Phase::Result; }
  Tok solveVar() const { return solveVar_; }
  double root() const { return x_; }
  double leftMinusRight() const { return lr_; }
  const std::vector<Tok>& prompts() const { return others_; }  // without the solve variable

 private:
  void next();
  void run();
  std::vector<Tok> others_;
  size_t idx_ = 0;
  Tok solveVar_ = Tok::VarX;
  double x_ = 0, lr_ = 0;
};

// ------------------------------------------------------------------ integration (device.*)
//  * Keys: CALC → calcRunner.start(expr_, vars_, angle_, norm_, random_); SHIFT CALC (SOLVE) →
//    solveRunner.start(...) (on the shown result's expression too). ALPHA CALC types Tok::Eq,
//    ∫dx types Tok::Integ, SHIFT ∫dx types Tok::Deriv: ordinary tokens (insert, DEL, history).
//  * While a runner is active() (its own View): if tokOf(k, shift, alpha) gives a token →
//    runner.insert(tok), else runner.onKey(k, shift). SHIFT / ALPHA stay the Device's.
//  * When wantsExit(): back to View::Calc editing expr_, showingResult_ = false;
//    Exit::Goto → cursor_ = gotoPos(), Exit::Done → cursor at the end.
//  * Ans and variables: the runners store results themselves (Ans/PreAns unless
//    setUpdatesAns(false), the "Y=" variable, SOLVE's variable). ∫ / d/dx in a normal =
//    evaluation update Ans the usual way in Device::equals().
//  * Render: Device::render() clears and draws its status bar, then runner.render(fb).

}  // namespace numeric
}  // namespace calc
