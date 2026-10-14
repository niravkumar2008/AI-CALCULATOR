// EQN mode (MODE 5), like the Casio fx-115ES / fx-991ES equation solver in linear display.
//
// Self-contained, like STAT and TABLE (core/stat.h, core/table.h): a key-driven state machine
// and a render for the 125x61 mono screen (rows 1-6; row 0 is the host's status bar).
// Coefficients are typed as expressions and evaluated with calc_engine; the solvers are the
// free functions in calc::eqn (unit-tested on their own).
//
// Screens and keys (DKey; the host handles the SHIFT / ALPHA keys and passes them as flags):
//   TypeSelect  1:anX+bnY=cn  2:anX+bnY+cnZ=dn  3:aX²+bX+c=0  4:aX³+bX²+cX+d=0. A digit picks
//               the type and opens the editor (picking another type zeroes the coefficients;
//               the same type keeps them). AC: back to the editor when a type was chosen
//               before, else wantsExit() (the host goes back to COMP, as for STAT).
//   Editor      the coefficient grid: columns a b c (d), one row per equation (2 or 3) or one
//               row for a polynomial. Arrows move (no wrap). A value key starts typing an
//               expression for the cell (shown on the bottom line); = stores its value and
//               moves to the next cell (right, then the next row's first); DEL backspaces
//               while typing, else sets the cell to 0; AC cancels typing, else goes to the type
//               list. = while not typing solves.
//   Result      one value per screen: "X=" "Y=" ("Z="), or "X1=" "X2=" ("X3=") / "X=" for a
//               polynomial (distinct roots; complex roots as a+bi, as the fx-115ES shows them
//               in EQN even outside CMPLX), and for a quadratic then "X-Value Minimum=" /
//               "Y-Value Minimum=" (Maximum when a < 0), as the newer Casio models do. ▼ / =
//               next, ▲ previous; = after the last and AC go back to the editor. S⇔D swaps
//               fraction / decimal. Results are NOT stored to Ans (the Casio doesn't either).
//   Message     "No Solution" / "Infinite Solution" (a singular system, as the Casio says);
//               AC, =, ◀ ▶ go back to the editor.
//   Error       "Math ERROR" (a = 0 for a polynomial, a coefficient or a root out of range),
//               "Syntax ERROR"... while typing a cell. AC goes back to the editor (dropping
//               the typed entry), ◀ ▶ back to it with the entry kept.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "calc_engine.h"
#include "framebuffer.h"
#include "keys.h"  // calc::DKey

namespace calc {

enum class EqnType : uint8_t {
  Lin2,   // anX+bnY=cn        2 equations, 2 unknowns
  Lin3,   // anX+bnY+cnZ=dn    3 equations, 3 unknowns
  Quad,   // aX²+bX+c=0
  Cubic,  // aX³+bX²+cX+d=0
  Count
};
const char* eqnTypeName(EqnType t);  // "anX+bnY=cn" ... (UTF-8: ² ³)

// ----------------------------------------------------------------------------- the solvers
namespace eqn {

struct Root {
  double re = 0, im = 0;
};

enum class Outcome : uint8_t { Ok, NoSolution, Infinite, MathError };

// n = 2 or 3 equations; `a` is n rows of n + 1 doubles (the coefficients, then the constant),
// row after row. Gaussian elimination with partial pivoting on a row- and column-equilibrated
// copy; a pivot below 1E-12 of its (equilibrated) column is no pivot. Rank deficient: No
// Solution if the constants disagree, else Infinite Solution. x gets n values (Ok only);
// MathError for a value that isn't finite or a solution out of range (|x| >= 1E100).
Outcome solveLinear(int n, const double* a, double* x);

// aX² + bX + c = 0. Ok: `count` distinct roots (1 for a double root) in out[0..count-1], in
// the Casio's order X1 = (−b + √D) / 2a, X2 = (−b − √D) / 2a; a complex pair as re ± im·i,
// +im first. The discriminant is computed with a compensated (fma) product and is 0 within
// rounding noise. MathError when a = 0, a value isn't finite, or a root is out of range.
// The vertex: xv = −b / 2a, yv = c − b² / 4a (yv from the same compensated discriminant).
Outcome solveQuadratic(double a, double b, double c, Root out[2], int& count, double* xv = nullptr,
                       double* yv = nullptr);

// aX³ + bX² + cX + d = 0. Ok: `count` distinct roots (1 for a triple root, 2 for a double
// root): the real ones from largest to smallest, then a complex pair (+im first). Closed forms
// on the depressed cubic with rounding-noise tolerances for the double / triple cases (so a
// repeated root has no spurious imaginary part), real roots polished by Newton's method, the
// complex pair from the deflated quadratic, |im| below 1E-12 of the roots' size snapped to 0.
// MathError when a = 0, a value isn't finite, or a root is out of range.
Outcome solveCubic(double a, double b, double c, double d, Root out[3], int& count);

// A result as the calculator shows it: real "1.5" (or "3⌟2" with preferFraction), complex
// "1+2i", "−0.5−0.8660254038i", "i".
std::string rootText(const Root& r, bool preferFraction, NormMode norm);

}  // namespace eqn

// ----------------------------------------------------------------------------- the mode

class EqnApp {
 public:
  enum class Screen : uint8_t { TypeSelect, Editor, Result, Message, Error };
  static constexpr int kMaxRows = 3, kMaxCols = 4;
  static constexpr int kMaxResults = 5;  // a quadratic: X1, X2, X-Value, Y-Value (+1 spare)

