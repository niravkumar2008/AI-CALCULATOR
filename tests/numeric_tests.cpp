// Tests for core/numeric.*: ∫dx, d/dx, SOLVE and CALC (fx-115ES style).
// int runNumericTests() returns the number of failed checks (0 = all passed).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/calc_engine.h"
#include "../core/font.h"
#include "../core/framebuffer.h"
#include "../core/numeric.h"

using namespace calc;
using namespace calc::numeric;

namespace {

int g_fail = 0, g_checks = 0;

#define NCHECK(cond)                                                       \
  do {                                                                     \
    ++g_checks;                                                            \
    if (!(cond)) {                                                         \
      ++g_fail;                                                            \
      std::printf("numeric FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                      \
  } while (0)

#define NEAR(a, b, tol) NCHECK(std::fabs((a) - (b)) <= (tol))

const double kPi = 3.14159265358979323846;
double half() { return 0.5; }

// Test-only text → tokens: longest match over the tokens' own display text, plus ASCII
// spellings: * / - (minus or sign by context), ~ (−) sign, "S(" for ∫(, "pi".
// 'E' after a digit is ×10ˣ, else the variable E; 'C' is the variable C.
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
  c.push_back({"S(", Tok::Integ});
  c.push_back({"pi", Tok::Pi});
  c.push_back({"~", Tok::Neg});
  std::vector<Tok> out;
  size_t i = 0;
  auto endsOperand = [](Tok t) {
    return t <= Tok::D9 || t == Tok::Dot || t == Tok::Close || (t >= Tok::Pi && t <= Tok::VarM) || t == Tok::Sq ||
           t == Tok::Cube || t == Tok::Inv || t == Tok::Fact || t == Tok::Pct;
  };
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
      std::printf("numeric tk: can't read '%s' at %zu\n", s.c_str(), i);
      ++g_fail;
      return out;
    }
    out.push_back(bt);
    i += best;
  }
  return out;
}

EvalResult ev(const std::string& s, AngleUnit u = AngleUnit::Rad, Vars* vars = nullptr) {
  Vars v;
  return evaluate(tk(s), vars ? *vars : v, u, half);
}
double val(const std::string& s, AngleUnit u = AngleUnit::Rad, Vars* vars = nullptr) {
  const EvalResult r = ev(s, u, vars);
  if (r.error != CalcError::None) {
    std::printf("numeric: '%s' gave %s\n", s.c_str(), errorText(r.error));
    return std::nan("");
  }
  return r.value.v;
}
CalcError err(const std::string& s, AngleUnit u = AngleUnit::Rad) { return ev(s, u).error; }

const std::string kInt = "\xE2\x88\xAB(";  // ∫(
const std::string kMinus = "\xE2\x88\x92";

// A screen rendered: it has pixels, and render() matches screen()'s rows.
bool renders(const RunnerBase& r) {
  Framebuffer fb;
  fb.clear();
  r.render(fb);
  int lit = 0;
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) lit += fb.get(x, y) ? 1 : 0;
  // Every row's text drawn on its own matches the same pixels.
  const Screen s = r.screen();
  Framebuffer ref;
  ref.clear();
  for (int i = 0; i < 6; ++i)
    if (!s.rows[i].empty()) ref.drawText(0, i + 1, s.rows[i]);
  for (int i = 0; i < 6; ++i) {
    if (s.rows[i].empty() || (s.big.size() && i >= 4)) continue;
    for (int y = (i + 1) * 8; y < (i + 1) * 8 + 7; ++y)
      for (int x = 0; x < Framebuffer::kWidth; ++x)
        if (ref.get(x, y) && !fb.get(x, y)) return false;
  }
  return lit > 0;
}

void typeNum(RunnerBase& r, const std::string& s) {
  for (Tok t : tk(s)) r.insert(t);
}

// ------------------------------------------------------------------------------------ ∫

