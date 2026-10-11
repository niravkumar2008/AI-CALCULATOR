#include "device.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "font.h"
#include "text.h"

namespace calc {

namespace {

const char* kKeyNames[] = {
    "SHIFT", "ALPHA", "UP", "DOWN", "LEFT", "RIGHT", "MODE", "ON",
    "CALC", "int_dx", "x^-1", "log_a",
    "frac", "sqrt", "x^2", "x^n", "log", "ln",
    "(-)", "dms", "hyp", "sin", "cos", "tan",
    "RCL", "ENG", "(", ")", "S<>D", "M+",
    "7", "8", "9", "DEL", "AC",
    "4", "5", "6", "x", "/",
    "1", "2", "3", "+", "-",
    "0", ".", "x10^x", "Ans", "=",
};
static_assert(sizeof(kKeyNames) / sizeof(kKeyNames[0]) == static_cast<size_t>(DKey::Count), "key names");

constexpr int kCols = Framebuffer::kCols;
constexpr int kExprRows = 4;  // rows 1-4 hold the expression, the result goes below

double defaultRandom() {
  static uint32_t s = 2463534242u;  // xorshift32; platforms can pass a better source
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return (s >> 8) / 16777216.0;
}

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

// The variable printed in red above a key (typed with ALPHA, or after STO / RCL).
bool varOf(DKey k, Tok& t) {
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

// The token a key types, with or without SHIFT / ALPHA.
bool tokOf(DKey k, bool shift, bool alpha, Tok& t) {
  if (alpha) {
    if (varOf(k, t)) return true;
    if (k == DKey::Exp10) { t = Tok::E; return true; }
    if (k == DKey::Dot) { t = Tok::RanInt; return true; }
    return false;
  }
  const int d = digitOf(k);
  if (d >= 0) {
    if (!shift) { t = static_cast<Tok>(d); return true; }
    if (d == 0) { t = Tok::Rnd; return true; }  // SHIFT 0 = Rnd(
    return false;                               // SHIFT 9 = CLR is handled as a command
  }
  struct M { DKey k; Tok plain, shifted; };
  static const M kMap[] = {
      {DKey::LogAB, Tok::Log, Tok::Count},  // logₐb: log(base,value)
      {DKey::Inv, Tok::Inv, Tok::Fact},
      {DKey::Frac, Tok::Frac, Tok::Count},
      {DKey::Sqrt, Tok::Sqrt, Tok::Cbrt},   {DKey::Sq, Tok::Sq, Tok::Cube},  // fx-115ES: SHIFT √ = ∛, SHIFT x² = x³
      {DKey::Pow, Tok::Pow, Tok::XRoot},    {DKey::Log, Tok::Log, Tok::Pow10},
      {DKey::Ln, Tok::Ln, Tok::Exp},        {DKey::Neg, Tok::Neg, Tok::Count},
      {DKey::Hyp, Tok::Count, Tok::Abs},    {DKey::Sin, Tok::Sin, Tok::Asin},  // fx-115ES: SHIFT hyp = Abs
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

// After a result, these keys carry on from it: "Ans+" ...
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

std::string clockText(uint32_t ms) {  // h:mm
  const uint32_t min = ms / 60000;
  char b[16];
  std::snprintf(b, sizeof b, "%u:%02u", static_cast<unsigned>(min / 60), static_cast<unsigned>(min % 60));
  return b;
}

std::string pad(std::string s, int width) {
  const int n = textLength(s);
  if (n < width) s.append(static_cast<size_t>(width - n), ' ');
  return s;
}

// A menu line with two options: "1:Deg      2:Rad".
std::string two(const std::string& a, const std::string& b) { return pad(a, 11) + b; }

}  // namespace

bool keysForChar(char c, DKey out[2], int& n) {
  struct M { char c; DKey a, b; int n; };
  static const M kMap[] = {
      {'0', DKey::D0, DKey::D0, 1}, {'1', DKey::D1, DKey::D1, 1}, {'2', DKey::D2, DKey::D2, 1},
      {'3', DKey::D3, DKey::D3, 1}, {'4', DKey::D4, DKey::D4, 1}, {'5', DKey::D5, DKey::D5, 1},
      {'6', DKey::D6, DKey::D6, 1}, {'7', DKey::D7, DKey::D7, 1}, {'8', DKey::D8, DKey::D8, 1},
      {'9', DKey::D9, DKey::D9, 1}, {'.', DKey::Dot, DKey::Dot, 1}, {'+', DKey::Add, DKey::Add, 1},
      {'-', DKey::Sub, DKey::Sub, 1}, {'*', DKey::Mul, DKey::Mul, 1}, {'/', DKey::Div, DKey::Div, 1},
      {'^', DKey::Pow, DKey::Pow, 1}, {'(', DKey::Open, DKey::Open, 1}, {')', DKey::Close, DKey::Close, 1},
      {'=', DKey::Eq, DKey::Eq, 1}, {'~', DKey::Neg, DKey::Neg, 1}, {'E', DKey::Exp10, DKey::Exp10, 1},
      {'s', DKey::Sin, DKey::Sin, 1}, {'c', DKey::Cos, DKey::Cos, 1}, {'t', DKey::Tan, DKey::Tan, 1},
      {'l', DKey::Log, DKey::Log, 1}, {'n', DKey::Ln, DKey::Ln, 1}, {'r', DKey::Sqrt, DKey::Sqrt, 1},
      {'q', DKey::Sq, DKey::Sq, 1}, {'i', DKey::Inv, DKey::Inv, 1}, {'f', DKey::Frac, DKey::Frac, 1},
      {'b', DKey::Shift, DKey::Hyp, 2}, {'g', DKey::LogAB, DKey::LogAB, 1},
      {'h', DKey::Hyp, DKey::Hyp, 1}, {'k', DKey::Rcl, DKey::Rcl, 1}, {'w', DKey::SD, DKey::SD, 1},
      {'M', DKey::MPlus, DKey::MPlus, 1}, {'a', DKey::Ans, DKey::Ans, 1}, {'#', DKey::Del, DKey::Del, 1},
      {'$', DKey::AC, DKey::AC, 1}, {'m', DKey::Mode, DKey::Mode, 1}, {'o', DKey::On, DKey::On, 1},
      {'[', DKey::Shift, DKey::Shift, 1}, {']', DKey::Alpha, DKey::Alpha, 1},
      {'<', DKey::Left, DKey::Left, 1}, {'>', DKey::Right, DKey::Right, 1},
      {'u', DKey::Up, DKey::Up, 1}, {'d', DKey::Down, DKey::Down, 1},
      {'p', DKey::Shift, DKey::Exp10, 2}, {'!', DKey::Shift, DKey::Inv, 2},
      {'%', DKey::Shift, DKey::Open, 2}, {',', DKey::Shift, DKey::Close, 2},
  };
  for (const M& m : kMap)
    if (m.c == c) {
      out[0] = m.a;
      out[1] = m.b;
      n = m.n;
      return true;
    }
  return false;
}

const char* dkeyName(DKey k) { return k < DKey::Count ? kKeyNames[static_cast<int>(k)] : "?"; }

Device::Device() { random_ = defaultRandom; }

// ---------------------------------------------------------------- time, power, exam

void Device::tick(uint32_t nowMs) {
  if (!timeKnown_) {
    nowMs_ = lastKeyMs_ = nowMs;
    timeKnown_ = true;
  }
  const uint32_t dt = nowMs - nowMs_;  // unsigned: survives millis() wrap-around
  nowMs_ = nowMs;
  app_.tick(nowMs);
  if (exam_) {
    examMs_ += dt;
    if (examMs_ >= kExamMaxMs) endExam("12 hours passed.");
  }
  const bool aiBusy = app_.activeRequest() != 0;
  if (view_ != View::Off && !aiBusy && nowMs - lastKeyMs_ >= kAutoOffMs) powerOff();
}

void Device::powerOn() {
  view_ = back_ = mode_ == Mode::Ai ? View::Ai : View::Calc;
  app_.onKey(Key::On);
  clearEntry();
  shift_ = alpha_ = sto_ = rcl_ = false;
}

void Device::powerOff() {
  app_.onKey(Key::Off);  // cancels a request in flight
  view_ = View::Off;
  shift_ = alpha_ = sto_ = rcl_ = false;
  offSequence_ = 0;
}

void Device::startExam() {
  exam_ = true;
  examMs_ = 0;
  if (mode_ == Mode::Ai) leaveAi();
  mode_ = Mode::Comp;
  online_ = false;
  app_.setOnline(false);
  back_ = View::Calc;
  notice({"EXAM MODE ON", "AI, camera and Wi-Fi are", "off. Unlock: USB cable,", "or wait 12 hours.", "",
          "AC:continue"});
}

void Device::endExam(const char* why) {
  if (!exam_) return;
  exam_ = false;
  examMs_ = 0;
  if (view_ == View::Off) return;
  back_ = mode_ == Mode::Ai ? View::Ai : View::Calc;
  notice({"EXAM MODE OFF", why, "AI and Wi-Fi are back.", "", "", "AC:continue"});
}

void Device::usbUnlock() { endExam("Unlocked by USB cable."); }

void Device::restoreExam(uint32_t elapsedMs) {
  if (elapsedMs >= kExamMaxMs) return;
  exam_ = true;
  examMs_ = elapsedMs;
  if (mode_ == Mode::Ai) leaveAi();
  mode_ = Mode::Comp;
  online_ = false;
  app_.setOnline(false);
  if (view_ == View::Ai) view_ = back_ = View::Calc;
}

void Device::setOnline(bool online) {
  online_ = online && !exam_;
  app_.setOnline(online_);
}

void Device::setNetInfo(const std::string& ssid, const std::string& keyHint, const std::string& howToChange) {
  ssid_ = ssid;
  keyHint_ = keyHint;
  howToChange_ = howToChange;
}

// ---------------------------------------------------------------- keys

void Device::onKey(DKey k) {
  lastKeyMs_ = nowMs_;
  if (view_ == View::Off) {
    // SHIFT, 7, ON (in that order, while off) starts exam mode.
    if (k == DKey::Shift) {
      offSequence_ = 1;
    } else if (k == DKey::D7 && offSequence_ == 1) {
      offSequence_ = 2;
    } else if (k == DKey::On) {
      const bool exam = offSequence_ == 2;
      powerOn();
      if (exam && !exam_) startExam();
      offSequence_ = 0;
    } else {
      offSequence_ = 0;
    }
    return;
  }
  if (k == DKey::On) {  // ON while on clears, like AC
    if (view_ == View::Ai) {
      app_.onKey(Key::AC);
    } else {
      view_ = View::Calc;
      if (mode_ == Mode::Ai) view_ = View::Ai;
      if (view_ == View::Calc) clearEntry();
    }
    shift_ = alpha_ = sto_ = rcl_ = false;
    return;
  }
  if (k == DKey::Shift) {
    shift_ = !shift_;
    alpha_ = false;
    return;
  }
  if (k == DKey::Alpha) {
    alpha_ = !alpha_;
    shift_ = false;
    return;
  }
  const bool shift = shift_, alpha = alpha_;
  shift_ = alpha_ = false;

  if (k == DKey::AC && shift) return powerOff();
  if (k == DKey::Mode) {
    sto_ = rcl_ = false;
    return openMenu(shift ? View::Setup : View::ModeMenu);
  }

  const int d = digitOf(k);
  auto pick = [d](int lo, int hi) { return d >= lo && d <= hi ? d : 0; };

  switch (view_) {
    case View::Off:
      return;
    case View::Calc:
      return keyCalc(k, shift, alpha);
    case View::Ai:
      return keyAi(k);
    case View::Error:
      if (k == DKey::Left || k == DKey::Right) {  // Goto: put the cursor where it went wrong
        view_ = View::Calc;
        showingResult_ = false;
        const int n = static_cast<int>(expr_.size());
        cursor_ = errorPos_ < 0 ? 0 : (errorPos_ > n ? n : errorPos_);
      } else if (k == DKey::AC || k == DKey::Del) {
        view_ = View::Calc;
        clearEntry();
      }
      return;
    case View::ModeMenu:
      if (k == DKey::AC) {
        view_ = back_;
        return;
      }
      switch (pick(1, 4)) {
        case 1:
          if (mode_ == Mode::Ai) leaveAi();
          mode_ = Mode::Comp;
          view_ = back_ = View::Calc;
          clearEntry();
          return;
        case 2:
        case 3:
          back_ = mode_ == Mode::Ai ? View::Ai : View::Calc;
          return notice({d == 2 ? "STAT" : "TABLE", "Coming in a later", "update.", "", "", "AC:back"});
        case 4:
          if (exam_) {
            back_ = View::Calc;
            return notice({"AI SOLVE IS OFF", "Exam mode is on.", "", "", "", "AC:back"});
          }
          mode_ = Mode::Ai;
          view_ = back_ = View::Ai;
          return;
      }
      return;
    case View::Setup:
      if (k == DKey::AC) {
        view_ = back_;
        return;
      }
      switch (pick(1, 8)) {
        case 1: angle_ = AngleUnit::Deg; view_ = back_; return;
        case 2: angle_ = AngleUnit::Rad; view_ = back_; return;
        case 3: angle_ = AngleUnit::Gra; view_ = back_; return;
        case 4: norm_ = NormMode::Norm1; view_ = back_; return;
        case 5: norm_ = NormMode::Norm2; view_ = back_; return;
        case 6: view_ = View::NetInfo; return;
        case 7: view_ = View::Look; return;
        case 8: view_ = View::ExamInfo; return;
      }
      return;
    case View::Look:
      if (k == DKey::AC) {
        view_ = View::Setup;
      } else if (pick(1, 2)) {
        dotMatrix_ = d == 2;
        view_ = back_;
      }
      return;
    case View::NetInfo:
      if (k == DKey::AC || k == DKey::Eq) view_ = View::Setup;
      return;
    case View::ExamInfo:
      if (k == DKey::AC) view_ = View::Setup;
      else if (k == DKey::Eq && !exam_) startExam();
      return;
    case View::Hyp: {
      static const Tok kHyp[] = {Tok::Sinh, Tok::Cosh, Tok::Tanh, Tok::Asinh, Tok::Acosh, Tok::Atanh};
      if (k == DKey::AC) {
        view_ = View::Calc;
      } else if (pick(1, 6)) {
        view_ = View::Calc;
        insert(kHyp[d - 1]);
      }
      return;
    }
    case View::Clr:
      if (k == DKey::AC) {
        view_ = View::Calc;
      } else if (pick(1, 3)) {
        clrChoice_ = d;
        view_ = View::ClrConfirm;
      }
      return;
    case View::ClrConfirm:
      if (k == DKey::AC) {
        view_ = View::Calc;
      } else if (k == DKey::Eq) {
        if (clrChoice_ != 2) {  // setup (never exam mode: only the USB cable ends that)
          angle_ = AngleUnit::Deg;
          norm_ = NormMode::Norm1;
          dotMatrix_ = true;
        }
        if (clrChoice_ != 1) {  // memory
          vars_ = Vars();
          history_.clear();
        }
        clearEntry();
        back_ = View::Calc;
        notice({clrChoice_ == 1 ? "Reset Setup" : clrChoice_ == 2 ? "Clear Memory" : "Reset All", "",
                "Press [AC] key"});
      }
      return;
    case View::Notice:
      if (k == DKey::AC || k == DKey::Eq) view_ = back_;
      return;
  }
}

void Device::openMenu(View v) {
  if (view_ == View::Calc || view_ == View::Ai) back_ = view_;
  view_ = v;
}

void Device::notice(const std::vector<std::string>& lines) {
  notice_ = lines;
  view_ = View::Notice;
}

void Device::keyAi(DKey k) {
  switch (k) {
    case DKey::Eq: app_.onKey(Key::Eq); break;
    case DKey::D1:  // 1 on the AI home screen: tutor mode on / off
      if (app_.screen() == Screen::Ready) app_.onKey(Key::Tutor);
      break;
    case DKey::Up: app_.onKey(Key::Up); break;
    case DKey::Down: app_.onKey(Key::Down); break;
    case DKey::AC:
      if (app_.screen() == Screen::Ready) {  // AC on the AI home screen: back to the calculator
        mode_ = Mode::Comp;
        view_ = back_ = View::Calc;
      } else if (app_.screen() == Screen::Result && scanNote_ != ScanNote::None && app_.activeRequest() == 0) {
        app_.onKey(Key::AC);  // the scan waits in the calculator: go and check it
        mode_ = Mode::Comp;
        view_ = back_ = View::Calc;
      } else {
        app_.onKey(Key::AC);
      }
      break;
    default:
      break;
  }
}

bool Device::startScan() {
  if (view_ == View::Off || exam_) return false;
  if (mode_ == Mode::Ai) leaveAi();
  mode_ = Mode::Ai;
  view_ = back_ = View::Ai;
  app_.onKey(Key::Eq);
  return app_.activeRequest() != 0;
}

void Device::leaveAi() {
  for (int i = 0; i < 3 && app_.screen() != Screen::Ready; ++i) app_.onKey(Key::AC);
}

void Device::onReply(int id, const std::string& replyJson) {
  app_.onReply(id, replyJson);
  pullScan();
}

void Device::pullScan() {
  const std::string e = app_.takeExpression();
  if (e.empty()) return;
  std::vector<Tok> toks;
  if (!parseExpression(e, toks)) return;  // still shown as text on the AI screen
  expr_ = toks;
  cursor_ = static_cast<int>(expr_.size());
  showingResult_ = false;
  label_.clear();
  historyPos_ = -1;
  scanNote_ = app_.lastConfidence() < App::kUnclearThreshold ? ScanNote::Unclear : ScanNote::Ok;
}

void Device::clearEntry() {
  expr_.clear();
  cursor_ = 0;
  showingResult_ = false;
  label_.clear();
  historyPos_ = -1;
  scanNote_ = ScanNote::None;
  sto_ = rcl_ = false;
  eng_ = false;
}

void Device::insert(Tok t) {
  if (showingResult_) {
    expr_.clear();
    if (continuesAns(t)) expr_.push_back(continueWith_);
    cursor_ = static_cast<int>(expr_.size());
    showingResult_ = false;
    label_.clear();
    historyPos_ = -1;
  }
  if (!insertMode_ && cursor_ < static_cast<int>(expr_.size())) {
    expr_[static_cast<size_t>(cursor_)] = t;
    ++cursor_;
    return;
  }
  if (static_cast<int>(expr_.size()) >= kMaxTokens) return;  // full, like the Casio's 99 steps
  expr_.insert(expr_.begin() + cursor_, t);
  ++cursor_;
}

void Device::showError(CalcError e, int pos) {
  error_ = e;
  errorPos_ = pos;
  view_ = View::Error;
}

void Device::equals() {
  if (expr_.empty()) return;
  const EvalResult r = evaluate(expr_, vars_, angle_, random_);
  if (r.error != CalcError::None) return showError(r.error, r.errorPos);
  vars_.preAns = vars_.ans;
  vars_.ans = r.value;
  history_.push_back({expr_, r.value});
  if (history_.size() > 20) history_.erase(history_.begin());
  historyPos_ = -1;
  showValue(r.value, "");
  scanNote_ = ScanNote::None;
}

void Device::showValue(const Num& v, const std::string& label) {
  result_ = v;
  label_ = label;
  showingResult_ = true;
  showFraction_ = true;
  eng_ = false;
  continueWith_ = Tok::Ans;
}

bool Device::valueForMemory(Num& out) {
  if (showingResult_) {
    out = result_;
    return true;
  }
  if (expr_.empty()) return false;
  const EvalResult r = evaluate(expr_, vars_, angle_, random_);
  if (r.error != CalcError::None) {
    showError(r.error, r.errorPos);
    return false;
  }
  out = r.value;
  return true;
}

void Device::keyCalc(DKey k, bool shift, bool alpha) {
  // STO / RCL wait for a variable key; any other key cancels them.
  if (sto_ || rcl_) {
    Tok var;
    const bool store = sto_;
    sto_ = rcl_ = false;
    if (!varOf(k, var)) return;
    const std::string name = tokText(var);
    if (store) {
      Num v;
      if (!valueForMemory(v)) return;
      vars_.get(var) = v;
      vars_.ans = v;
      showValue(v, "Ans\xE2\x86\x92" + name);  // Ans→A
    } else {
      showValue(vars_.get(var), name + "=");
      continueWith_ = var;  // "A=" then + gives "A+"
    }
    return;
  }

  Tok t;
  if (tokOf(k, shift, alpha, t)) return insert(t);
  if (alpha) return;

  const int n = static_cast<int>(expr_.size());
  switch (k) {
    case DKey::Eq:
      if (showingResult_ && !label_.empty()) return;  // after STO / RCL there's nothing to redo
      return equals();
    case DKey::AC:
      return clearEntry();
    case DKey::Del:
      if (shift) {  // INS: insert / overwrite
        insertMode_ = !insertMode_;
        return;
      }
      if (showingResult_) {
        if (!label_.empty()) return clearEntry();
        showingResult_ = false;
        cursor_ = n;
      }
      if (insertMode_ && cursor_ > 0) {
        expr_.erase(expr_.begin() + cursor_ - 1);
        --cursor_;
      } else if (!insertMode_ && cursor_ < n) {
        expr_.erase(expr_.begin() + cursor_);
      } else if (!insertMode_ && cursor_ > 0) {
        expr_.pop_back();
        --cursor_;
      }
      return;
    case DKey::Left:
    case DKey::Right:
      if (showingResult_) {
        if (!label_.empty()) return clearEntry();
        showingResult_ = false;
        cursor_ = k == DKey::Left ? n : 0;
        return;
      }
      if (n == 0) return;
      if (k == DKey::Left) cursor_ = cursor_ == 0 ? n : cursor_ - 1;  // wraps, as on the Casio
      else cursor_ = cursor_ == n ? 0 : cursor_ + 1;
      return;
    case DKey::Up:
    case DKey::Down: {
      // Replay: browse earlier calculations and their results.
      if (history_.empty() || (!expr_.empty() && !showingResult_)) return;
      const int last = static_cast<int>(history_.size()) - 1;
      int pos;
      if (historyPos_ >= 0) pos = historyPos_ + (k == DKey::Up ? -1 : 1);
      else if (k == DKey::Down) return;
      else pos = showingResult_ && label_.empty() ? last - 1 : last;
      if (pos < 0 || pos > last) return;
      historyPos_ = pos;
      expr_ = history_[static_cast<size_t>(pos)].expr;
      cursor_ = static_cast<int>(expr_.size());
      const Num v = history_[static_cast<size_t>(pos)].value;
      showValue(v, "");
      return;
    }
    case DKey::SD:  // S⇔D: fraction <-> decimal
      if (showingResult_) {
        showFraction_ = !showFraction_;
        eng_ = false;
      }
      return;
    case DKey::Eng:  // ENG: exponent in steps of 3; SHIFT ENG goes the other way
      if (!showingResult_ || result_.v == 0) return;
      if (!eng_) {
        const int e = static_cast<int>(std::floor(std::log10(std::fabs(result_.v))));
        engExp_ = (e >= 0 ? e / 3 : -((-e + 2) / 3)) * 3;
        if (shift) engExp_ += 3;
        eng_ = true;
      } else {
        engExp_ += shift ? 3 : -3;
      }
      return;
    case DKey::Rcl:
      if (shift) sto_ = true;
      else rcl_ = true;
      return;
    case DKey::MPlus: {  // M+ adds the result (or the evaluated expression) to M; SHIFT: M−
      Num v;
      if (!valueForMemory(v)) return;
      Vars tmp = vars_;
      tmp.ans = v;
      const EvalResult r = evaluate({Tok::VarM, shift ? Tok::Sub : Tok::Add, Tok::Ans}, tmp, angle_, random_);
      if (r.error != CalcError::None) return showError(r.error, 0);
      vars_.m = r.value;
      vars_.ans = v;
      showValue(v, "");
      return;
    }
    case DKey::Hyp:
      return openMenu(View::Hyp);
    case DKey::Calc:      // fx-115ES CALC / SHIFT SOLVE and
    case DKey::Integral:  // ∫dx / SHIFT d/dx: not implemented yet, so say so instead of doing something else
      back_ = View::Calc;
      return notice({k == DKey::Calc ? (shift ? "SOLVE" : "CALC") : (shift ? "d/dx" : "â«" "dx"), "",
                     "Not supported yet", "", "Press [AC] key"});
    case DKey::D9:
      if (shift) return openMenu(View::Clr);
      return;
    default:
      return;  // Pol, °'" and the other SHIFT functions of this model: later
  }
}

// ---------------------------------------------------------------- drawing

std::string Device::resultText() const {
  if (!showingResult_) return "";
  if (eng_) return engText();
  return formatResult(result_, showFraction_, norm_);
}

std::string Device::engText() const {
  const double m = result_.v / std::pow(10.0, engExp_);
  const int intDigits = std::fabs(m) >= 1 ? static_cast<int>(std::floor(std::log10(std::fabs(m)))) + 1 : 1;
  int decimals = 10 - intDigits;
  if (decimals < 0) decimals = 0;
  char b[64];
  std::snprintf(b, sizeof b, "%.*f", decimals, std::fabs(m));
  std::string s = b;
  if (s.find('.') != std::string::npos) {
    while (s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
  }
  static const char* sup[] = {"\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3", "\xE2\x81\xB4",
                              "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7", "\xE2\x81\xB8", "\xE2\x81\xB9"};
  s += "\xC3\x97" "10";
  if (engExp_ < 0) s += "\xE2\x81\xBB";
  for (char c : std::to_string(engExp_ < 0 ? -engExp_ : engExp_)) s += sup[c - '0'];
  return (m < 0 ? std::string("\xE2\x88\x92") : std::string()) + s;
}

void Device::drawStatus(Framebuffer& fb) const {
  if (shift_) fb.drawText(0, 0, "S");
  if (alpha_) fb.drawText(1, 0, "A");
  if (vars_.m.v != 0) fb.drawText(3, 0, "M");
  if (sto_) fb.drawText(5, 0, "STO");
  if (rcl_) fb.drawText(5, 0, "RCL");
  fb.drawText(9, 0, angle_ == AngleUnit::Deg ? "D" : angle_ == AngleUnit::Rad ? "R" : "G");
  if (exam_) {
    fb.drawText(12, 0, "\xEE\x80\x80" "EXAM " + clockText(examMs_));  // padlock
  } else {
    if (online_) fb.drawText(18, 0, "\xEE\x80\x81");  // Wi-Fi
    if (mode_ == Mode::Ai) fb.drawText(19, 0, "AI");
  }
  bool up = false, down = false;
  if (view_ == View::Ai) {
    static Framebuffer tmp;  // 7.6 KB: too big for the ESP32's 8 KB loop stack
    tmp.clear();
    app_.render(tmp);
    up = tmp.icon(Icon::Up);
    down = tmp.icon(Icon::Down);
  } else if (view_ == View::Calc && showingResult_ && label_.empty() && !history_.empty()) {
    const int last = static_cast<int>(history_.size()) - 1;
    const int pos = historyPos_ < 0 ? last : historyPos_;
    up = pos > 0;
    down = pos < last;
  }
  if (battery_ >= 0 && (battery_ <= 20 || charging_)) {  // outline, tip, what's left
    fb.fillRect(105, 1, 7, 1, true);
    fb.fillRect(105, 5, 7, 1, true);
    fb.fillRect(105, 1, 1, 5, true);
    fb.fillRect(111, 1, 1, 5, true);
    fb.fillRect(112, 2, 1, 3, true);
    fb.fillRect(106, 2, (battery_ > 100 ? 100 : battery_) * 5 / 100, 3, true);
  }
  if (up) fb.drawText(23, 0, "\xE2\x96\xB2");
  if (down) fb.drawText(24, 0, "\xE2\x96\xBC");
  if (exam_) fb.invertRect(0, 0, Framebuffer::kWidth, 8);  // black bar: easy to see from across a room
}

void Device::drawCalc(Framebuffer& fb) const {
  // Lay the expression out as characters, noting where the cursor falls.
  std::vector<uint32_t> cps;
  int cursorCell = 0;
  for (size_t i = 0; i <= expr_.size(); ++i) {
    if (static_cast<int>(i) == cursor_) cursorCell = static_cast<int>(cps.size());
    if (i == expr_.size()) break;
    for (uint32_t cp : decodeUtf8(tokText(expr_[i]))) cps.push_back(cp);
  }
  const bool editing = !showingResult_;
  if (showingResult_ && !label_.empty()) {
    fb.drawText(0, 1, label_);
  } else {
    const int lines = static_cast<int>(cps.size()) / kCols + 1;
    const int cursorLine = editing ? cursorCell / kCols : 0;
    int first = cursorLine - (kExprRows - 1);
    if (first < 0) first = 0;
    for (int l = first; l < lines && l < first + kExprRows; ++l) {
      const size_t a = static_cast<size_t>(l * kCols);
      if (a >= cps.size()) break;
      const size_t b = a + kCols < cps.size() ? a + kCols : cps.size();
      fb.drawText(0, 1 + l - first, encodeUtf8(std::vector<uint32_t>(cps.begin() + a, cps.begin() + b)));
    }
    if (editing && (!blink_ || ((nowMs_ - lastKeyMs_) / 500) % 2 == 0)) {
      const int col = cursorCell % kCols, row = 1 + cursorCell / kCols - first;
      if (insertMode_) fb.fillRect(col == 0 ? 0 : col * 5 - 1, row * 8, 1, 7, true);  // "|"
      else fb.fillRect(col * 5, row * 8 + 7, 4, 1, true);                              // "_"
    }
  }
  if (showingResult_) {
    const std::string r = resultText();
    const int n = textLength(r);
    if (n * 10 <= Framebuffer::kWidth) fb.drawTextPx(Framebuffer::kWidth - n * 10 + 1, 46, r, 2);
    else fb.drawTextPx(Framebuffer::kWidth - n * 5 + 1, 52, r, 1);
  } else if (scanNote_ == ScanNote::Ok) {
    fb.drawText(0, 6, "Scanned: check, press =");
  } else if (scanNote_ == ScanNote::Unclear) {
    fb.drawText(0, 6, "! UNCLEAR SCAN: CHECK IT");
    fb.invertRect(0, 47, Framebuffer::kWidth, 9);
  }
}

void Device::drawMenu(Framebuffer& fb) const {
  auto mark = [&](int col, int row, int len) { fb.invertRect(col * 5 - 1, row * 8 - 1, len * 5 + 1, 9); };
  switch (view_) {
    case View::ModeMenu:
      fb.drawText(0, 1, two("1:COMP", "2:STAT"));
      fb.drawText(0, 2, two("3:TABLE", "4:AI SOLVE"));
      if (exam_) fb.drawText(0, 4, "(AI is off: exam mode)");
      break;
    case View::Setup:
      fb.drawText(0, 1, two("1:Deg", "2:Rad"));
      fb.drawText(0, 2, two("3:Gra", "4:Norm1"));
      fb.drawText(0, 3, two("5:Norm2", "6:Wi-Fi & key"));
      fb.drawText(0, 4, "7:Screen look");
      fb.drawText(0, 5, "8:Exam mode");
      if (angle_ == AngleUnit::Deg) mark(2, 1, 3);
      if (angle_ == AngleUnit::Rad) mark(13, 1, 3);
      if (angle_ == AngleUnit::Gra) mark(2, 2, 3);
      if (norm_ == NormMode::Norm1) mark(13, 2, 5);
      if (norm_ == NormMode::Norm2) mark(2, 3, 5);
      break;
    case View::Look:
      fb.drawText(0, 1, "SCREEN LOOK", true);
      fb.drawText(0, 2, two("1:Plain", "2:LCD dots"));
      if (dotMatrix_) mark(13, 2, 8);
      else mark(2, 2, 5);
      fb.drawText(0, 6, "AC:back");
      break;
    case View::NetInfo:
      fb.drawText(0, 1, "WI-FI & KEY", true);
      if (exam_) {
        fb.drawText(0, 2, "Off in exam mode.");
      } else {
        fb.drawText(0, 2, "Wi-Fi: " + (ssid_.empty() ? std::string("not set") : ssid_));
        fb.drawText(0, 3, online_ ? "Connected" : "Not connected");
        fb.drawText(0, 4, "Key: " + keyHint_);
        fb.drawText(0, 5, howToChange_);
      }
      fb.drawText(0, 6, "AC:back");
      break;
    case View::ExamInfo:
      if (exam_) {
        fb.drawText(0, 1, "EXAM MODE IS ON", true);
        fb.drawText(0, 2, "On for " + clockText(examMs_));
        fb.drawText(0, 3, "Unlock: USB cable, or");
        fb.drawText(0, 4, "it ends in " + clockText(kExamMaxMs - examMs_));
        fb.drawText(0, 6, "AC:back");
      } else {
        fb.drawText(0, 1, "EXAM MODE", true);
        fb.drawText(0, 2, "Turns off AI, camera and");
        fb.drawText(0, 3, "Wi-Fi until a USB cable");
        fb.drawText(0, 4, "unlocks it or 12 h pass.");
        fb.drawText(0, 6, two("=:Start", "AC:Cancel"));
      }
      break;
    case View::Hyp:
      fb.drawText(0, 1, two("1:sinh", "2:cosh"));
      fb.drawText(0, 2, two("3:tanh", "4:sinh\xE2\x81\xBB\xC2\xB9"));
      fb.drawText(0, 3, two("5:cosh\xE2\x81\xBB\xC2\xB9", "6:tanh\xE2\x81\xBB\xC2\xB9"));
      break;
    case View::Clr:
      fb.drawText(0, 1, "CLR", true);
      fb.drawText(0, 2, two("1:Setup", "2:Memory"));
      fb.drawText(0, 3, "3:All");
      break;
    case View::ClrConfirm:
      fb.drawText(0, 1, clrChoice_ == 1 ? "Reset Setup?" : clrChoice_ == 2 ? "Clear Memory?" : "Reset All?");
      fb.drawText(0, 3, "[=]  :Yes");
      fb.drawText(0, 4, "[AC] :Cancel");
      break;
    case View::Error:
      fb.drawText(2, 1, errorText(error_));
      fb.drawText(2, 3, "[AC]  :Cancel");
      fb.drawText(2, 4, "[\xE2\x97\x80][\xE2\x96\xB6]:Goto");
      break;
    case View::Notice:
      for (size_t i = 0; i < notice_.size() && i < 6; ++i)
        fb.drawText(0, 1 + static_cast<int>(i), notice_[i], i == 0);
      break;
    default:
      break;
  }
}

void Device::render(Framebuffer& fb) const {
  fb.clear();
  if (view_ == View::Off) {
    // Off is a blank screen like the Casio's, except with a cable in: say that it is
    // charging (there is no charge LED on the board). battery_ < 0 = no battery (simulators).
    if (battery_ >= 0 && charging_) {
      fb.drawText(5, 3, "Charging " + std::to_string(battery_ > 100 ? 100 : battery_) + "%");
      fb.drawText(3, 4, "ON: use it meanwhile");
    }
    return;
  }
  drawStatus(fb);
  switch (view_) {
    case View::Calc: drawCalc(fb); break;
    case View::Ai: app_.render(fb); break;
    default: drawMenu(fb); break;
  }
}

}  // namespace calc

// ---------------------------------------------------------------- state across deep sleep

namespace calc {

namespace {
constexpr uint8_t kStateVersion = 1;

void putU8(std::string& o, uint8_t v) { o += static_cast<char>(v); }
void putU32(std::string& o, uint32_t v) {
  for (int i = 0; i < 4; ++i) o += static_cast<char>((v >> (8 * i)) & 0xFF);
}
void putU64(std::string& o, uint64_t v) {
  for (int i = 0; i < 8; ++i) o += static_cast<char>((v >> (8 * i)) & 0xFF);
}
void putNum(std::string& o, const Num& n) {
  uint64_t bits;
  std::memcpy(&bits, &n.v, sizeof bits);
  putU64(o, bits);
  putU8(o, n.exact ? 1 : 0);
  putU64(o, static_cast<uint64_t>(n.num));
  putU64(o, static_cast<uint64_t>(n.den));
}

struct Reader {
  const std::string& s;
  size_t pos = 0;
  bool ok = true;
  uint8_t u8() {
    if (pos + 1 > s.size()) return ok = false, 0;
    return static_cast<uint8_t>(s[pos++]);
  }
  uint32_t u32() {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= uint32_t(u8()) << (8 * i);
    return v;
  }
  uint64_t u64() {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= uint64_t(u8()) << (8 * i);
    return v;
  }
  Num num() {
    Num n;
    const uint64_t bits = u64();
    std::memcpy(&n.v, &bits, sizeof bits);
    n.exact = u8() != 0;
    n.num = static_cast<long long>(u64());
    n.den = static_cast<long long>(u64());
    if (n.den == 0) n.exact = false, n.den = 1;
    return n;
  }
};

Num* varSlots(Vars& v, int i) {
  Num* slots[] = {&v.a, &v.b, &v.c, &v.d, &v.e, &v.f, &v.x, &v.y, &v.m, &v.ans, &v.preAns};
  return slots[i];
}
constexpr int kVarSlots = 11;
}  // namespace

std::string Device::saveState(size_t maxBytes) const {
  std::string o;
  putU8(o, kStateVersion);
  putU8(o, static_cast<uint8_t>(mode_));
  putU8(o, static_cast<uint8_t>(angle_));
  putU8(o, static_cast<uint8_t>(norm_));
  putU8(o, static_cast<uint8_t>((dotMatrix_ ? 1 : 0) | (showFraction_ ? 2 : 0) | (exam_ ? 4 : 0) |
                                (app_.tutor() ? 8 : 0) | (view_ == View::Off ? 16 : 0)));
  putU8(o, static_cast<uint8_t>(app_.effort()));
  putU8(o, static_cast<uint8_t>(offSequence_));
  putU32(o, examMs_);
  Vars v = vars_;
  for (int i = 0; i < kVarSlots; ++i) putNum(o, *varSlots(v, i));
  // Newest history first, as many as fit (the Casio keeps its replay too).
  std::string hist;
  int count = 0;
  for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
    std::string item;
    putU8(item, static_cast<uint8_t>(std::min<size_t>(it->expr.size(), kMaxTokens)));
    for (size_t t = 0; t < it->expr.size() && t < size_t(kMaxTokens); ++t) putU8(item, static_cast<uint8_t>(it->expr[t]));
    putNum(item, it->value);
    if (o.size() + 1 + hist.size() + item.size() > maxBytes) break;
    hist += item;
    ++count;
  }
  putU8(o, static_cast<uint8_t>(count));
  return o + hist;
}

bool Device::restoreState(const std::string& state, uint32_t asleepMs) {
  Reader r{state};
  if (r.u8() != kStateVersion) return false;
  const uint8_t mode = r.u8(), angle = r.u8(), norm = r.u8(), flags = r.u8(), effort = r.u8(), offSeq = r.u8();
  const uint32_t examMs = r.u32();
  Vars v;
  for (int i = 0; i < kVarSlots; ++i) *varSlots(v, i) = r.num();
  const int count = r.u8();
  std::vector<HistoryItem> hist;
  for (int i = 0; i < count && r.ok; ++i) {
    HistoryItem h;
    const int n = r.u8();
    for (int t = 0; t < n; ++t) {
      const uint8_t tok = r.u8();
      if (tok >= static_cast<uint8_t>(Tok::Count)) return false;
      h.expr.push_back(static_cast<Tok>(tok));
    }
    h.value = r.num();
    hist.insert(hist.begin(), h);  // stored newest first
  }
  if (!r.ok || mode > 1 || angle > 2 || norm > 1 || effort > 2) return false;
  mode_ = static_cast<Mode>(mode);
  angle_ = static_cast<AngleUnit>(angle);
  norm_ = static_cast<NormMode>(norm);
  dotMatrix_ = (flags & 1) != 0;
  showFraction_ = (flags & 2) != 0;
  app_.setTutor((flags & 8) != 0);
  app_.setEffort(static_cast<Effort>(effort));
  vars_ = v;
  history_ = hist;
  offSequence_ = offSeq <= 2 ? offSeq : 0;
  exam_ = false;
  examMs_ = 0;
  if (flags & 4) restoreExam(examMs + asleepMs);  // ends itself if 12 h passed while asleep
  if (flags & 16) {
    if (view_ != View::Off) powerOff();
    offSequence_ = offSeq <= 2 ? offSeq : 0;  // powerOff() cleared it
  } else {
    view_ = back_ = (mode_ == Mode::Ai && !exam_) ? View::Ai : View::Calc;
  }
  return true;
}

}  // namespace calc
