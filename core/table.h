// TABLE mode (MODE 3), like the Casio fx-115ES / fx-991ES in linear display.
//
// Self-contained: a key-driven state machine and a render for the 125x61 mono screen
// (rows 1-6; row 0 is the host's status bar). f(X) is a calc_engine token list in which
// Tok::VarX is the variable; each row is calc::evaluate() with Vars::x set to the row's X.
//
// Screens and keys (DKey; the host handles the SHIFT / ALPHA keys and passes them as flags):
//   Function  "f(X)=" and the expression. Keys type as in COMP (ALPHA ) = X); DEL
//             backspaces; AC clears it (on an empty line: wantsExit(), the host may go
//             back to COMP); = asks for the range.
//   Start / End / Step  "Start?" with the current value (defaults 1, 5, 1) right-aligned
//             under it. Typing replaces it (an expression, evaluated); = keeps / stores and
//             moves on; after Step the table is built. AC goes back to f(X)=.
//   Table     X and F(X), up to 30 rows, 4 visible (rows 2-5); ▲▼ move (the status bar
//             arrows show more rows), ◀ ▶ pick the column; the bottom line shows the
//             selected value in full (10 digits, calc_engine formatting), "ERROR" for a row
//             whose f(X) fails. AC goes back to f(X)= with the expression kept.
//   Error     "Math ERROR" (Step 0, a Step pointing away from End, a value error in
//             Start / End / Step), "Insufficient MEM" (more than 30 rows, as the Casio says),
//             "Syntax ERROR"; AC (clearing the input) or ◀ ▶ go back to the prompt that
//             caused it (Start? for a range error).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "calc_engine.h"
#include "framebuffer.h"
#include "keys.h"  // calc::DKey

namespace calc {

class TableApp {
 public:
  enum class Screen : uint8_t { Function, Start, End, Step, Table, Error };
  static constexpr int kMaxRows = 30;     // the Casio's limit
  static constexpr int kVisibleRows = 4;  // table rows on the mono screen (rows 2-5)

  TableApp() { reset(); }

  // Fresh start (what MODE 3 does): empty f(X)=, Start 1, End 5, Step 1. Keeps setEnv().
  void reset();
  // The calculator's variables (A-F, Y, M, Ans for expressions; X is set per row on a
  // copy, so the host's X is untouched), angle unit, Norm mode, Ran# source.
  void setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)());
  AngleUnit angleUnit() const { return unit_; }  // as setEnv() last set them
  NormMode normMode() const { return norm_; }

  // Returns true if the key changed anything (redraw).
  bool onKey(DKey k, bool shift, bool alpha);
  // Draws rows 1-6 and sets the Up / Down icons. Doesn't clear.
  void render(Framebuffer& fb) const;
  // AC on an empty f(X)=: the host may leave TABLE.
  bool wantsExit() const { return exit_; }
  void resume() { exit_ = false; }

  // --- the function (the host may supply it instead of the keys)
  void setFunction(const std::vector<Tok>& toks);  // Tok::VarX = X; at most kMaxTokens
  // Text such as "X^2+1", "2sin(x)", "AX+B": parseExpression() with every X / x (not the
  // x of "exp(") read as the variable X, and a lone capital A B C D F Y M as that memory
  // (E stays ×10ⁿ / e). False (function unchanged) if it doesn't parse.
  bool setFunctionText(const std::string& text);
  const std::vector<Tok>& function() const { return func_; }
  std::string functionText() const { return exprText(func_); }

  // --- the range
  void setRange(double start, double end, double step);
  double start() const { return start_; }
  double end() const { return end_; }
  double step() const { return step_; }
  // Rows the range gives: >= 1; 0 for a bad range (Step 0, or pointing away from End);
  // more than kMaxRows means "Insufficient MEM".
  static int rowCount(double start, double end, double step);
  // Row i's X: start + i·step without binary rounding noise (0 below 1E-9 of a step, else
  // rounded to 15 significant digits), so −0.3 + 3 × 0.1 shows 0.
  static double rowValue(double start, double step, int i);
  // Fills the table from the function and the range and shows it. False (Error screen,
  // errorMessage() set) for a bad range or too many rows.
  bool build();

  // --- the table
  int rows() const { return rows_; }
  double rowX(int i) const { return xs_[clampRow(i)]; }
  bool rowOk(int i) const { return errs_[clampRow(i)] == CalcError::None; }
  double rowY(int i) const { return ys_[clampRow(i)]; }
  CalcError rowError(int i) const { return errs_[clampRow(i)]; }
  int cursorRow() const { return row_; }
  int cursorCol() const { return col_; }  // 0 = X, 1 = F(X)
  int top() const { return top_; }        // first visible row

  // --- screens
  Screen screen() const { return screen_; }
  const std::vector<Tok>& entry() const { return entry_; }  // typed on Function / Start / End / Step
  const char* errorMessage() const { return errorMsg_; }    // on the Error screen

 private:
  int clampRow(int i) const { return i < 0 ? 0 : (i >= kMaxRows ? kMaxRows - 1 : i); }
  Vars& vars() const;
  void keyFunction(DKey k, bool shift, bool alpha);
  void keyRange(DKey k, bool shift, bool alpha);
  void keyTable(DKey k);
  void showError(const char* msg, Screen back);
  void moveTo(int row);

  Screen screen_ = Screen::Function;
  Screen errorBack_ = Screen::Function;
  std::vector<Tok> func_;
  std::vector<Tok> entry_;  // Start / End / Step input
  double start_ = 1, end_ = 5, step_ = 1;
  int rows_ = 0;
  double xs_[kMaxRows] = {}, ys_[kMaxRows] = {};
  CalcError errs_[kMaxRows] = {};
  int row_ = 0, col_ = 0, top_ = 0;
  const char* errorMsg_ = "";
  bool exit_ = false;

  Vars* vars_ = nullptr;
  mutable Vars ownVars_;
  AngleUnit unit_ = AngleUnit::Deg;
  NormMode norm_ = NormMode::Norm1;
  double (*random_)() = nullptr;
};

}  // namespace calc
