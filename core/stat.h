// STAT mode (MODE 2), like the Casio fx-115ES / fx-991ES in linear display.
//
// Self-contained: a key-driven state machine and a render for the 125x61 mono screen
// (rows 1-6; row 0 is the host's status bar). Statistics and regressions are computed
// here; expressions are evaluated with calc_engine (calc::evaluate / formatResult).
// The v15 colour LCD shows calculator modes in the same classic mono look, but its UI
// can also draw from the public state below (screen(), menuItems(), editor cells...).
//
// Screens and keys (DKey; the host handles the SHIFT / ALPHA keys and passes them as flags):
//   TypeSelect  1:1-VAR 2:A+BX 3:_+CX² 4:ln X 5:e^X 6:A·B^X 7:A·X^B 8:1/X. A digit picks
//               the type, clears the data (as on the Casio) and opens the editor. AC goes
//               back where SHIFT 1 > Type came from; on the first screen it asks the host
//               to leave the mode (wantsExit()).
//   Editor      columns X [Y if paired] [FREQ if setFrequency(true)], up to maxRows() rows
//               (Casio: 80 X only, 40 X+FREQ, 40 X+Y, 26 X+Y+FREQ). Arrows move. A value key
//               starts typing an expression for the cell (shown on the bottom line); =
//               stores its value (calc_engine) and moves down; DEL backspaces while typing,
//               else deletes the row; AC cancels typing, else goes to the stat calculation
//               screen. The bottom line shows the selected cell's value in full.
//               SHIFT 1: 1:Type 2:Data 3:Edit 4:Sum 5:Var 6:Reg 7:MinMax (1-VAR: no Reg,
//               6:MinMax). Edit: 1:Ins inserts a row at the cursor, 2:Del-A deletes all data.
//   Calc        the stat calculation screen: a COMP-style line where statistics are
//               values (x̄, σx, A, r...) and x̂ / ŷ are postfix functions ("3ŷ"). = shows
//               the result (big digits, right-aligned) and updates Ans / PreAns; S⇔D swaps
//               fraction / decimal; DEL or ◀ ▶ edit again; AC clears. SHIFT 1:
//               1:Type 2:Data 3:Sum 4:Var 5:Reg 6:MinMax (1-VAR: no Reg, 5:MinMax).
//   Menu        the STAT menu or a sub-menu (Sum / Var / Reg / MinMax / Edit): a digit picks,
//               AC closes it. Picking a statistic (also from the editor) inserts it into the
//               Calc line and shows the Calc screen.
//   Error       "Math ERROR" / "Syntax ERROR"...; AC clears what caused it, ◀ ▶ go back
//               to it unchanged.
//
// How statistics get into an expression (the "STAT value token" API): the Calc line is a
// vector<StatItem>; an item is either a calculator token or a statistic. To evaluate,
// expand() replaces every statistic with its value written as number tokens
// "(" [-] digits [E [-] exp] ")" (numberTokens(): the shortest form that reads back as
// the same double), and every x̂ / x̂1 / x̂2 / ŷ with the estimate for the operand before
// it; then calc::evaluate() runs on plain tokens. A host that keeps its own COMP-style
// line can do the same: value(v, d) + numberTokens(d, toks) inserts a statistic as a number.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "calc_engine.h"
#include "framebuffer.h"
#include "keys.h"  // calc::DKey

