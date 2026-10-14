#include "table.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "stat.h"  // modeKeyToken, cellText, drawEntry, drawErrorScreen
#include "text.h"

namespace calc {

namespace {
constexpr int kCols = Framebuffer::kCols;
constexpr int kXCol = 3, kXWidth = 9;   // X cells: columns 3-11
constexpr int kFCol = 13, kFWidth = 11; // F(X) cells: columns 13-23

double fallbackRandom() { return static_cast<double>(std::rand()) / (static_cast<double>(RAND_MAX) + 1.0); }

void rightText(Framebuffer& fb, int row, const std::string& s) {
  const int n = textLength(s);
  fb.drawText(n >= kCols ? 0 : kCols - n, row, s);
}
}  // namespace

void TableApp::reset() {
  screen_ = errorBack_ = Screen::Function;
  func_.clear();
  entry_.clear();
  start_ = 1;
  end_ = 5;
  step_ = 1;
  rows_ = 0;
  row_ = col_ = top_ = 0;
  errorMsg_ = "";
  exit_ = false;
}

void TableApp::setEnv(Vars* vars, AngleUnit unit, NormMode norm, double (*random)()) {
  vars_ = vars;
  unit_ = unit;
  norm_ = norm;
  random_ = random;
}

Vars& TableApp::vars() const { return vars_ ? *vars_ : ownVars_; }

void TableApp::setFunction(const std::vector<Tok>& toks) {
  func_.assign(toks.begin(), toks.size() > static_cast<size_t>(kMaxTokens) ? toks.begin() + kMaxTokens : toks.end());
}

bool TableApp::setFunctionText(const std::string& text) {
  // parseExpression() has no variables: write each one as "ans" (a value it knows), then
  // turn those Ans tokens back into the variable, leaving any "ans" the text really had.
  // X / x anywhere (but the x of "exp("); A B C D F Y M as single capitals (E stays ×10 / e).
  auto isLetter = [&](size_t j) {
    return j < text.size() && ((text[j] >= 'a' && text[j] <= 'z') || (text[j] >= 'A' && text[j] <= 'Z'));
  };
  std::string s;
  std::vector<Tok> vars;  // per "ans" in s, in order: the variable, or Ans for a real "ans"
  for (size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    const bool expX = (c == 'x' || c == 'X') && i > 0 && (text[i - 1] == 'e' || text[i - 1] == 'E') &&
                      i + 1 < text.size() && (text[i + 1] == 'p' || text[i + 1] == 'P');
    Tok v = Tok::Count;
    if ((c == 'x' || c == 'X') && !expX) {
      v = Tok::VarX;
    } else if (!(i > 0 && isLetter(i - 1)) && !isLetter(i + 1)) {
      switch (c) {
        case 'A': v = Tok::VarA; break;
        case 'B': v = Tok::VarB; break;
        case 'C': v = Tok::VarC; break;
        case 'D': v = Tok::VarD; break;
        case 'F': v = Tok::VarF; break;
        case 'Y': v = Tok::VarY; break;
        case 'M': v = Tok::VarM; break;
        default: break;
      }
    }
    if (v != Tok::Count) {
      s += " ans ";
      vars.push_back(v);
      continue;
    }
    if (i + 3 <= text.size()) {
      std::string w = text.substr(i, 3);
      for (char& ch : w) ch = static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
      if (w == "ans") vars.push_back(Tok::Ans);
    }
    s += c;
  }
  std::vector<Tok> toks;
  if (!parseExpression(s, toks)) return false;
  size_t k = 0;
  for (Tok& t : toks)
    if (t == Tok::Ans) {
      if (k < vars.size()) t = vars[k];
      ++k;
    }
  setFunction(toks);
  return true;
}

void TableApp::setRange(double start, double end, double step) {
  start_ = start;
  end_ = end;
  step_ = step;
}

int TableApp::rowCount(double start, double end, double step) {
  if (step == 0 || !std::isfinite(start) || !std::isfinite(end) || !std::isfinite(step)) return 0;
  const double q = (end - start) / step;
  if (!std::isfinite(q) || q < -1e-9) return 0;
  if (q >= kMaxRows) return kMaxRows + 1;
  return static_cast<int>(std::floor(q + 1e-9)) + 1;
}

// Row i's X without the binary rounding of start + i·step: −0.3 + 3 × 0.1 is 5.6E-17, not 0,
// and −0.3 + 2 × 0.1 is −0.19999999999999998. Below 1E-9 of a step it is 0; otherwise rounded
// to 15 significant digits (the decimal the user meant; the display shows 10).
double TableApp::rowValue(double start, double step, int i) {
  const double x = start + i * step;
  if (std::fabs(x) < 1e-9 * std::fabs(step)) return 0;
  char b[40];
  std::snprintf(b, sizeof b, "%.14e", x);
  const double r = std::strtod(b, nullptr);
  return std::isfinite(r) ? r : x;
}

bool TableApp::build() {
  const int n = rowCount(start_, end_, step_);
  if (n == 0) {
    showError(errorText(CalcError::Math), Screen::Start);
    return false;
  }
  if (n > kMaxRows) {
    showError("Insufficient MEM", Screen::Start);
    return false;
  }
  rows_ = n;
  for (int i = 0; i < n; ++i) {
    const double x = rowValue(start_, step_, i);
    xs_[i] = x;
    Vars copy = vars();
    copy.x = Num::ofDouble(x);
    const EvalResult r = func_.empty() ? EvalResult{CalcError::Syntax, 0, Num()}
                                       : evaluate(func_, copy, unit_, random_ ? random_ : fallbackRandom);
    errs_[i] = r.error;
    ys_[i] = r.error == CalcError::None ? r.value.v : 0;
    if (r.error == CalcError::None && !(std::isfinite(ys_[i]) && std::fabs(ys_[i]) < 1e100)) {
      errs_[i] = CalcError::Math;
      ys_[i] = 0;
    }
  }
  row_ = col_ = top_ = 0;
  screen_ = Screen::Table;
  return true;
}

void TableApp::showError(const char* msg, Screen back) {
  errorMsg_ = msg;
  errorBack_ = back;
  screen_ = Screen::Error;
}

void TableApp::moveTo(int row) {
  row_ = row < 0 ? 0 : (row >= rows_ ? (rows_ > 0 ? rows_ - 1 : 0) : row);
  if (row_ < top_) top_ = row_;
  if (row_ >= top_ + kVisibleRows) top_ = row_ - kVisibleRows + 1;
}

bool TableApp::onKey(DKey k, bool shift, bool alpha) {
  if (k == DKey::Shift || k == DKey::Alpha || k == DKey::Mode || k == DKey::On || k >= DKey::Count) return false;
  switch (screen_) {
    case Screen::Function: keyFunction(k, shift, alpha); break;
    case Screen::Start: case Screen::End: case Screen::Step: keyRange(k, shift, alpha); break;
    case Screen::Table: keyTable(k); break;
    case Screen::Error:
      if (k == DKey::AC) entry_.clear();
      else if (k != DKey::Left && k != DKey::Right) return false;
      screen_ = errorBack_;
      break;
  }
  return true;
}

void TableApp::keyFunction(DKey k, bool shift, bool alpha) {
  Tok t;
  switch (k) {
    case DKey::Eq:
      if (!func_.empty()) {
        entry_.clear();
        screen_ = Screen::Start;
      }
      return;
    case DKey::Del:
      if (!func_.empty()) func_.pop_back();
      return;
    case DKey::AC:
      if (func_.empty()) exit_ = true;
      func_.clear();
      return;
    default: break;
  }
  if (modeKeyToken(k, shift, alpha, t) && static_cast<int>(func_.size()) < kMaxTokens) func_.push_back(t);
}

void TableApp::keyRange(DKey k, bool shift, bool alpha) {
  Tok t;
  switch (k) {
    case DKey::AC:
      entry_.clear();
      screen_ = Screen::Function;
      return;
    case DKey::Del:
      if (!entry_.empty()) entry_.pop_back();
      return;
    case DKey::Eq: {
      if (!entry_.empty()) {
        Vars copy = vars();
        const EvalResult r = evaluate(entry_, copy, unit_, random_ ? random_ : fallbackRandom);
        if (r.error != CalcError::None) {
          showError(errorText(r.error), screen_);
          return;
        }
        if (!(std::isfinite(r.value.v) && std::fabs(r.value.v) < 1e100)) {
          showError(errorText(CalcError::Math), screen_);
          return;
        }
        (screen_ == Screen::Start ? start_ : screen_ == Screen::End ? end_ : step_) = r.value.v;
      }
      entry_.clear();
      if (screen_ == Screen::Start) screen_ = Screen::End;
      else if (screen_ == Screen::End) screen_ = Screen::Step;
      else build();
      return;
    }
    default: break;
  }
  if (modeKeyToken(k, shift, alpha, t) && static_cast<int>(entry_.size()) < kMaxTokens) entry_.push_back(t);
}

void TableApp::keyTable(DKey k) {
  switch (k) {
    case DKey::Up: moveTo(row_ - 1); break;
    case DKey::Down: moveTo(row_ + 1); break;
    case DKey::Left: col_ = 0; break;
    case DKey::Right: col_ = 1; break;
    case DKey::AC: screen_ = Screen::Function; break;
    default: break;
  }
}

void TableApp::render(Framebuffer& fb) const {
  fb.setIcon(Icon::Up, false);
  fb.setIcon(Icon::Down, false);
  switch (screen_) {
    case Screen::Function:
      drawEntry(fb, 1, 5, "f(X)=" + functionText(), true);
      break;
    case Screen::Start: case Screen::End: case Screen::Step: {
      const double v = screen_ == Screen::Start ? start_ : screen_ == Screen::End ? end_ : step_;
      fb.drawText(0, 1, screen_ == Screen::Start ? "Start?" : screen_ == Screen::End ? "End?" : "Step?");
      if (entry_.empty()) rightText(fb, 2, formatDecimal(v, norm_));
      else drawEntry(fb, 2, 2, exprText(entry_), true);
      break;
    }
    case Screen::Table: {
      fb.drawText(kXCol + (kXWidth - 1) / 2, 1, "X");
      fb.drawText(kFCol + (kFWidth - 4) / 2, 1, "F(X)");
      for (int r = 0; r < kVisibleRows && top_ + r < rows_; ++r) {
        const int i = top_ + r, row = 2 + r;
        const std::string num = std::to_string(i + 1);
        fb.drawText(2 - static_cast<int>(num.size()), row, num);
        const std::string xs = cellText(xs_[i], kXWidth);
        fb.drawText(kXCol + kXWidth - static_cast<int>(xs.size()), row, xs);
        const std::string ys = errs_[i] == CalcError::None ? cellText(ys_[i], kFWidth) : "ERROR";
        fb.drawText(kFCol + kFWidth - static_cast<int>(ys.size()), row, ys);
        if (i == row_) {
          const int c0 = col_ == 0 ? kXCol : kFCol, w = col_ == 0 ? kXWidth : kFWidth;
          fb.invertRect(c0 * 5 - 1, row * 8, w * 5 + 1, 8);
        }
      }
      if (rows_ > 0) {
        if (col_ == 0) rightText(fb, 6, formatDecimal(xs_[row_], norm_));
        else rightText(fb, 6, errs_[row_] == CalcError::None ? formatDecimal(ys_[row_], norm_) : "ERROR");
      }
      fb.setIcon(Icon::Up, top_ > 0);
      fb.setIcon(Icon::Down, top_ + kVisibleRows < rows_);
      break;
    }
    case Screen::Error:
      drawErrorScreen(fb, errorMsg_);
      break;
  }
}

}  // namespace calc
