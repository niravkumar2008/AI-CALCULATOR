#include "stat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "font.h"
#include "text.h"

namespace calc {

// ============================================================================= shared helpers

namespace {

constexpr int kCols = Framebuffer::kCols;
constexpr uint32_t kMacron = 0x0304;
constexpr uint32_t kCircumflex = 0x0302;

int digitOf(DKey k) {
  switch (k) {
    case DKey::D0: return 0;
    case DKey::D1: return 1;
    case DKey::D2: return 2;
    case DKey::D3: return 3;
    case DKey::D4: return 4;
    case DKey::D5: return 5;
    case DKey::D6: return 6;
    case DKey::D7: return 7;
    case DKey::D8: return 8;
    case DKey::D9: return 9;
    default: return -1;
  }
}

// ALPHA + key: the variable printed above it (as in COMP).
bool alphaVar(DKey k, Tok& t) {
  switch (k) {
    case DKey::Neg: t = Tok::VarA; return true;
    case DKey::Dms: t = Tok::VarB; return true;
    case DKey::Hyp: t = Tok::VarC; return true;
    case DKey::Sin: t = Tok::VarD; return true;
    case DKey::Cos: t = Tok::VarE; return true;
    case DKey::Tan: t = Tok::VarF; return true;
    case DKey::Close: t = Tok::VarX; return true;
    case DKey::SD: t = Tok::VarY; return true;
    case DKey::MPlus: t = Tok::VarM; return true;
    default: return false;
  }
}

bool isDigitTok(Tok t) { return t <= Tok::D9; }
bool isValueTok(Tok t) { return t >= Tok::Pi && t <= Tok::VarM; }
bool isPostfixTok(Tok t) {
  return t == Tok::Sq || t == Tok::Cube || t == Tok::Inv || t == Tok::Fact || t == Tok::Pct;
}

// After a result, these keys carry on from it: "Ans+" ... (as in COMP)
bool continuesAns(Tok t) {
  switch (t) {
    case Tok::Add: case Tok::Sub: case Tok::Mul: case Tok::Div: case Tok::Pow: case Tok::XRoot:
    case Tok::Sq: case Tok::Cube: case Tok::Inv: case Tok::Fact: case Tok::Pct:
    case Tok::NPr: case Tok::NCr: case Tok::Frac:
      return true;
    default:
      return false;
  }
}

// Index of the first token of the operand that ends at `end` (for a postfix x̂ / ŷ):
// a number, a value (π, Ans, A...), a bracket group (with its function), a power, each
// with any x² x³ x⁻¹ ! % after it. -1 if there is none.
int operandStart(const std::vector<Tok>& t, int end) {
  int i = end;
  while (i >= 0 && isPostfixTok(t[static_cast<size_t>(i)])) --i;
  if (i < 0) return -1;
  const Tok k = t[static_cast<size_t>(i)];
  if (k == Tok::Close) {
    int depth = 0;
    for (int j = i; j >= 0; --j) {
      const Tok u = t[static_cast<size_t>(j)];
      if (u == Tok::Close) {
        ++depth;
      } else if (tokOpensBracket(u) && --depth == 0) {
        if (u == Tok::Pow || u == Tok::XRoot) return operandStart(t, j - 1);  // 2^(3): from the 2
        return j;
      }
    }
    return -1;
  }
  if (isDigitTok(k) || k == Tok::Dot) {
    int j = i;
    while (j > 0) {
      const Tok p = t[static_cast<size_t>(j - 1)];
      if (isDigitTok(p) || p == Tok::Dot || p == Tok::Exp10) --j;
      else if (p == Tok::Neg && j >= 2 && t[static_cast<size_t>(j - 2)] == Tok::Exp10) --j;
      else break;
    }
    return j;
  }
  if (isValueTok(k)) return i;
  return -1;
}

void putDeco(Framebuffer& fb, int cell, int row, uint32_t mark) {
  const int x = cell * 5, y = row * 8;
  if (cell < 0 || cell >= kCols) return;
  if (mark == kMacron) {
    fb.fillRect(x, y, 4, 1, true);
  } else {  // circumflex: a small hat over rows 0-1 (lower-case x / y start at row 2)
    fb.set(x, y + 1, true);
    fb.set(x + 1, y, true);
    fb.set(x + 2, y, true);
    fb.set(x + 3, y + 1, true);
  }
}

struct Cell {
  uint32_t cp;
  uint32_t mark;  // 0, kMacron or kCircumflex
};

std::vector<Cell> cellsOf(const std::string& utf8) {
  std::vector<Cell> cells;
  for (uint32_t cp : decodeUtf8(utf8)) {
    if (cp == kMacron || cp == kCircumflex) {
      if (!cells.empty()) cells.back().mark = cp;
    } else {
      cells.push_back(Cell{cp, 0});
    }
  }
  return cells;
}

void drawCell(Framebuffer& fb, int col, int row, const Cell& c) {
  if (col < 0 || col >= kCols) return;
  fb.drawText(col, row, encodeUtf8(std::vector<uint32_t>{c.cp}));
  if (c.mark) putDeco(fb, col, row, c.mark);
}

void rightText(Framebuffer& fb, int row, const std::string& s) {
  const int n = markedLength(s);
  drawMarkedText(fb, n >= kCols ? 0 : kCols - n, row, s);
}

}  // namespace

bool modeKeyToken(DKey k, bool shift, bool alpha, Tok& t) {
  if (alpha) {
    if (alphaVar(k, t)) return true;
    if (k == DKey::Exp10) { t = Tok::E; return true; }
    if (k == DKey::Dot) { t = Tok::RanInt; return true; }
    return false;
  }
  const int d = digitOf(k);
  if (d >= 0) {
    if (!shift) { t = static_cast<Tok>(d); return true; }
    if (d == 0) { t = Tok::Rnd; return true; }  // SHIFT 0 = Rnd(
    return false;                               // SHIFT 1 = STAT (a command), SHIFT 9 = CLR
  }
  struct M { DKey k; Tok plain, shifted; };
  static const M kMap[] = {
      {DKey::LogAB, Tok::Log, Tok::Count},
      {DKey::Inv, Tok::Inv, Tok::Fact},
      {DKey::Frac, Tok::Frac, Tok::Count},
      {DKey::Sqrt, Tok::Sqrt, Tok::Cbrt},   {DKey::Sq, Tok::Sq, Tok::Cube},
      {DKey::Pow, Tok::Pow, Tok::XRoot},    {DKey::Log, Tok::Log, Tok::Pow10},
      {DKey::Ln, Tok::Ln, Tok::Exp},        {DKey::Neg, Tok::Neg, Tok::Count},
      {DKey::Hyp, Tok::Count, Tok::Abs},    {DKey::Sin, Tok::Sin, Tok::Asin},
      {DKey::Cos, Tok::Cos, Tok::Acos},     {DKey::Tan, Tok::Tan, Tok::Atan},
      {DKey::Open, Tok::Open, Tok::Pct},    {DKey::Close, Tok::Close, Tok::Comma},
      {DKey::Mul, Tok::Mul, Tok::NPr},      {DKey::Div, Tok::Div, Tok::NCr},
      {DKey::Add, Tok::Add, Tok::Count},    {DKey::Sub, Tok::Sub, Tok::Count},
      {DKey::Dot, Tok::Dot, Tok::Ran},      {DKey::Exp10, Tok::Exp10, Tok::Pi},
      {DKey::Ans, Tok::Ans, Tok::Count},
  };
  for (const M& m : kMap)
    if (m.k == k) {
      t = shift ? m.shifted : m.plain;
      return t != Tok::Count;
    }
  return false;
}

bool numberTokens(double v, std::vector<Tok>& out) {
  if (!std::isfinite(v) || std::fabs(v) >= 1e100) return false;
  if (v == 0 || std::fabs(v) < 1e-99) {  // below the Casio's range: 0
    out.push_back(Tok::D0);
    return true;
  }
  char b[64];
  for (int p = 1; p <= 17; ++p) {
    std::snprintf(b, sizeof b, "%.*e", p - 1, v);
    if (std::strtod(b, nullptr) == v) break;
  }
  // b = [-]d[.ddd]e[+-]XX
  const char* s = b;
  out.push_back(Tok::Open);
  if (*s == '-') {
    out.push_back(Tok::Neg);
    ++s;
  }
  std::string mant;
  while (*s && *s != 'e') {
    if (*s != '.') mant += *s;
    ++s;
  }
  while (mant.size() > 1 && mant.back() == '0') mant.pop_back();
  const int ex = *s == 'e' ? std::atoi(s + 1) : 0;
  out.push_back(static_cast<Tok>(mant[0] - '0'));
  if (mant.size() > 1) {
    out.push_back(Tok::Dot);
    for (size_t i = 1; i < mant.size(); ++i) out.push_back(static_cast<Tok>(mant[i] - '0'));
  }
  if (ex != 0) {
    out.push_back(Tok::Exp10);
    if (ex < 0) out.push_back(Tok::Neg);
    const int a = ex < 0 ? -ex : ex;
    if (a >= 10) out.push_back(static_cast<Tok>(a / 10));
    out.push_back(static_cast<Tok>(a % 10));
  }
  out.push_back(Tok::Close);
  return true;
}

std::string cellText(double v, int width) {
  if (v == 0) return "0";
  if (!std::isfinite(v)) return "ERROR";
  // The most significant digits that fit, as %g or as %e: %g alone writes −0.000152 as
  // "-0.000152" down to one digit ("-0.0002", 7 wide), where "-1.5E-4" fits in 7 and "-2E-4" in 6.
  for (int p = width; p >= 1; --p) {
    for (int form = 0; form < 2; ++form) {
      char b[48];
      if (form == 0) std::snprintf(b, sizeof b, "%.*g", p, v);
      else std::snprintf(b, sizeof b, "%.*e", p - 1, v);
      // "1.2e-05" -> "1.2E-5", "1e+06" -> "1E6"; %e's trailing zeros go ("1.50E-4" -> "1.5E-4")
      std::string s;
      for (const char* c = b; *c; ++c) {
        if (*c == 'e') {
          if (s.find('.') != std::string::npos) {
            while (s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
          }
          s += 'E';
          ++c;
          if (*c == '-') s += '-';
          if (*c == '+' || *c == '-') ++c;
          while (*c == '0' && c[1]) ++c;
          s += c;
          break;
        }
        s += *c;
      }
      if (static_cast<int>(s.size()) <= width) return s;
    }
  }
  return std::string(static_cast<size_t>(width > 0 ? width : 0), '#');
}

int drawMarkedText(Framebuffer& fb, int col, int row, const std::string& utf8) {
  const std::vector<Cell> cells = cellsOf(utf8);
  for (size_t i = 0; i < cells.size(); ++i) drawCell(fb, col + static_cast<int>(i), row, cells[i]);
  return static_cast<int>(cells.size());
}

int markedLength(const std::string& utf8) { return static_cast<int>(cellsOf(utf8).size()); }

void drawEntry(Framebuffer& fb, int row, int rows, const std::string& text, bool cursor) {
  const std::vector<Cell> cells = cellsOf(text);
  const int n = static_cast<int>(cells.size());
  const int used = n + (cursor ? 1 : 0);
  const int lastLine = used > 0 ? (used - 1) / kCols : 0;
  int firstLine = lastLine - (rows - 1);
  if (firstLine < 0) firstLine = 0;
  const int first = firstLine * kCols;
  for (int i = first; i < n && i < first + rows * kCols; ++i)
    drawCell(fb, (i - first) % kCols, row + (i - first) / kCols, cells[static_cast<size_t>(i)]);
  if (cursor) {
    const int c = n - first;
    if (c >= 0 && c < rows * kCols) fb.fillRect((c % kCols) * 5, (row + c / kCols) * 8 + 7, 4, 1, true);
  }
}

void drawErrorScreen(Framebuffer& fb, const char* message) {
  fb.drawText(2, 1, message);
  fb.drawText(2, 3, "[AC]  :Cancel");
  fb.drawText(2, 4, "[\xE2\x97\x80][\xE2\x96\xB6]:Goto");
}

// ============================================================================= names

const char* statTypeName(StatType t) {
  switch (t) {
    case StatType::OneVar: return "1-VAR";
    case StatType::Linear: return "A+BX";
    case StatType::Quad: return "_+CX\xC2\xB2";
    case StatType::Log: return "ln X";
    case StatType::Exp: return "e^X";
    case StatType::ABExp: return "A\xC2\xB7" "B^X";
    case StatType::Power: return "A\xC2\xB7X^B";
    case StatType::Inverse: return "1/X";
    default: return "?";
  }
}

const char* statVarText(StatVar v) {
  switch (v) {
    case StatVar::SumX2: return "\xCE\xA3x\xC2\xB2";
    case StatVar::SumX: return "\xCE\xA3x";
    case StatVar::SumY2: return "\xCE\xA3y\xC2\xB2";
    case StatVar::SumY: return "\xCE\xA3y";
    case StatVar::SumXY: return "\xCE\xA3xy";
    case StatVar::SumX3: return "\xCE\xA3x\xC2\xB3";
    case StatVar::SumX2Y: return "\xCE\xA3x\xC2\xB2y";
    case StatVar::SumX4: return "\xCE\xA3x\xE2\x81\xB4";
    case StatVar::N: return "n";
    case StatVar::MeanX: return "x\xCC\x84";
    case StatVar::SigmaX: return "\xCF\x83x";
    case StatVar::SX: return "sx";
    case StatVar::MeanY: return "y\xCC\x84";
    case StatVar::SigmaY: return "\xCF\x83y";
    case StatVar::SY: return "sy";
    case StatVar::A: return "A";
    case StatVar::B: return "B";
    case StatVar::C: return "C";
    case StatVar::R: return "r";
    case StatVar::XHat: return "x\xCC\x82";
    case StatVar::XHat1: return "x\xCC\x82" "1";
    case StatVar::XHat2: return "x\xCC\x82" "2";
    case StatVar::YHat: return "y\xCC\x82";
    case StatVar::MinX: return "minX";
    case StatVar::MaxX: return "maxX";
    case StatVar::MinY: return "minY";
    case StatVar::MaxY: return "maxY";
    default: return "";
  }
}

bool statVarIsPostfix(StatVar v) {
  return v == StatVar::XHat || v == StatVar::XHat1 || v == StatVar::XHat2 || v == StatVar::YHat;
}

// ============================================================================= data

void StatApp::reset() {
  type_ = StatType::OneVar;
  clearData();
  screen_ = Screen::TypeSelect;
  menuFrom_ = errorBack_ = Screen::Calc;
  typeChosen_ = false;
  menu_ = Menu::Stat;
  expr_.clear();
  showing_ = false;
  showFraction_ = true;
  result_ = Num();
  error_ = CalcError::None;
  exit_ = false;
}

void StatApp::setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)()) {
  vars_ = vars;
  unit_ = unit;
  norm_ = norm;
  random_ = random;
}