namespace calc {

// ----------------------------------------------------------------------------- shared helpers
// (also used by table.cpp)

// The token a key types in STAT / TABLE (the COMP key map: digits, operators, functions,
// SHIFT functions, ALPHA variables; ALPHA ) = X). False for keys that type nothing.
bool modeKeyToken(DKey k, bool shift, bool alpha, Tok& t);

// Appends v as number tokens "(" [Neg] mantissa [Exp10 [Neg] exp] ")", the shortest form
// that reads back as exactly v (0 is just "0"). False if v is not finite or |v| >= 1E100
// (the Casio's range): nothing is appended.
bool numberTokens(double v, std::vector<Tok>& out);

// A value squeezed into `width` characters for a table / editor cell, Casio style:
// as many significant digits as fit ("0.3333", "-1.2E-05" -> "-1.2E-5", "123456").
std::string cellText(double v, int width);

// Draws UTF-8 text starting at (col, row); a combining macron (U+0304) or circumflex
// (U+0302) after a character is drawn as a bar / hat over it (x̄, ȳ, x̂, ŷ: the font has
// no such glyphs). Returns the number of character cells used.
int drawMarkedText(Framebuffer& fb, int col, int row, const std::string& utf8);
// Character cells the text takes (combining marks take none).
int markedLength(const std::string& utf8);
// An input line: `text` wrapped over `rows` rows from `row`, showing the end if it is
// too long, with the "_" cursor after the last character when `cursor` is set.
void drawEntry(Framebuffer& fb, int row, int rows, const std::string& text, bool cursor);
// The Casio error screen: message / "[AC]  :Cancel" / "[◀][▶]:Goto".
void drawErrorScreen(Framebuffer& fb, const char* message);

// ----------------------------------------------------------------------------- types and values

enum class StatType : uint8_t {
  OneVar,   // 1-VAR
  Linear,   // A+BX       y = A + Bx
  Quad,     // _+CX²      y = A + Bx + Cx²
  Log,      // ln X       y = A + B·ln x
  Exp,      // e^X        y = A·e^(Bx)
  ABExp,    // A·B^X      y = A·B^x
  Power,    // A·X^B      y = A·x^B
  Inverse,  // 1/X        y = A + B/x
  Count
};
const char* statTypeName(StatType t);  // "1-VAR", "A+BX", "_+CX²", "ln X", "e^X", "A·B^X", "A·X^B", "1/X"

enum class StatVar : uint8_t {
  None,  // not a statistic (a plain calculator token)
  // Sum
  SumX2, SumX, SumY2, SumY, SumXY, SumX3, SumX2Y, SumX4,
  // Var
  N, MeanX, SigmaX, SX, MeanY, SigmaY, SY,
  // Reg
  A, B, C, R,
  XHat, XHat1, XHat2, YHat,  // postfix: "value x̂"
  // MinMax
  MinX, MaxX, MinY, MaxY,
  Count
};
// How the statistic is drawn, UTF-8 with combining marks for the v15 UI: "Σx²", "x̄"
// (x + U+0304), "σx", "sx", "x̂1" (x + U+0302 + "1"), "minX". Draw with drawMarkedText().
const char* statVarText(StatVar v);
bool statVarIsPostfix(StatVar v);  // x̂, x̂1, x̂2, ŷ

// One item of the stat calculation line: a calculator token (var == None) or a statistic.
struct StatItem {
  Tok tok = Tok::Count;
  StatVar var = StatVar::None;
  static StatItem token(Tok t) { return StatItem{t, StatVar::None}; }
  static StatItem stat(StatVar v) { return StatItem{Tok::Count, v}; }
};

// ----------------------------------------------------------------------------- the mode

class StatApp {
 public:
  enum class Screen : uint8_t { TypeSelect, Editor, Calc, Menu, Error };
  enum class Menu : uint8_t { Stat, Edit, Sum, Var, Reg, MinMax };
  enum class Column : uint8_t { X, Y, Freq };

  static constexpr int kMaxRows = 80;    // the largest Casio limit (1-VAR, no FREQ)
  static constexpr int kEditorRows = 4;  // data rows visible on the mono screen (rows 2-5)

  // One menu entry (for the v15 UI and the render); shown as "<index+1>:<text>".
  struct MenuItem {
    std::string text;             // "Sum", "Σx²", "Ins"...
    StatVar var = StatVar::None;  // the statistic it inserts (Sum / Var / Reg / MinMax menus)
    uint8_t action = 0;           // internal: what a STAT / Edit menu entry does
  };

  StatApp() { reset(); }