  struct Result {
    std::string label;  // "X=", "X1=", "X-Value Minimum="
    eqn::Root value;
  };

  EqnApp() { reset(); }

  // Fresh start (what MODE 5 does): the type list, no type chosen, all coefficients 0.
  // Keeps setEnv().
  void reset();
  // The calculator's variables (read by coefficient expressions; never changed: EQN keeps
  // Ans as it is), angle unit, Norm mode, Ran# source.
  void setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)());
  AngleUnit angleUnit() const { return unit_; }
  NormMode normMode() const { return norm_; }

  // Returns true if the key changed anything (redraw).
  bool onKey(DKey k, bool shift, bool alpha);
  // Draws rows 1-6 and sets the Up / Down icons. Doesn't clear.
  void render(Framebuffer& fb) const;
  // AC on the type list before any type was chosen: the host may leave EQN.
  bool wantsExit() const { return exit_; }
  void resume() { exit_ = false; }

  // --- type and coefficients
  bool typeChosen() const { return chosen_; }
  EqnType type() const { return type_; }
  void setType(EqnType t);  // opens the editor at the first cell (a new type zeroes the grid)
  static int rowsOf(EqnType t);  // 2, 3, 1, 1
  static int colsOf(EqnType t);  // 3, 4, 3, 4
  int rows() const { return rowsOf(type_); }
  int cols() const { return colsOf(type_); }
  static const char* columnName(int c);  // "a" "b" "c" "d"
  double coef(int r, int c) const;
  bool setCoef(int r, int c, double v);  // false out of the grid or not finite / |v| >= 1E100

  // --- editor
  int curRow() const { return row_; }
  int curCol() const { return col_; }
  bool typing() const { return typing_; }
  const std::vector<Tok>& entry() const { return entry_; }

  // --- solving (= in the editor)
  bool solve();  // true: the Result screen; else the Message / Error screen
  eqn::Outcome outcome() const { return outcome_; }
  int resultCount() const { return nResults_; }
  int resultIndex() const { return shown_; }
  const Result& result(int i) const { return results_[i < 0 ? 0 : (i >= kMaxResults ? kMaxResults - 1 : i)]; }
  std::string resultText(int i) const;  // rootText(): a fraction unless S⇔D
  std::string shownLabel() const { return nResults_ ? result(shown_).label : std::string(); }
  std::string shownText() const { return nResults_ ? resultText(shown_) : std::string(); }
  bool showFraction() const { return showFraction_; }
  const char* message() const { return message_; }  // Message / Error screens

  Screen screen() const { return screen_; }

 private:
  Vars& vars() const;
  void keyTypeSelect(DKey k);
  void keyEditor(DKey k, bool shift, bool alpha);
  void keyResult(DKey k, bool alpha);
  void commitEntry();
  void showError(const char* msg);
  void drawEditor(Framebuffer& fb) const;
  void drawResult(Framebuffer& fb) const;
  void addResult(const std::string& label, double re, double im = 0);

  Screen screen_ = Screen::TypeSelect;
  EqnType type_ = EqnType::Lin2;
  bool chosen_ = false;
  double coef_[kMaxRows][kMaxCols] = {};
  int row_ = 0, col_ = 0;
  bool typing_ = false;
  std::vector<Tok> entry_;
  eqn::Outcome outcome_ = eqn::Outcome::Ok;
  Result results_[kMaxResults];
  int nResults_ = 0, shown_ = 0;
  bool showFraction_ = true;
  const char* message_ = "";
  bool exit_ = false;

  Vars* vars_ = nullptr;
  mutable Vars ownVars_;
  AngleUnit unit_ = AngleUnit::Deg;
  NormMode norm_ = NormMode::Norm1;
  double (*random_)() = nullptr;
};

}  // namespace calc