Vars& StatApp::vars() const { return vars_ ? *vars_ : ownVars_; }

namespace {
double statFallbackRandom() { return static_cast<double>(std::rand()) / (static_cast<double>(RAND_MAX) + 1.0); }
}  // namespace

StatApp::RandomFn StatApp::rnd() const { return random_ ? random_ : statFallbackRandom; }

void StatApp::setFrequency(bool on) {
  if (on == freqOn_) return;
  freqOn_ = on;
  clearData();
}

void StatApp::setType(StatType t) {
  if (t >= StatType::Count) return;
  type_ = t;
  clearData();
  typeChosen_ = true;
  screen_ = Screen::Editor;
}

int StatApp::columns() const { return 1 + (paired() ? 1 : 0) + (freqOn_ ? 1 : 0); }

StatApp::Column StatApp::column(int c) const {
  if (c <= 0) return Column::X;
  if (c == 1) return paired() ? Column::Y : Column::Freq;
  return Column::Freq;
}

int StatApp::maxRows() const {
  if (!paired()) return freqOn_ ? 40 : 80;
  return freqOn_ ? 26 : 40;
}

double StatApp::cell(int row, Column c) const {
  switch (c) {
    case Column::X: return x(row);
    case Column::Y: return y(row);
    default: return freq(row);
  }
}