void testIntegrals() {
  // Tokens: text and glyphs.
  NCHECK(std::string(tokText(Tok::Integ)) == kInt);
  NCHECK(std::string(tokText(Tok::Deriv)) == "d/dx(");
  NCHECK(std::string(tokText(Tok::Eq)) == "=");
  NCHECK(tokOpensBracket(Tok::Integ) && tokOpensBracket(Tok::Deriv) && !tokOpensBracket(Tok::Eq));
  for (Tok t : {Tok::Integ, Tok::Deriv, Tok::Eq})
    for (uint32_t cp : decodeUtf8(tokText(t))) NCHECK(hasGlyph(cp));
  NCHECK(exprText(tk("S(X\xC2\xB2,0,1)")) == kInt + "X\xC2\xB2,0,1)");

  NEAR(val("S(X\xC2\xB2,0,1)"), 1.0 / 3, 1e-12);
  NEAR(val("S(X\xC2\xB2,0,1"), 1.0 / 3, 1e-12);  // auto-closed
  NEAR(val("S(sin(X),0,\xCF\x80)"), 2, 1e-10);
  NEAR(val("S(1/X,1,e)"), 1, 1e-9);
  NEAR(val("S(e^(X),0,1)"), std::exp(1.0) - 1, 1e-12);
  NEAR(val("S(1/\xE2\x88\x9A(X),0,1)"), 2, 2e-4);  // endpoint singularity, never evaluated at 0
  NEAR(val("S(X,1,0)"), -0.5, 1e-14);              // reversed limits
  NCHECK(val("S(X,2,2)") == 0);
  NCHECK(val("S(X\xC2\xB3,-1,1)") == 0);            // exactly 0, not 1×10⁻¹⁸
  NCHECK(val("S(sin(X),0,2\xCF\x80)") == 0);
  NEAR(val("S(sin(X),0,180)", AngleUnit::Deg), 360 / kPi, 1e-8);  // current angle unit
  NEAR(val("S(X\xC2\xB2,0,1,1E-8)"), 1.0 / 3, 1e-12);              // tol argument
  NEAR(val("2S(X,0,1)+1"), 2, 1e-14);                                // implicit ×, inside an expression
  NEAR(val("S(1/(1+X\xC2\xB2),0,1)*4"), kPi, 1e-10);
  NEAR(val("S(\xE2\x88\x9A(1-X\xC2\xB2),-1,1)"), kPi / 2, 1e-6);  // √ with vertical ends
  {
    Vars v;
    v.a = Num::ofInt(3);
    NEAR(val("S(AX,0,2)", AngleUnit::Rad, &v), 6, 1e-12);  // other variables are constants
    v.x = Num::ofInt(7);
    NEAR(val("S(X,0,1)+X", AngleUnit::Rad, &v), 7.5, 1e-12);  // X outside f is the memory X
    NCHECK(v.x.v == 7);                                         // and ∫ leaves it alone
  }
  // Failures.
  NCHECK(err("S(1/X,0,1)") == CalcError::Math);       // diverges
  NCHECK(err("S(\xE2\x88\x9A(X),-1,1)") == CalcError::Math);  // undefined on part of the range
  NCHECK(err("S(X,0)") == CalcError::Syntax);          // missing b
  NCHECK(err("S(X)") == CalcError::Syntax);
  NCHECK(err("S(,0,1)") == CalcError::Syntax);
  NCHECK(err("S(S(X,0,1),0,1)") == CalcError::Syntax);  // no nesting in f
  NCHECK(err("S(d/dx(X,1),0,1)") == CalcError::Syntax);
  NCHECK(err("S(X,0,1,0)") == CalcError::Math);         // tol must be > 0
  {
    const EvalResult r = ev("S(X+\xC3\x97" "2,0,1)");  // Syntax ERROR inside f: Goto points into f
    NCHECK(r.error == CalcError::Syntax);
    NCHECK(r.errorPos >= 1 && r.errorPos <= 3);
  }
  NCHECK(err("X=1") == CalcError::Syntax);  // "=" is only for CALC / SOLVE

  // The pure function.
  double out = 0, e = 0;
  NCHECK(integrate([](double x, double& y) { y = x * x; return true; }, 0, 3, 0, out, &e) == Status::Ok);
  NEAR(out, 9, 1e-12);
  NCHECK(integrate([](double x, double& y) { y = std::exp(-x * x); return true; }, -10, 10, 1e-10, out) == Status::Ok);
  NEAR(out, std::sqrt(kPi), 1e-10);
  // Wildly oscillating: a result or NoConverge, but it ends (bounded by kMaxIntervals).
  const Status osc = integrate([](double x, double& y) { y = std::sin(1 / x); return true; }, 0.001, 1, 0, out);
  NCHECK(osc == Status::Ok || osc == Status::NoConverge);
  if (osc == Status::Ok) NEAR(out, 0.504, 2e-3);  // ∫0..1 sin(1/x) ≈ 0.5041, minus at most 0.001
  NCHECK(integrate([](double x, double& y) { y = 1 / (x * x); return true; }, 0, 1, 0, out) != Status::Ok);
  NCHECK(integrate([](double, double&) { return false; }, 0, 1, 0, out) == Status::Domain);
}

// ------------------------------------------------------------------------------------ d/dx