  // Fresh start (what MODE 2 does): the type selection screen, no data, empty line.
  // Keeps setEnv() and setFrequency().
  void reset();
  // The calculator's variables (A-F, X, Y, M, Ans: read by expressions; = updates Ans and
  // PreAns), angle unit, Norm mode and Ran# source. Call on entering the mode and whenever
  // SETUP changes. Without it the app uses its own variables, Deg, Norm 1.
  void setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)());
  AngleUnit angleUnit() const { return unit_; }  // as setEnv() last set them
  NormMode normMode() const { return norm_; }
  // SETUP "Frequency" (Stat) on/off, default off. Changing it clears the data (the row
  // limits change), as the Casio does.
  void setFrequency(bool on);
  bool frequency() const { return freqOn_; }

  // Returns true if the key changed anything (redraw).
  bool onKey(DKey k, bool shift, bool alpha);
  // Draws rows 1-6 (y 8-60) and sets the Up / Down icons; row 0 is the host's. Doesn't clear.
  void render(Framebuffer& fb) const;
  // AC on the first type selection: the host should leave STAT (back to COMP).
  bool wantsExit() const { return exit_; }
  void resume() { exit_ = false; }

  // --- data (the STAT editor)
  StatType type() const { return type_; }
  bool paired() const { return type_ != StatType::OneVar; }
  void setType(StatType t);  // clears the data and opens the editor at row 1
  int columns() const;       // 1-3
  Column column(int c) const;
  int rows() const { return rows_; }
  int maxRows() const;       // 80 / 40 / 40 / 26
  double x(int i) const { return x_[clampRow(i)]; }
  double y(int i) const { return y_[clampRow(i)]; }
  double freq(int i) const { return freqOn_ ? f_[clampRow(i)] : 1.0; }
  double cell(int row, Column c) const;
  // Adds a row (y = 0, FREQ = 1 by default); false when full.
  bool addRow(double x, double y = 0, double f = 1);
  // Sets a cell; row == rows() adds a row. False if out of range or a FREQ below 0.
  bool setCell(int row, Column c, double v);
  bool insertRow(int row);  // a 0 / 0 / 1 row before `row`; false when full
  void deleteRow(int row);
  void clearData();

  // --- editor state
  int curRow() const { return row_; }     // 0..rows() (rows() = the empty line for a new row)
  int curCol() const { return col_; }     // 0..columns()-1
  int editorTop() const { return top_; }  // first visible row
  bool typing() const { return typing_; }
  const std::vector<Tok>& entry() const { return entry_; }  // what is being typed in a cell

  // --- statistics (Casio formulas; regressions by least squares on transformed data)
  // A statistic that needs no operand. False (err = Math) when it doesn't exist here
  // (no data, n-1 = 0, a regression with no spread, ln of a value <= 0, not this type...).
  bool value(StatVar v, double& out, CalcError* err = nullptr) const;
  // x̂ (x for a y), x̂1 / x̂2 (quadratic, x̂1 = the "+√" root), ŷ (y for an x).
  bool estimate(StatVar v, double arg, double& out, CalcError* err = nullptr) const;
  // Is the statistic in this type's menus? (y sums / y stats / Reg need paired data;
  // C, x̂1, x̂2 only quadratic; r and x̂ not quadratic.)
  bool available(StatVar v) const;

  // --- the stat calculation line
  const std::vector<StatItem>& expression() const { return expr_; }
  void setExpression(const std::vector<StatItem>& e);  // and shows the Calc screen, editing
  // Expands statistics to number tokens (see the top of this file). False with err set.
  bool expand(const std::vector<StatItem>& e, std::vector<Tok>& out, CalcError& err) const;
  // expand() + calc::evaluate() on a copy of the variables (nothing changes).
  EvalResult evaluate(const std::vector<StatItem>& e) const;
  std::string expressionText() const;  // the line as drawn (UTF-8, combining marks)
  bool showingResult() const { return showing_; }
  const Num& result() const { return result_; }
  std::string resultText() const;      // formatResult(): a fraction unless S⇔D
  CalcError error() const { return error_; }

  // --- screens and menus
  Screen screen() const { return screen_; }
  Menu menu() const { return menu_; }
  std::vector<MenuItem> menuItems() const;  // of the open menu (or the one menu() names)

 private:
  struct Fit;  // a regression's coefficients

  int clampRow(int i) const { return i < 0 ? 0 : (i >= kMaxRows ? kMaxRows - 1 : i); }
  Vars& vars() const;
  using RandomFn = double (*)();
  RandomFn rnd() const;  // setEnv()'s, or std::rand()-based
  void openMenu(Menu m);
  void pickMenu(int n);
  void keyTypeSelect(DKey k);
  void keyEditor(DKey k, bool shift, bool alpha);
  void keyCalc(DKey k, bool shift, bool alpha);
  void commitEntry();
  void moveTo(int row, int col);
  void insertStat(StatVar v);
  void runCalc();
  void showError(CalcError e, Screen back);
  bool fit(Fit& out, CalcError& err) const;

  void drawEditor(Framebuffer& fb) const;
  void drawCalc(Framebuffer& fb) const;

  StatType type_ = StatType::OneVar;
  bool freqOn_ = false;
  int rows_ = 0;
  double x_[kMaxRows] = {}, y_[kMaxRows] = {}, f_[kMaxRows] = {};

  Screen screen_ = Screen::TypeSelect;
  Screen menuFrom_ = Screen::Calc;  // where a menu / TypeSelect returns to (Editor / Calc)
  Screen errorBack_ = Screen::Calc;
  bool typeChosen_ = false;         // a type was picked since reset() (AC on TypeSelect goes back)
  Menu menu_ = Menu::Stat;
  int row_ = 0, col_ = 0, top_ = 0;
  bool typing_ = false;
  std::vector<Tok> entry_;
  std::vector<StatItem> expr_;
  bool showing_ = false, showFraction_ = true;
  Num result_;
  CalcError error_ = CalcError::None;
  bool exit_ = false;

  Vars* vars_ = nullptr;  // the host's; nullptr = ownVars_
  mutable Vars ownVars_;
  AngleUnit unit_ = AngleUnit::Deg;
  NormMode norm_ = NormMode::Norm1;
  double (*random_)() = nullptr;
};

}  // namespace calc