bool StatApp::addRow(double xv, double yv, double fv) {
  if (rows_ >= maxRows() || fv < 0) return false;
  x_[rows_] = xv;
  y_[rows_] = yv;
  f_[rows_] = fv;
  ++rows_;
  return true;
}

bool StatApp::setCell(int row, Column c, double v) {
  if (row < 0 || row > rows_) return false;
  if (c == Column::Y && !paired()) return false;
  if (c == Column::Freq && (!freqOn_ || v < 0)) return false;
  if (row == rows_ && !addRow(0, 0, 1)) return false;
  (c == Column::X ? x_ : c == Column::Y ? y_ : f_)[row] = v;
  return true;
}

bool StatApp::insertRow(int row) {
  if (rows_ >= maxRows() || row < 0 || row > rows_) return false;
  for (int i = rows_; i > row; --i) {
    x_[i] = x_[i - 1];
    y_[i] = y_[i - 1];
    f_[i] = f_[i - 1];
  }
  x_[row] = y_[row] = 0;
  f_[row] = 1;
  ++rows_;
  return true;
}

void StatApp::deleteRow(int row) {
  if (row < 0 || row >= rows_) return;
  for (int i = row; i + 1 < rows_; ++i) {
    x_[i] = x_[i + 1];
    y_[i] = y_[i + 1];
    f_[i] = f_[i + 1];
  }
  --rows_;
  moveTo(row_, col_);
}