void testDerivatives() {
  NEAR(val("d/dx(X\xC2\xB3,2)"), 12, 1e-8);
  NEAR(val("d/dx(sin(X),0)"), 1, 1e-10);
  NEAR(val("d/dx(sin(X),0)", AngleUnit::Deg), kPi / 180, 1e-12);  // per degree, as the Casio
  NEAR(val("d/dx(ln(X),1)"), 1, 1e-9);
  NEAR(val("d/dx(e^(X),0)"), 1, 1e-10);
  NEAR(val("d/dx(\xE2\x88\x9A(X),0.01)"), 5, 1e-6);  // near a domain edge: steps shrink
  NEAR(val("d/dx(X\xC2\xB2,3)+1"), 7, 1e-8);
  NEAR(val("d/dx(X\xC2\xB2,3,1E-12)"), 6, 1e-8);  // tol argument
  NEAR(val("d/dx(e^(10X),0)"), 10, 1e-6);
  NEAR(val("d/dx(1/X,1E6)"), -1e-12, 1e-20);
  // Not differentiable / undefined: Math ERROR (the Casio would give 0 for Abs at 0; see numeric.h).
  NCHECK(err("d/dx(Abs(X),0)") == CalcError::Math);
  NEAR(val("d/dx(Abs(X),2)"), 1, 1e-10);
  NCHECK(err("d/dx(\xE2\x88\x9A(X),0)") == CalcError::Math);
  NCHECK(err("d/dx(1/X,0)") == CalcError::Math);
  NCHECK(err("d/dx(\xC2\xB3\xE2\x88\x9A(X),0)") == CalcError::Math);
  NCHECK(err("d/dx(X)") == CalcError::Syntax);
  double out;
  NCHECK(derivative([](double x, double& y) { y = std::fabs(x); return true; }, 0, 0, out) == Status::NoConverge);
  NCHECK(derivative([](double x, double& y) { y = x * x * x * x; return true; }, -1, 0, out) == Status::Ok);
  NEAR(out, -4, 1e-8);
}

// ------------------------------------------------------------------------------------ SOLVE

SolveOutcome sv(const std::string& s, double guess, AngleUnit u = AngleUnit::Rad, Tok var = Tok::VarX,
                Vars* vars = nullptr) {
  Vars v;
  return solveExpr(tk(s), var, guess, vars ? *vars : v, u, half);
}

void testSolve() {
  SolveOutcome o = sv("X\xC2\xB2-2", 1);  // no "=": expression = 0
  NCHECK(o.kind == SolveOutcome::Solved);
  NEAR(o.x, std::sqrt(2.0), 1e-12);
  NCHECK(formatDecimal(o.x, NormMode::Norm1) == "1.414213562");
  NCHECK(o.lr == 0);
  o = sv("X\xC2\xB2=2", -1);
  NEAR(o.x, -std::sqrt(2.0), 1e-12);
  o = sv("cos(X)=X", 0);
  NCHECK(o.kind == SolveOutcome::Solved);
  NCHECK(formatDecimal(o.x, NormMode::Norm1) == "0.7390851332");
  o = sv("X\xC2\xB2+1=0", 1);
  NCHECK(o.kind == SolveOutcome::CantSolve);
  o = sv("1/X=0", 1);
  NCHECK(o.kind == SolveOutcome::CantSolve);
  o = sv("e^(X)=10", 0);
  NEAR(o.x, std::log(10.0), 1e-12);
  o = sv("X\xC2\xB3-2X+2=0", 0);  // plain Newton cycles 0 ↔ 1 here
  NCHECK(o.kind == SolveOutcome::Solved);
  NCHECK(formatDecimal(o.x, NormMode::Norm1) == kMinus + "1.769292354");
  o = sv("X\xC2\xB2=0", 1);  // double root: slow but fine
  NCHECK(o.kind == SolveOutcome::Solved && std::fabs(o.x) < 1e-5);
  o = sv("ln(X)=1", 0);  // undefined at the guess
  NCHECK(o.kind == SolveOutcome::Error && o.error == CalcError::Math);
  o = sv("\xE2\x88\x9A(X)=2", 1);
  NEAR(o.x, 4, 1e-10);
  o = sv("\xE2\x88\x9A(X)=2", -5);
  NCHECK(o.kind == SolveOutcome::Error && o.error == CalcError::Math);
  o = sv("1E10(X\xC2\xB2-2)", 3);  // big coefficients: L−R not tiny, root still exact
  NEAR(o.x, std::sqrt(2.0), 1e-12);
  o = sv("X+\xC3\x97", 1);
  NCHECK(o.kind == SolveOutcome::Error && o.error == CalcError::Syntax);
  o = sv("X=1=2", 1);
  NCHECK(o.kind == SolveOutcome::Error && o.error == CalcError::Syntax && o.errorPos == 3);
  o = sv("X=", 1);
  NCHECK(o.kind == SolveOutcome::Error && o.error == CalcError::Syntax);
  o = sv("tan(X)=1", 0.5);
  NEAR(o.x, kPi / 4, 1e-12);
  o = sv("sin(X)=0.5", 10, AngleUnit::Deg);
  NEAR(o.x, 30, 1e-9);
  {
    Vars v;
    v.a = Num::ofInt(2);
    v.b = Num::ofInt(-6);
    o = sv("AX+B=0", 0, AngleUnit::Rad, Tok::VarX, &v);
    NEAR(o.x, 3, 1e-12);
    NCHECK(v.x.v == 0);  // solveExpr leaves vars alone
  }
  o = sv("Y\xC2\xB2=9", 1, AngleUnit::Rad, Tok::VarY);
  NEAR(o.x, 3, 1e-12);
  o = sv("S(X,0,A)=2", 1, AngleUnit::Rad, Tok::VarA);  // an equation with ∫ in it
  NEAR(o.x, 2, 1e-8);
  // The pure function.
  double x, lr;
  NCHECK(solve([](double t, double& l, double& r) { l = t * t * t; r = 27; return true; }, 1, x, lr) == Status::Ok);
  NEAR(x, 3, 1e-12);
  NCHECK(solve([](double t, double& l, double& r) { l = std::log(t); r = 0; return t > 0; }, -1, x, lr) ==
         Status::Domain);
}

