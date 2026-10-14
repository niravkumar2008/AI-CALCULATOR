// MATRIX (MODE 8) and VECTOR (MODE 9), like the Casio fx-115ES PLUS matrix and vector modes
// (fx-115ES PLUS / fx-991ES PLUS C User's Guide, "Matrix Calculations (MATRIX)" and "Vector
// Calculations (VECTOR)"; casioeducation.com, support.casio.com).
//
// The calculation screen is COMP's (View::Calc): the Device evaluates its expression with
// evaluateMatVec() instead of evaluate() in these modes. A small parser with the real one's
// structure (same priorities, implicit ×, auto-closed brackets, the shared kMaxNesting budget, an
// EvalRun) over the same tokens; every scalar step goes back to the real engine (calc::realOp), so
// matrix elements stay exact fractions (the inverse of [[1,2],[3,4]] is [[−2,1],[3⌟2,−1⌟2]]).
//
// As the guide says:
//   - MATRIX: MatA MatB MatC up to 3 rows × 3 columns, and MatAns (a matrix result is shown on
//     the MatAns screen and assigned to MatAns). + − × (sizes must agree), scalar ×, det( (square),
//     Trn(, x⁻¹ (inverse; a singular matrix is a Math ERROR), x² x³ (square), Abs( (each element).
//     The ^ key is not for matrices ("use x² ... x³"; "use the x⁻¹ key").
//   - VECTOR: VctA VctB VctC of 2 or 3 dimensions, and VctAns. + −, scalar ×, × of two 3D vectors
//     (the cross product), the dot product (SHIFT 5 Dot, "·"), Abs( (the magnitude). The angle
//     between two vectors is the guide's cos⁻¹((A·B)÷(Abs(A)Abs(B))): scalar maths, no own key.
//   - SHIFT 4 (MATRIX): 1:Dim 2:Data 3:MatA 4:MatB 5:MatC 6:MatAns 7:det 8:Trn;
//     SHIFT 5 (VECTOR): 1:Dim 2:Data 3:VctA 4:VctB 5:VctC 6:VctAns 7:Dot.
//     Dim: pick the variable, then its size (m×n? 1:3×3 2:3×2 3:3×1 4:2×3 5:2×2 6:2×1 ▼ 1:1×3
//     2:1×2 3:1×1 / m? 1:3 2:2), then the editor (all elements 0). Data: the editor on what is there.
//   - On the MatAns / VctAns screen + − × ÷ x⁻¹ x² x³ (VctAns: + − × ÷) go to the calculation
//     screen as "MatAns" and that key. In an editor, STO then A / B / C (the (−) °'" hyp keys)
//     copies what it shows to MatA / MatB / MatC.
//   - Dimension ERROR (shown "Dim ERROR"): sizes that don't allow the calculation, or a variable
//     used before Dim gave it a size.
// Choices where the guide says nothing (PORT_NOTES 2026-10-12):
//   - a scalar added to (or subtracted from) a matrix / vector, a matrix in a scalar-only place (a
//     function other than Abs / det / Trn, ^, ⌟, nPr, x!, %, a divisor): Syntax ERROR; M ÷ k is each
//     element ÷ k (the MatAns screen takes ÷). Two 2D vectors × : Dim ERROR (no cross product).
//   - the matrix stack holds kMvSlots matrices / vectors at once: one more is a Stack ERROR.
//   - entering MODE 8 / 9 clears the variables (as MODE 2 and MODE 5 clear STAT's and EQN's data
//     here) and opens Dim's "Matrix?" / "Vector?" list, as the Casio's first screen of the mode.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "calc_engine.h"
#include "framebuffer.h"
#include "keys.h"  // calc::DKey