void StatApp::clearData() {
  rows_ = 0;
  row_ = col_ = top_ = 0;
  typing_ = false;
  entry_.clear();
}

// ============================================================================= statistics

struct StatApp::Fit {
  double a = 0, b = 0, c = 0, r = 0;
  double lnB = 0;  // A·B^X: ln B as fitted (B = e^lnB rounds away B's distance from 1)
  bool hasR = false;
};

namespace {
bool fail(CalcError* err) {
  if (err) *err = CalcError::Math;
  return false;
}
bool inRange(double v) { return std::isfinite(v) && std::fabs(v) < 1e100; }

// Rounding noise of values up to `big` in size, squared and summed over n: a sum of squared
// deviations at or below it is no spread at all (identical values computed differently).
double noiseSS(double n, double big) {
  const double e = 8 * std::numeric_limits<double>::epsilon() * big;
  return n * e * e;
}

// Weighted (w: nullptr = 1 each) sum of squared deviations of u[0..rows) about its mean,
// shifted by the first counted value first (equal values give exactly 0); noise is 0.
double spreadOf(const double* u, const double* w, int rows) {
  double n = 0, u0 = 0, m = 0, big = 0;
  bool any = false;
  for (int i = 0; i < rows; ++i) {
    const double f = w ? w[i] : 1.0;
    if (f == 0) continue;
    if (!any) u0 = u[i];
    any = true;
    n += f;
    m += f * (u[i] - u0);
    big = std::max(big, std::fabs(u[i]));
  }
  if (!(n > 0)) return 0;
  m /= n;
  double ss = 0;
  for (int i = 0; i < rows; ++i) {
    const double f = w ? w[i] : 1.0;
    if (f == 0) continue;
    const double d = (u[i] - u0) - m;
    ss += f * d * d;
  }
  return ss <= noiseSS(n, big) ? 0 : ss;
}
}  // namespace

bool StatApp::available(StatVar v) const {
  switch (v) {
    case StatVar::SumX2: case StatVar::SumX: case StatVar::N: case StatVar::MeanX:
    case StatVar::SigmaX: case StatVar::SX: case StatVar::MinX: case StatVar::MaxX:
      return true;
    case StatVar::SumY2: case StatVar::SumY: case StatVar::SumXY: case StatVar::SumX3:
    case StatVar::SumX2Y: case StatVar::SumX4: case StatVar::MeanY: case StatVar::SigmaY:
    case StatVar::SY: case StatVar::MinY: case StatVar::MaxY: case StatVar::A: case StatVar::B:
    case StatVar::YHat:
      return paired();
    case StatVar::C: case StatVar::XHat1: case StatVar::XHat2:
      return type_ == StatType::Quad;
    case StatVar::R: case StatVar::XHat:
      return paired() && type_ != StatType::Quad;
    default:
      return false;
  }
}