// ------------------------------------------------------------------------------------ token helpers

void testHelpers() {
  std::vector<Tok> v = variablesIn(tk("B+AX+A+Y=M"));
  NCHECK(v.size() == 5 && v[0] == Tok::VarB && v[1] == Tok::VarA && v[2] == Tok::VarX && v[3] == Tok::VarY &&
         v[4] == Tok::VarM);
  v = variablesIn(tk("S(XA,0,B)+d/dx(X,C)"));  // X in f is bound
  NCHECK(v.size() == 3 && v[0] == Tok::VarA && v[1] == Tok::VarB && v[2] == Tok::VarC);
  v = variablesIn(tk("S(X,0,X)"));  // ...but X as a limit is the memory X
  NCHECK(v.size() == 1 && v[0] == Tok::VarX);
  NCHECK(variablesIn(tk("Ans+PreAns+2")).empty());
  std::vector<Tok> l, r;
  bool eq;
  int p, e;
  NCHECK(splitEquation(tk("Y=X+1"), l, r, eq, p, e) && eq && p == 1 && l.size() == 1 && r.size() == 3);
  NCHECK(splitEquation(tk("X+1"), l, r, eq, p, e) && !eq && p == -1 && l.size() == 3 && r.empty());
  NCHECK(!splitEquation(tk("=2"), l, r, eq, p, e) && e == 0);
  NCHECK(std::string(varName(Tok::VarM)) == "M" && std::string(varName(Tok::Pi)).empty());
  Vars vars;
  vars.y = Num::ofInt(4);
  const EvalResult er = evalWith(tk("X+Y"), Tok::VarX, 1.5, vars, AngleUnit::Rad, nullptr);
  NCHECK(er.error == CalcError::None && er.value.v == 5.5);
}

// ------------------------------------------------------------------------------------ CALC