namespace calc {

// A matrix of up to 3 × 3, or a vector (one row of 2 or 3). rows == 0: no size set (Dim) yet.
struct Mat {
  static constexpr int kMax = 3;
  uint8_t rows = 0, cols = 0;
  Num c[kMax * kMax];
  bool defined() const { return rows > 0; }
  Num& at(int r, int k) { return c[r * kMax + k]; }
  const Num& at(int r, int k) const { return c[r * kMax + k]; }
  static Mat zero(int rows, int cols);  // every element an exact 0
};

enum class MvKind : uint8_t { Scalar, Matrix, Vector };
constexpr int kMvSlots = 8;  // the matrix / vector stack

struct MvResult {
  CalcError error = CalcError::None;
  int errorPos = 0;  // token index for Goto, as EvalResult
  MvKind kind = MvKind::Scalar;
  Num scalar;  // kind Scalar
  Mat m;       // kind Matrix / Vector
};

// Evaluates a MATRIX (vectors false) or VECTOR expression. mv: MatA MatB MatC MatAns (or VctA
// ... VctAns), four of them. vars: the memories and Ans (read only, as evaluate()).
MvResult evaluateMatVec(const std::vector<Tok>& toks, Vars& vars, const Mat* mv, bool vectors, AngleUnit unit,
                        double (*random)());

// The maths on their own (tests): false with err (Dim / Math) on an error.
bool matDet(const Mat& a, Num& out, CalcError& err);
bool matInverse(const Mat& a, Mat& out, CalcError& err);

// An element in `width` characters: its fraction if that fits, else cellText()'s digits.
std::string mvCellText(const Num& v, int width);

// The mode's own screens (View::MatVec): Dim's variable and size lists, the editors (also the
// MatAns / VctAns screen, read only) and the SHIFT 4 / SHIFT 5 menu. Rows 1-6, like EQN; the
// calculation screen itself is the Device's View::Calc.
class MatVecApp {
 public:
  enum class Screen : uint8_t { Select, Dim, Editor, Menu, Error };
  // What the host does after a key: nothing, back to the calculation screen, or that and
  // insert(token) (a menu's variable / det( / Trn( / ·, or the MatAns screen's operator).
  enum class Action : uint8_t { None, ToCalc, Insert };

  MatVecApp() { reset(false); }
  // Fresh (MODE 8 / 9): all four variables cleared, Dim's variable list.
  void reset(bool vectors);
  void setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)());
  bool vectors() const { return vectors_; }

  void onKey(DKey k, bool shift, bool alpha);
  void render(Framebuffer& fb) const;  // rows 1-6, the ▲▼ icons; doesn't clear
  Action takeAction(Tok& t);           // once per action

  void openMenu();          // SHIFT 4 / SHIFT 5 from the calculation screen
  void showAns(const Mat& m);  // a matrix / vector result: MatAns = m, its screen

  static const char* varName(bool vectors, int i);  // "MatA" ... "MatAns" / "VctA" ... "VctAns"
  const Mat* vars() const { return var_; }
  const Mat& var(int i) const { return var_[i < 0 ? 0 : i > 3 ? 3 : i]; }
  bool setVar(int i, const Mat& m);  // false (nothing changes) for a size the mode can't hold

  Screen screen() const { return screen_; }
  int editing() const { return ed_; }  // the variable on the Editor screen (3: the Ans screen)
  int curRow() const { return row_; }
  int curCol() const { return col_; }
  int dimPage() const { return dimPage_; }
  bool typing() const { return typing_; }
  bool storing() const { return sto_; }
  const std::vector<Tok>& entry() const { return entry_; }

 private:
  Vars& calcVars() const;
  void keyEditor(DKey k, bool shift, bool alpha);
  void commitEntry();
  void openEditor(int var);
  void drawEditor(Framebuffer& fb) const;

  bool vectors_ = false;
  Mat var_[4];
  Screen screen_ = Screen::Select;
  Screen menuBack_ = Screen::Select;  // the Menu's AC: an app screen, or (Select) the calculation screen
  bool menuFromCalc_ = true;
  bool forDim_ = true;  // Select: for Dim (a new size), or Data (edit)
  int pick_ = 0;        // Dim: the variable chosen on Select
  int dimPage_ = 0;
  int ed_ = 0, row_ = 0, col_ = 0;
  bool typing_ = false, sto_ = false;
  std::vector<Tok> entry_;
  const char* message_ = "";
  Action action_ = Action::None;
  Tok actionTok_ = Tok::Count;

  Vars* vars_ = nullptr;
  mutable Vars ownVars_;
  AngleUnit unit_ = AngleUnit::Deg;
  NormMode norm_ = NormMode::Norm1;
  double (*random_)() = nullptr;
};

}  // namespace calc