bool StatApp::fit(Fit& out, CalcError& err) const {
  err = CalcError::Math;
  if (!paired()) return false;
  if (type_ == StatType::Quad) {
    // Deviations of x and x² from their means, with x shifted by the first value x0 first
    // (t = x − x0, q = x² − x0² = t(t + 2x0)): equal x give exactly no spread.
    double n = 0, m1 = 0, m2 = 0, my = 0, x0 = 0, big = 0;
    bool any = false;
    for (int i = 0; i < rows_; ++i) {
      const double f = freq(i);
      if (f == 0) continue;
      if (!any) x0 = x_[i];
      any = true;
      const double t = x_[i] - x0;
      n += f;
      m1 += f * t;
      m2 += f * t * (t + 2 * x0);
      my += f * y_[i];
      big = std::max(big, std::fabs(x_[i]));
    }
    if (n <= 0) return false;
    m1 /= n;
    m2 /= n;
    my /= n;
    double sxx = 0, sxx2 = 0, sx2x2 = 0, sxy = 0, sx2y = 0;
    for (int i = 0; i < rows_; ++i) {
      const double f = freq(i);
      if (f == 0) continue;
      const double t = x_[i] - x0, dx = t - m1, dx2 = t * (t + 2 * x0) - m2, dy = y_[i] - my;
      sxx += f * dx * dx;
      sxx2 += f * dx * dx2;
      sx2x2 += f * dx2 * dx2;
      sxy += f * dx * dy;
      sx2y += f * dx2 * dy;
    }
    if (sxx <= noiseSS(n, big)) return false;  // all x equal: no fit (Math ERROR)
    const double det = sxx * sx2x2 - sxx2 * sxx2;
    if (!(det > 1e-12 * sxx * sx2x2) || det == 0) return false;
    out.b = (sxy * sx2x2 - sx2y * sxx2) / det;
    out.c = (sx2y * sxx - sxy * sxx2) / det;
    out.a = my - out.b * (x0 + m1) - out.c * (x0 * x0 + m2);
    out.hasR = false;
  } else {
    // Linear least squares on (u, v): u = x, ln x or 1/x; v = y or ln y.
    const bool lnX = type_ == StatType::Log || type_ == StatType::Power;
    const bool invX = type_ == StatType::Inverse;
    const bool lnY = type_ == StatType::Exp || type_ == StatType::ABExp || type_ == StatType::Power;
    // Means of u − u0 and v − v0 (shifted by the first counted pair: equal values give exactly
    // no spread); spreads at rounding-noise level are none (all x equal: no fit, all y equal:
    // no r), so 0.1, 0.1, 0.1 can't make a regression out of last-bit noise.
    double n = 0, mu = 0, mv = 0, u0 = 0, v0 = 0, bigU = 0, bigV = 0;
    bool any = false;
    std::vector<double> us(static_cast<size_t>(rows_)), vs(static_cast<size_t>(rows_));
    for (int i = 0; i < rows_; ++i) {
      const double f = freq(i);
      double u = x_[i], v = y_[i];
      if (f == 0) continue;
      if (lnX) {
        if (u <= 0) return false;
        u = std::log(u);
      } else if (invX) {
        if (u == 0) return false;
        u = 1 / u;
      }
      if (lnY) {
        if (v <= 0) return false;
        v = std::log(v);
      }
      us[static_cast<size_t>(i)] = u;
      vs[static_cast<size_t>(i)] = v;
      if (!any) {
        u0 = u;
        v0 = v;
      }
      any = true;
      n += f;
      mu += f * (u - u0);
      mv += f * (v - v0);
      bigU = std::max(bigU, std::fabs(u));
      bigV = std::max(bigV, std::fabs(v));
    }
    if (n <= 0) return false;
    mu /= n;
    mv /= n;
    double suu = 0, suv = 0, svv = 0;
    for (int i = 0; i < rows_; ++i) {
      const double f = freq(i);
      if (f == 0) continue;
      const double du = (us[static_cast<size_t>(i)] - u0) - mu, dv = (vs[static_cast<size_t>(i)] - v0) - mv;
      suu += f * du * du;
      suv += f * du * dv;
      svv += f * dv * dv;
    }
    if (!(suu > noiseSS(n, bigU))) return false;
    const double b = suv / suu, a = (v0 + mv) - b * (u0 + mu);
    out.hasR = svv > noiseSS(n, bigV);
    out.lnB = b;
    out.r = out.hasR ? suv / std::sqrt(suu * svv) : 0;
    if (out.r > 1) out.r = 1;
    if (out.r < -1) out.r = -1;
    switch (type_) {
      case StatType::Exp: case StatType::Power:
        out.a = std::exp(a);
        out.b = b;
        break;
      case StatType::ABExp:
        out.a = std::exp(a);
        out.b = std::exp(b);
        break;
      default:
        out.a = a;
        out.b = b;
        break;
    }
  }
  if (!inRange(out.a) || !inRange(out.b) || !inRange(out.c)) return false;
  err = CalcError::None;
  return true;
}

bool StatApp::value(StatVar v, double& out, CalcError* err) const {
  if (err) *err = CalcError::None;
  if (!available(v) || statVarIsPostfix(v)) return fail(err);
  double n = 0, sx = 0, sy = 0, sx2 = 0, sy2 = 0, sxy = 0, sx3 = 0, sx2y = 0, sx4 = 0;
  bool any = false;
  double mnx = 0, mxx = 0, mny = 0, mxy = 0;
  for (int i = 0; i < rows_; ++i) {
    const double f = freq(i), xv = x_[i], yv = paired() ? y_[i] : 0;
    n += f;
    sx += f * xv;
    sy += f * yv;
    sx2 += f * xv * xv;
    sy2 += f * yv * yv;
    sxy += f * xv * yv;
    sx3 += f * xv * xv * xv;
    sx2y += f * xv * xv * yv;
    sx4 += f * xv * xv * xv * xv;
    if (f > 0) {
      if (!any || xv < mnx) mnx = xv;
      if (!any || xv > mxx) mxx = xv;
      if (!any || yv < mny) mny = yv;
      if (!any || yv > mxy) mxy = yv;
      any = true;
    }
  }
  // σ and s from the spread about the mean (two passes: no cancellation), shifted by the first
  // value (identical values give exactly 0: 0.1, 0.1, 0.1 had σx = 1.4E-17), and no more than
  // rounding noise is no spread at all.
  auto spread = [&](bool ofY, double& ss) { ss = spreadOf(ofY ? y_ : x_, freqOn_ ? f_ : nullptr, rows_); };
  double r = 0;
  switch (v) {
    case StatVar::SumX2: r = sx2; break;
    case StatVar::SumX: r = sx; break;
    case StatVar::SumY2: r = sy2; break;
    case StatVar::SumY: r = sy; break;
    case StatVar::SumXY: r = sxy; break;
    case StatVar::SumX3: r = sx3; break;
    case StatVar::SumX2Y: r = sx2y; break;
    case StatVar::SumX4: r = sx4; break;
    case StatVar::N: r = n; break;
    case StatVar::MeanX: case StatVar::MeanY:
      if (n <= 0) return fail(err);
      r = (v == StatVar::MeanX ? sx : sy) / n;
      break;
    case StatVar::SigmaX: case StatVar::SigmaY: case StatVar::SX: case StatVar::SY: {
      const bool sample = v == StatVar::SX || v == StatVar::SY;
      if (n <= 0 || (sample && n <= 1)) return fail(err);
      double ss = 0;
      spread(v == StatVar::SigmaY || v == StatVar::SY, ss);
      r = std::sqrt(ss / (sample ? n - 1 : n));
      break;
    }
    case StatVar::MinX: case StatVar::MaxX: case StatVar::MinY: case StatVar::MaxY:
      if (!any) return fail(err);
      r = v == StatVar::MinX ? mnx : v == StatVar::MaxX ? mxx : v == StatVar::MinY ? mny : mxy;
      break;
    case StatVar::A: case StatVar::B: case StatVar::C: case StatVar::R: {
      Fit ft;
      CalcError e;
      if (!fit(ft, e)) return fail(err);
      if (v == StatVar::R && !ft.hasR) return fail(err);
      r = v == StatVar::A ? ft.a : v == StatVar::B ? ft.b : v == StatVar::C ? ft.c : ft.r;
      break;
    }
    default:
      return fail(err);
  }
  if (!inRange(r)) return fail(err);
  out = r;
  return true;
}