void testCalcRunner() {
  Vars vars;
  CalcRunner c;
  NCHECK(!c.start({}, vars, AngleUnit::Rad, NormMode::Norm1, half));
  NCHECK(!c.active());

  // X²+1: prompt X (current value 0), type 3, = → 10; = loops, AC ends.
  NCHECK(c.start(tk("X\xC2\xB2+1"), vars, AngleUnit::Rad, NormMode::Norm1, half));
  NCHECK(c.active() && !c.showingResult());
  Screen s = c.screen();
  NCHECK(s.rows[0] == "X?" && s.big == "0");
  NCHECK(renders(c));
  typeNum(c, "3");
  s = c.screen();
  NCHECK(s.rows[5] == "3" && s.cursorRow == 6 && s.cursorCol == 1 && s.big.empty());
  NCHECK(renders(c));
  c.onKey(DKey::Eq);
  NCHECK(c.showingResult() && c.result().v == 10);
  NCHECK(vars.x.v == 3 && vars.ans.v == 10);
  s = c.screen();
  NCHECK(s.rows[0] == "X\xC2\xB2+1" && s.big == "10");
  NCHECK(renders(c));
  c.onKey(DKey::Eq);  // again: prompt X showing 3
  s = c.screen();
  NCHECK(!c.showingResult() && s.rows[0] == "X?" && s.big == "3");
  typeNum(c, "12");
  c.onKey(DKey::Del);  // DEL the 2
  c.onKey(DKey::Eq);
  NCHECK(c.result().v == 2 && vars.preAns.v == 10 && vars.ans.v == 2);
  c.onKey(DKey::Calc);  // CALC also loops
  NCHECK(!c.showingResult());
  c.onKey(DKey::Eq);    // = alone keeps X = 1
  NCHECK(c.result().v == 2);
  c.onKey(DKey::AC);
  NCHECK(c.wantsExit() && c.exitKind() == Exit::Done && !c.active());
  c.onKey(DKey::Eq);  // ignored once finished
  NCHECK(c.exitKind() == Exit::Done);

  // Y=X+1: prompts X only, stores into Y.
  vars = Vars();
  NCHECK(c.start(tk("Y=X+1"), vars, AngleUnit::Rad, NormMode::Norm1, half));
  NCHECK(c.prompts().size() == 1 && c.prompts()[0] == Tok::VarX);
  typeNum(c, "4");
  c.onKey(DKey::Eq);
  NCHECK(c.showingResult() && vars.y.v == 5 && vars.ans.v == 5);
  NCHECK(c.screen().rows[0] == "Y=X+1");
  c.onKey(DKey::Left);  // ◀ on the result: edit, cursor at the end
  NCHECK(c.exitKind() == Exit::Goto && c.gotoPos() == 5);

  // A+B: A then B, in order of appearance; an expression typed at a prompt.
  vars = Vars();
  c.start(tk("B-A"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(c.screen().rows[0] == "B?");
  typeNum(c, "2*5");
  c.onKey(DKey::Eq);
  NCHECK(c.screen().rows[0] == "A?");
  typeNum(c, "pi");
  c.onKey(DKey::Eq);
  NEAR(c.result().v, 10 - kPi, 1e-12);

  // No variables: straight to the result.
  vars = Vars();
  c.start(tk("2+3"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(c.showingResult() && c.result().v == 5);
  c.onKey(DKey::Eq);  // and = recalculates
  NCHECK(c.showingResult());

  // A fraction shown as a fraction, as on the calculator screen.
  c.start(tk("X/3"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  typeNum(c, "1");
  c.onKey(DKey::Eq);
  NCHECK(c.screen().big == "1\xE2\x8C\x9F" "3");

  // Math ERROR in the expression: ◀ / ▶ = Goto in the expression.
  vars = Vars();
  c.start(tk("1/X"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  c.onKey(DKey::Eq);  // X = 0
  s = c.screen();
  NCHECK(s.rows[0] == "  Math ERROR" && s.rows[2] == "  [AC]  :Cancel");
  NCHECK(renders(c));
  c.onKey(DKey::D5);  // nothing on an error screen
  NCHECK(c.active());
  c.onKey(DKey::Right);
  NCHECK(c.exitKind() == Exit::Goto && c.gotoPos() >= 0 && c.gotoPos() <= 3);

  // An error in the typed value: ▶ goes back to the prompt with the input, AC ends.
  vars = Vars();
  c.start(tk("X+1"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  typeNum(c, "1/0");
  c.onKey(DKey::Eq);
  NCHECK(c.screen().rows[0] == "  Math ERROR");
  c.onKey(DKey::Right);
  s = c.screen();
  NCHECK(c.active() && s.rows[0] == "X?" && s.rows[5] == "1\xC3\xB7" "0");
  NCHECK(s.cursorRow == 6 && s.cursorCol >= 1 && s.cursorCol <= 3);  // at the error
  c.onKey(DKey::Right);
  c.onKey(DKey::Right);
  c.onKey(DKey::Right);
  c.onKey(DKey::Del);
  c.onKey(DKey::Del);
  c.onKey(DKey::Left);
  c.onKey(DKey::Right);
  typeNum(c, "+1");
  c.onKey(DKey::Eq);
  NCHECK(c.showingResult() && c.result().v == 3);
  c.onKey(DKey::AC);
  NCHECK(c.exitKind() == Exit::Done);

  // AC on a prompt ends at once.
  c.start(tk("X+1"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  c.onKey(DKey::AC);
  NCHECK(c.exitKind() == Exit::Done && vars.ans.v == 3);

  // "X+1=3" can't be calculated: Syntax ERROR at the "=".
  c.start(tk("X+1=3"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(c.screen().rows[0] == "  Syntax ERROR");
  c.onKey(DKey::Left);
  NCHECK(c.exitKind() == Exit::Goto && c.gotoPos() == 3);

  // X inside ∫'s f is not prompted.
  vars = Vars();
  c.start(tk("S(X,0,A)"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(c.prompts().size() == 1 && c.prompts()[0] == Tok::VarA);
  typeNum(c, "2");
  c.onKey(DKey::Eq);
  NEAR(c.result().v, 2, 1e-12);
  NCHECK(renders(c));

  // Ans not touched when asked.
  vars = Vars();
  c.setUpdatesAns(false);
  c.start(tk("7"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(c.result().v == 7 && vars.ans.v == 0);

  // A long typed value scrolls so the cursor stays on screen.
  vars = Vars();
  CalcRunner c2;
  c2.start(tk("X"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  typeNum(c2, "1234567890123456789012345678901");
  s = c2.screen();
  NCHECK(s.cursorRow == 6 && s.cursorCol == 24);
  NCHECK(renders(c2));
}

// ------------------------------------------------------------------------------------ SOLVE flow

void testSolveRunner() {
  Vars vars;
  SolveRunner sr;
  NCHECK(!sr.start({}, vars, AngleUnit::Rad, NormMode::Norm1, half));

  vars.x = Num::ofInt(1);
  vars.ans = Num::ofInt(42);
  NCHECK(sr.start(tk("X\xC2\xB2=2"), vars, AngleUnit::Rad, NormMode::Norm1, half));
  Screen s = sr.screen();
  NCHECK(s.rows[0] == "X?" && s.rows[1] == "Solve for X" && s.big == "1");
  NCHECK(renders(sr));
  sr.onKey(DKey::Eq);
  NCHECK(sr.showingResult());
  s = sr.screen();
  NCHECK(s.rows[0] == "X\xC2\xB2=2");
  NCHECK(s.rows[4] == "X=            1.414213562");
  NCHECK(s.rows[5] == "L" + kMinus + "R=" + std::string(20, ' ') + "0");  // 25 columns
  NCHECK(renders(sr));
  NEAR(vars.x.v, std::sqrt(2.0), 1e-12);
  NCHECK(vars.ans.v == 42);  // SOLVE leaves Ans alone by default
  sr.onKey(DKey::Eq);        // solve again from the root
  s = sr.screen();
  NCHECK(s.rows[0] == "X?" && s.big == "1.414213562");
  typeNum(sr, "-3");
  sr.onKey(DKey::Eq);
  NEAR(vars.x.v, -std::sqrt(2.0), 1e-12);
  sr.onKey(DKey::AC);
  NCHECK(sr.exitKind() == Exit::Done);

  // Several variables: the others first, in order of appearance, then X.
  vars = Vars();
  sr.start(tk("AX+B"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(sr.prompts().size() == 2 && sr.solveVar() == Tok::VarX);
  NCHECK(sr.screen().rows[0] == "A?" && sr.screen().rows[1].empty());
  typeNum(sr, "2");
  sr.onKey(DKey::Eq);
  NCHECK(sr.screen().rows[0] == "B?");
  typeNum(sr, "-6");
  sr.onKey(DKey::Eq);
  NCHECK(sr.screen().rows[0] == "X?");
  sr.onKey(DKey::Eq);
  NCHECK(sr.showingResult() && sr.root() == 3 && vars.x.v == 3 && vars.x.exact);
  NCHECK(sr.screen().rows[4] == "X=" + std::string(22, ' ') + "3");

  // No X: solves for the first variable.
  vars = Vars();
  sr.start(tk("Y\xC2\xB2=B"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(sr.solveVar() == Tok::VarY && sr.prompts().size() == 1 && sr.prompts()[0] == Tok::VarB);
  typeNum(sr, "9");
  sr.onKey(DKey::Eq);
  NCHECK(sr.screen().rows[0] == "Y?" && sr.screen().rows[1] == "Solve for Y");
  typeNum(sr, "1");
  sr.onKey(DKey::Eq);
  NEAR(vars.y.v, 3, 1e-12);
  NCHECK(sr.screen().rows[4].compare(0, 2, "Y=") == 0);

  // cos X = X in Rad.
  vars = Vars();
  sr.start(tk("cos(X)=X"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  sr.onKey(DKey::Eq);
  NCHECK(sr.screen().rows[4] == "X=           0.7390851332");

  // No solution: Can't Solve; AC ends, ◀ goes to the equation.
  vars = Vars();
  sr.start(tk("X\xC2\xB2+1=0"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  sr.onKey(DKey::Eq);
  s = sr.screen();
  NCHECK(s.rows[0] == "  Can't Solve" && s.rows[3] == "  [\xE2\x97\x80][\xE2\x96\xB6]:Goto");
  NCHECK(renders(sr));
  sr.onKey(DKey::Left);
  NCHECK(sr.exitKind() == Exit::Goto && sr.gotoPos() == 0);
  sr.start(tk("X\xC2\xB2+1=0"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  sr.onKey(DKey::Calc, true);  // SHIFT CALC works like = on the prompt
  NCHECK(sr.screen().rows[0] == "  Can't Solve");
  sr.onKey(DKey::AC);
  NCHECK(sr.exitKind() == Exit::Done);

  // Undefined at the guess: Math ERROR.
  vars = Vars();
  sr.start(tk("ln(X)=1"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  sr.onKey(DKey::Eq);
  NCHECK(sr.screen().rows[0] == "  Math ERROR");
  // Nothing to solve for / two "=": Syntax ERROR.
  sr.start(tk("2=3"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(sr.screen().rows[0] == "  Syntax ERROR");
  sr.start(tk("X=1=2"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  NCHECK(sr.screen().rows[0] == "  Syntax ERROR");
  sr.onKey(DKey::Right);
  NCHECK(sr.exitKind() == Exit::Goto && sr.gotoPos() == 3);

  // Updating Ans when asked (after start(), which resets SOLVE's default).
  vars = Vars();
  sr.start(tk("2X=8"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  sr.setUpdatesAns(true);
  sr.onKey(DKey::Eq);
  NCHECK(vars.x.v == 4 && vars.ans.v == 4);

  // Big-number L−R and a long equation over several rows.
  vars = Vars();
  sr.start(tk("X\xC2\xB3+2X\xC2\xB2+3X+4+5X+6X+7X+8X+9X+10X+11X=12"), vars, AngleUnit::Rad, NormMode::Norm1, half);
  sr.onKey(DKey::Eq);
  NCHECK(sr.showingResult());
  NCHECK(!sr.screen().rows[1].empty());
  NCHECK(renders(sr));
}

// ------------------------------------------------------------------------------------ bug hunt 2026-10-11

std::vector<Tok> repeat(Tok t, int n) { return std::vector<Tok>(static_cast<size_t>(n), t); }
void append(std::vector<Tok>& a, const std::vector<Tok>& b) { a.insert(a.end(), b.begin(), b.end()); }
CalcError errOf(const std::vector<Tok>& t) {
  Vars v;
  return evaluate(t, v, AngleUnit::Rad, half).error;
}
int g_hookCalls = 0;
bool countingHook() {
  ++g_hookCalls;
  return false;
}
bool stopHook() { return true; }

void testBugHunt() {
  // R-1: one nesting budget (kMaxNesting = 24, Casio's limit), shared with ∫ / d/dx's f; one more
  // level is Stack ERROR, not a stack overflow. 24 brackets and 24 functions evaluate.
  {
    std::vector<Tok> t = repeat(Tok::Open, kMaxNesting);
    t.push_back(Tok::D1);
    NCHECK(errOf(t) == CalcError::None);
    t.insert(t.begin(), Tok::Open);
    NCHECK(errOf(t) == CalcError::Stack);
    t = repeat(Tok::Sin, kMaxNesting);
    t.push_back(Tok::D0);
    NCHECK(errOf(t) == CalcError::None);
    t.insert(t.begin(), Tok::Cos);
    NCHECK(errOf(t) == CalcError::Stack);
    t = {Tok::D2};  // 2^(2^(2^(... 24 exponents deep
    for (int i = 0; i < kMaxNesting; ++i) append(t, {Tok::Pow, Tok::D1});
    for (int i = 0; i < kMaxNesting; ++i) t.push_back(Tok::Close);
    NCHECK(errOf(t) == CalcError::None);
    t = repeat(Tok::Neg, 98);  // a run of (−) is counted, not recursed into
    t.push_back(Tok::D3);
    Vars v;
    const EvalResult r = evaluate(t, v, AngleUnit::Rad, half);
    NCHECK(r.error == CalcError::None && r.value.v == 3);
    // ∫ at outer depth n with f nested m deep: allowed while n + 1 + kTemplateDepth + m <= 24.
    for (Tok kind : {Tok::Integ, Tok::Deriv})
      for (int n : {0, 10, kMaxNesting - 1 - kTemplateDepth}) {
        const int m = kMaxNesting - n - 1 - kTemplateDepth;
        for (int extra = 0; extra < 2; ++extra) {
          std::vector<Tok> e = repeat(Tok::Open, n);
          e.push_back(kind);
          append(e, repeat(Tok::Open, m + extra));
          e.push_back(Tok::VarX);
          append(e, repeat(Tok::Close, m + extra));
          append(e, {Tok::Comma, Tok::D0});
          if (kind == Tok::Integ) append(e, {Tok::Comma, Tok::D1});
          const CalcError ce = errOf(e);
          NCHECK(extra == 0 ? ce == CalcError::None : ce == CalcError::Stack);
        }
      }
    // Too deep inside f: the Stack ERROR's Goto points into f.
    std::vector<Tok> e = {Tok::Integ};
    append(e, repeat(Tok::Open, kMaxNesting));
    e.push_back(Tok::VarX);
    append(e, repeat(Tok::Close, kMaxNesting));
    append(e, {Tok::Comma, Tok::D0, Tok::Comma, Tok::D1});
    Vars v2;
    const EvalResult rs = evaluate(e, v2, AngleUnit::Rad, half);
    NCHECK(rs.error == CalcError::Stack && rs.errorPos >= 1 && rs.errorPos <= kMaxNesting + 1);
  }
  // R-2: d/dx on ordinary inputs (a first step of 0.1·|x0| straddled whole periods).
  NEAR(val("d/dx(cos(X),100)"), -std::sin(100.0), 1e-9);
  NEAR(val("d/dx(e^(X),100)") / std::exp(100.0), 1, 1e-9);
  NEAR(val("d/dx(sin(50X),0)"), 50, 1e-8);
  NEAR(val("d/dx(sin(X),3600)", AngleUnit::Deg), kPi / 180, 1e-12);
  NEAR(val("d/dx(ln(X),1E-4)"), 1e4, 1e-5);
  NCHECK(val("d/dx(XAbs(X),0)") == 0);
  NEAR(val("d/dx(X^(X),50)") / (std::pow(50.0, 50.0) * (std::log(50.0) + 1)), 1, 1e-9);
  NCHECK(err("d/dx(Abs(X),0)") == CalcError::Math);  // still a corner
  // R-3: one budget of f evaluations per run, and a cancel hook (AC on the device).
  {
    g_hookCalls = 0;
    setCancelHook(countingHook);
    const SolveOutcome o = sv("S(cos(X),0,A)=5", 0, AngleUnit::Rad, Tok::VarA);  // no root: |∫| <= 1
    setCancelHook(nullptr);
    NCHECK(o.kind == SolveOutcome::CantSolve);
    NCHECK(g_hookCalls * 64 <= kMaxFunctionEvals + 64);  // stopped by the budget (polled every 64)
    setCancelHook(stopHook);  // AC pressed: ∫ is Math ERROR, SOLVE Can't Solve, at once
    NCHECK(err("S(sin(X),0,1)") == CalcError::Math);
    NCHECK(sv("S(X,0,A)=2", 1, AngleUnit::Rad, Tok::VarA).kind == SolveOutcome::CantSolve);
    NEAR(val("2+3"), 5, 0);  // no f evaluations: never stopped
    setCancelHook(nullptr);
    NEAR(val("S(sin(X),0,1)"), 1 - std::cos(1.0), 1e-12);  // a new run: a fresh budget
  }
  // R-7: no false convergence on one piece's nodes; R-8: a singularity at an interior node.
  NEAR(val("S(1/(1+X\xC2\xB2),-1E10,1E10)"), kPi, 1e-6);
  NEAR(val("S(1/(1+X\xC2\xB2),0,1E10)"), kPi / 2, 1e-6);
  NEAR(val("S(e^(-X),0,1E5)"), 1, 1e-6);
  NEAR(val("S(ln(Abs(X)),-1,1)"), -2, 1e-4);
  NEAR(val("S(1/\xE2\x88\x9A(Abs(X)),-1,1)"), 4, 1e-3);
  NEAR(val("S(ln(Abs(X)),-1,2)"), 2 * std::log(2.0) - 3, 1e-4);  // off-centre: split where undefined
  NCHECK(err("S(1/X,-1,1)") == CalcError::Math);                     // not integrable: still an error
  // R-10: relative convergence: a root at 1E-15, not a stop at 9E-13 with L−R = 0.
  {
    SolveOutcome o = sv("X\xC2\xB2-1E-30", 1);
    NCHECK(o.kind == SolveOutcome::Solved);
    NEAR(o.x / 1e-15, 1, 1e-9);
    o = sv("X\xC2\xB2-4E-24", 1);
    NEAR(o.x / 2e-12, 1, 1e-9);
    o = sv("X\xC2\xB2=0", 1);  // a double root at 0 still ends
    NCHECK(o.kind == SolveOutcome::Solved && std::fabs(o.x) < 1e-11);
    o = sv("sin(X)", 1);
    NCHECK(o.kind == SolveOutcome::Solved && std::fabs(o.x) < 1e-20);
  }
}

}  // namespace

int runNumericTests() {
  g_fail = 0;
  g_checks = 0;
  testIntegrals();
  testDerivatives();
  testSolve();
  testHelpers();
  testCalcRunner();
  testSolveRunner();
  testBugHunt();
  std::printf("numeric: %d checks, %d failed\n", g_checks, g_fail);
  return g_fail;
}