bool StatApp::estimate(StatVar v, double arg, double& out, CalcError* err) const {
  if (err) *err = CalcError::None;
  if (!available(v) || !statVarIsPostfix(v)) return fail(err);
  Fit ft;
  CalcError e;
  if (!fit(ft, e)) return fail(err);
  const double A = ft.a, B = ft.b, C = ft.c;
  double r = 0;
  if (v == StatVar::YHat) {
    switch (type_) {
      case StatType::Linear: r = A + B * arg; break;
      case StatType::Quad: r = A + B * arg + C * arg * arg; break;
      case StatType::Log:
        if (arg <= 0) return fail(err);
        r = A + B * std::log(arg);
        break;
      case StatType::Exp: r = A * std::exp(B * arg); break;
      case StatType::ABExp: r = A * std::exp(ft.lnB * arg); break;
      case StatType::Power: r = A * std::pow(arg, B); break;
      case StatType::Inverse:
        if (arg == 0) return fail(err);
        r = A + B / arg;
        break;
      default: return fail(err);
    }
  } else if (v == StatVar::XHat) {
    switch (type_) {
      case StatType::Linear:
        if (B == 0) return fail(err);
        r = (arg - A) / B;
        break;
      case StatType::Log:
        if (B == 0) return fail(err);
        r = std::exp((arg - A) / B);
        break;
      case StatType::Exp:
        if (B == 0 || arg / A <= 0) return fail(err);
        r = std::log(arg / A) / B;
        break;
      case StatType::ABExp:
        if (ft.lnB == 0 || arg / A <= 0) return fail(err);
        r = std::log(arg / A) / ft.lnB;
        break;
      case StatType::Power:
        if (B == 0 || arg / A <= 0) return fail(err);
        r = std::pow(arg / A, 1 / B);
        break;
      case StatType::Inverse:
        if (arg == A) return fail(err);
        r = B / (arg - A);
        break;
      default: return fail(err);
    }
  } else {  // x̂1 / x̂2: the roots of C x² + B x + (A - y) = 0
    if (C == 0) return fail(err);
    const double d = B * B - 4 * C * (A - arg);
    if (d < 0) return fail(err);
    const double s = std::sqrt(d);
    r = (v == StatVar::XHat1 ? -B + s : -B - s) / (2 * C);
  }
  if (!inRange(r)) return fail(err);
  out = r;
  return true;
}

// ============================================================================= the calculation line

bool StatApp::expand(const std::vector<StatItem>& e, std::vector<Tok>& out, CalcError& err) const {
  out.clear();
  err = CalcError::None;
  for (const StatItem& it : e) {
    if (it.var == StatVar::None) {
      if (it.tok >= Tok::Count) {
        err = CalcError::Syntax;
        return false;
      }
      out.push_back(it.tok);
      continue;
    }
    double v = 0;
    if (!statVarIsPostfix(it.var)) {
      if (!value(it.var, v, &err)) return false;
    } else {
      const int s = operandStart(out, static_cast<int>(out.size()) - 1);
      if (s < 0) {
        err = CalcError::Syntax;
        return false;
      }
      const std::vector<Tok> operand(out.begin() + s, out.end());
      Vars copy = vars();
      const EvalResult r = calc::evaluate(operand, copy, unit_, rnd());
      if (r.error != CalcError::None) {
        err = r.error;
        return false;
      }
      if (!estimate(it.var, r.value.v, v, &err)) return false;
      out.resize(static_cast<size_t>(s));
    }
    if (!numberTokens(v, out)) {
      err = CalcError::Math;
      return false;
    }
  }
  return true;
}

EvalResult StatApp::evaluate(const std::vector<StatItem>& e) const {
  EvalResult r;
  std::vector<Tok> toks;
  CalcError err;
  if (!expand(e, toks, err)) {
    r.error = err;
    return r;
  }
  if (toks.empty()) {
    r.error = CalcError::Syntax;
    return r;
  }
  Vars copy = vars();
  return calc::evaluate(toks, copy, unit_, rnd());
}

std::string StatApp::expressionText() const {
  std::string s;
  for (const StatItem& it : expr_) s += it.var == StatVar::None ? tokText(it.tok) : statVarText(it.var);
  return s;
}

std::string StatApp::resultText() const { return showing_ ? formatResult(result_, showFraction_, norm_) : ""; }

void StatApp::setExpression(const std::vector<StatItem>& e) {
  expr_ = e;
  showing_ = false;
  screen_ = Screen::Calc;
}

void StatApp::insertStat(StatVar v) {
  if (showing_) {
    expr_.clear();
    if (statVarIsPostfix(v)) expr_.push_back(StatItem::token(Tok::Ans));
    showing_ = false;
  }
  if (static_cast<int>(expr_.size()) < kMaxTokens) expr_.push_back(StatItem::stat(v));
  screen_ = Screen::Calc;
}

void StatApp::runCalc() {
  if (expr_.empty()) return;
  const EvalResult r = evaluate(expr_);
  if (r.error != CalcError::None) {
    showError(r.error, Screen::Calc);
    return;
  }
  Vars& vs = vars();
  vs.preAns = vs.ans;
  vs.ans = r.value;
  result_ = r.value;
  showing_ = true;
  showFraction_ = true;
}

void StatApp::showError(CalcError e, Screen back) {
  error_ = e;
  errorBack_ = back;
  screen_ = Screen::Error;
}

// ============================================================================= keys

namespace {
enum : uint8_t { kActType = 1, kActData, kActEdit, kActSum, kActVar, kActReg, kActMinMax, kActIns, kActDelA };
}

std::vector<StatApp::MenuItem> StatApp::menuItems() const {
  std::vector<MenuItem> items;
  auto act = [&](const char* t, uint8_t a) { items.push_back(MenuItem{t, StatVar::None, a}); };
  auto var = [&](StatVar v) { items.push_back(MenuItem{statVarText(v), v, 0}); };
  switch (menu_) {
    case Menu::Stat:
      act("Type", kActType);
      act("Data", kActData);
      if (menuFrom_ == Screen::Editor) act("Edit", kActEdit);
      act("Sum", kActSum);
      act("Var", kActVar);
      if (paired()) act("Reg", kActReg);
      act("MinMax", kActMinMax);
      break;
    case Menu::Edit:
      act("Ins", kActIns);
      act("Del-A", kActDelA);
      break;
    case Menu::Sum:
      var(StatVar::SumX2);
      var(StatVar::SumX);
      if (paired())
        for (StatVar v : {StatVar::SumY2, StatVar::SumY, StatVar::SumXY, StatVar::SumX3, StatVar::SumX2Y,
                          StatVar::SumX4})
          var(v);
      break;
    case Menu::Var:
      for (StatVar v : {StatVar::N, StatVar::MeanX, StatVar::SigmaX, StatVar::SX}) var(v);
      if (paired())
        for (StatVar v : {StatVar::MeanY, StatVar::SigmaY, StatVar::SY}) var(v);
      break;
    case Menu::Reg:
      if (type_ == StatType::Quad) {
        for (StatVar v : {StatVar::A, StatVar::B, StatVar::C, StatVar::XHat1, StatVar::XHat2, StatVar::YHat}) var(v);
      } else {
        for (StatVar v : {StatVar::A, StatVar::B, StatVar::R, StatVar::XHat, StatVar::YHat}) var(v);
      }
      break;
    case Menu::MinMax:
      var(StatVar::MinX);
      var(StatVar::MaxX);
      if (paired()) {
        var(StatVar::MinY);
        var(StatVar::MaxY);
      }
      break;
  }
  return items;
}

void StatApp::openMenu(Menu m) {
  menu_ = m;
  screen_ = Screen::Menu;
}

void StatApp::pickMenu(int n) {
  const std::vector<MenuItem> items = menuItems();
  if (n < 1 || n > static_cast<int>(items.size())) return;
  const MenuItem& it = items[static_cast<size_t>(n - 1)];
  if (it.var != StatVar::None) {
    typing_ = false;
    entry_.clear();
    insertStat(it.var);
    return;
  }
  switch (it.action) {
    case kActType: screen_ = Screen::TypeSelect; break;
    case kActData:
      typing_ = false;
      entry_.clear();
      screen_ = Screen::Editor;
      break;
    case kActEdit: openMenu(Menu::Edit); break;
    case kActSum: openMenu(Menu::Sum); break;
    case kActVar: openMenu(Menu::Var); break;
    case kActReg: openMenu(Menu::Reg); break;
    case kActMinMax: openMenu(Menu::MinMax); break;
    case kActIns:
      insertRow(row_ < rows_ ? row_ : rows_);
      moveTo(row_, col_);
      screen_ = Screen::Editor;
      break;
    case kActDelA:
      clearData();
      screen_ = Screen::Editor;
      break;
    default: break;
  }
}

void StatApp::moveTo(int row, int col) {
  const int lastRow = rows_ < maxRows() ? rows_ : maxRows() - 1;
  row_ = row < 0 ? 0 : (row > lastRow ? lastRow : row);
  const int nc = columns();
  col_ = col < 0 ? 0 : (col >= nc ? nc - 1 : col);
  if (row_ < top_) top_ = row_;
  if (row_ >= top_ + kEditorRows) top_ = row_ - kEditorRows + 1;
  if (top_ < 0) top_ = 0;
}

bool StatApp::onKey(DKey k, bool shift, bool alpha) {
  if (k == DKey::Shift || k == DKey::Alpha || k == DKey::Mode || k == DKey::On || k >= DKey::Count) return false;
  switch (screen_) {
    case Screen::TypeSelect: keyTypeSelect(k); break;
    case Screen::Editor: keyEditor(k, shift, alpha); break;
    case Screen::Calc: keyCalc(k, shift, alpha); break;
    case Screen::Menu: {
      const int d = digitOf(k);
      if (k == DKey::AC) screen_ = menuFrom_;
      else if (d >= 1 && !shift && !alpha) pickMenu(d);
      else return false;
      break;
    }
    case Screen::Error:
      if (k == DKey::AC) {
        if (errorBack_ == Screen::Editor) {
          typing_ = false;
          entry_.clear();
        } else {
          expr_.clear();
          showing_ = false;
        }
        screen_ = errorBack_;
      } else if (k == DKey::Left || k == DKey::Right) {
        if (errorBack_ == Screen::Calc) showing_ = false;
        screen_ = errorBack_;
      } else {
        return false;
      }
      break;
  }
  return true;
}

void StatApp::keyTypeSelect(DKey k) {
  const int d = digitOf(k);
  if (d >= 1 && d <= 8) {
    setType(static_cast<StatType>(d - 1));
    return;
  }
  if (k == DKey::AC) {
    if (typeChosen_) screen_ = menuFrom_;
    else exit_ = true;
  }
}

void StatApp::keyEditor(DKey k, bool shift, bool alpha) {
  if (shift && k == DKey::D1) {  // SHIFT 1 = STAT
    typing_ = false;
    entry_.clear();
    menuFrom_ = Screen::Editor;
    openMenu(Menu::Stat);
    return;
  }
  Tok t;
  if (typing_) {
    if (k == DKey::Eq) {
      commitEntry();
    } else if (k == DKey::Del) {
      if (!entry_.empty()) entry_.pop_back();
    } else if (k == DKey::AC) {
      typing_ = false;
      entry_.clear();
    } else if (modeKeyToken(k, shift, alpha, t)) {
      if (static_cast<int>(entry_.size()) < kMaxTokens) entry_.push_back(t);
    }
    return;
  }
  switch (k) {
    case DKey::Up: moveTo(row_ - 1, col_); return;
    case DKey::Down: moveTo(row_ + 1, col_); return;
    case DKey::Left: moveTo(row_, col_ - 1); return;
    case DKey::Right: moveTo(row_, col_ + 1); return;
    case DKey::Del: deleteRow(row_); return;
    case DKey::AC:
      screen_ = Screen::Calc;
      return;
    default: break;
  }
  if (modeKeyToken(k, shift, alpha, t)) {
    typing_ = true;
    entry_.assign(1, t);
  }
}

void StatApp::commitEntry() {
  if (entry_.empty()) {
    typing_ = false;
    return;
  }
  Vars copy = vars();
  const EvalResult r = calc::evaluate(entry_, copy, unit_, rnd());
  if (r.error != CalcError::None) {
    showError(r.error, Screen::Editor);
    return;
  }
  const Column c = column(col_);
  if (!inRange(r.value.v) || !setCell(row_, c, r.value.v)) {
    showError(CalcError::Math, Screen::Editor);
    return;
  }
  typing_ = false;
  entry_.clear();
  moveTo(row_ + 1, col_);
}

void StatApp::keyCalc(DKey k, bool shift, bool alpha) {
  if (shift && k == DKey::D1) {
    menuFrom_ = Screen::Calc;
    openMenu(Menu::Stat);
    return;
  }
  Tok t;
  switch (k) {
    case DKey::Eq:
      runCalc();
      return;
    case DKey::AC:
      expr_.clear();
      showing_ = false;
      return;
    case DKey::Del:
      if (showing_) showing_ = false;
      else if (!expr_.empty()) expr_.pop_back();
      return;
    case DKey::Left: case DKey::Right:
      showing_ = false;
      return;
    case DKey::SD:
      if (!alpha) {
        if (showing_) showFraction_ = !showFraction_;
        return;
      }
      break;
    default: break;
  }
  if (!modeKeyToken(k, shift, alpha, t)) return;
  if (showing_) {
    expr_.clear();
    if (continuesAns(t)) expr_.push_back(StatItem::token(Tok::Ans));
    showing_ = false;
  }
  if (static_cast<int>(expr_.size()) < kMaxTokens) expr_.push_back(StatItem::token(t));
}

// ============================================================================= render

void StatApp::render(Framebuffer& fb) const {
  fb.setIcon(Icon::Up, false);
  fb.setIcon(Icon::Down, false);
  auto twoCols = [&](const std::vector<std::string>& items) {
    for (size_t i = 0; i < items.size(); ++i) {
      const std::string s = std::to_string(i + 1) + ":" + items[i];
      drawMarkedText(fb, (i % 2) * 12, 1 + static_cast<int>(i / 2), s);
    }
  };
  switch (screen_) {
    case Screen::TypeSelect: {
      std::vector<std::string> names;
      for (int i = 0; i < static_cast<int>(StatType::Count); ++i) names.push_back(statTypeName(static_cast<StatType>(i)));
      twoCols(names);
      break;
    }
    case Screen::Editor: drawEditor(fb); break;
    case Screen::Calc: drawCalc(fb); break;
    case Screen::Menu: {
      std::vector<std::string> names;
      for (const MenuItem& it : menuItems()) names.push_back(it.text);
      twoCols(names);
      break;
    }
    case Screen::Error: drawErrorScreen(fb, errorText(error_)); break;
  }
}

void StatApp::drawEditor(Framebuffer& fb) const {
  const int nc = columns();
  auto colStart = [](int c) { return 3 + c * 7; };
  static const char* const kHead[] = {"X", "Y", "FREQ"};
  for (int c = 0; c < nc; ++c) {
    const char* h = kHead[static_cast<int>(column(c))];
    const int len = static_cast<int>(std::strlen(h));
    fb.drawText(colStart(c) + (6 - len) / 2, 1, h);
  }
  const int lastRow = rows_ < maxRows() ? rows_ : maxRows() - 1;
  for (int r = 0; r < kEditorRows; ++r) {
    const int i = top_ + r;
    if (i > lastRow) break;
    const int row = 2 + r;
    const std::string num = std::to_string(i + 1);
    fb.drawText(2 - static_cast<int>(num.size()), row, num);
    for (int c = 0; c < nc; ++c) {
      if (i < rows_) {
        const std::string s = cellText(cell(i, column(c)), 6);
        fb.drawText(colStart(c) + 6 - static_cast<int>(s.size()), row, s);
      }
      if (i == row_ && c == col_) fb.invertRect(colStart(c) * 5 - 1, row * 8, 6 * 5 + 1, 8);
    }
  }
  if (typing_) {
    drawEntry(fb, 6, 1, exprText(entry_), true);
  } else if (row_ < rows_) {
    rightText(fb, 6, formatDecimal(cell(row_, column(col_)), norm_));
  }
  fb.setIcon(Icon::Up, top_ > 0);
  fb.setIcon(Icon::Down, top_ + kEditorRows - 1 < lastRow);
}

void StatApp::drawCalc(Framebuffer& fb) const {
  drawEntry(fb, 1, showing_ ? 3 : 4, expressionText(), !showing_);
  if (showing_) {
    const std::string r = resultText();
    const int n = textLength(r);
    if (n * 10 <= Framebuffer::kWidth) fb.drawTextPx(Framebuffer::kWidth - n * 10 + 1, 46, r, 2);
    else fb.drawTextPx(Framebuffer::kWidth - n * 5 + 1, 52, r, 1);
  }
}

}  // namespace calc
