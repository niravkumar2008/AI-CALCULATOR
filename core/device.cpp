#include "device.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "font.h"
#include "png.h"
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
    if (k == DKey::Calc) { t = Tok::Eq; return true; }  // ALPHA CALC: "=" (CALC "Y=X+1", SOLVE "X²=2")
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
      {DKey::Integral, Tok::Integ, Tok::Deriv},  // ∫dx: "∫(" ; SHIFT ∫dx: "d/dx("
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
    case Tok::ToPolar: case Tok::ToRect:  // CMPLX: "Ans▶r∠θ"
    case Tok::And: case Tok::Or: case Tok::Xnor: case Tok::Xor:  // BASE-N: "Ans and"
    case Tok::DotP:                                                // VECTOR: "VctAns·"
      return true;
    default:
      return false;
  }
}

// The Apps menu and its apps (closed in exam mode). The calculator views added after them
// (Stat ... SetupFreq) are not apps.
bool isAppView(View v) {
  const int i = static_cast<int>(v);
  return i >= static_cast<int>(View::Apps) && i <= static_cast<int>(View::Link);
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
      {'C', DKey::Calc, DKey::Calc, 1}, {'S', DKey::Shift, DKey::Calc, 2},           // CALC, SOLVE
      {'I', DKey::Integral, DKey::Integral, 1}, {'D', DKey::Shift, DKey::Integral, 2},  // ∫dx, d/dx
      {'Q', DKey::Alpha, DKey::Calc, 2},                                                // "=" in an equation
      {'X', DKey::Alpha, DKey::Close, 2}, {'Y', DKey::Alpha, DKey::SD, 2},              // the variables X, Y
      {'j', DKey::Eng, DKey::Eng, 1}, {'@', DKey::Shift, DKey::Neg, 2},                 // CMPLX: i, ∠
      {'"', DKey::Dms, DKey::Dms, 1},                                                    // °'" (B in BASE-N)
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
  view_ = back_ = modeView();
  app_.onKey(Key::On);
  app_.clearTyped();  // AI SOLVE comes back on its photo home screen
  aiFrom_ = AiFrom::Photo;
  toastOn_ = false;
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
  mode_ = Mode::Comp;  // STAT / TABLE stay: calculator modes, allowed in exams
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;  // any app closes (stored notes and history stay, unreadable until it ends)
  toastOn_ = false;
  online_ = false;
  app_.setOnline(false);
  back_ = modeView();
  infoBack_ = View::Setup;
  notice({"EXAM MODE ON", "AI, camera and Wi-Fi are", "off. Unlock: USB cable,", "or wait 12 hours.", "",
          "Press [AC] key"});  // a calculator message: the Casio's own wording, no key hints
}

void Device::endExam(const char* why) {
  if (!exam_) return;
  exam_ = false;
  examMs_ = 0;
  if (view_ == View::Off) return;
  back_ = modeView();
  notice({"EXAM MODE OFF", why, "AI and Wi-Fi are back.", "", "", "Press [AC] key"});
}

void Device::usbUnlock() { endExam("Unlocked by USB cable."); }

void Device::restoreExam(uint32_t elapsedMs) {
  examExpiredOnRestore_ = elapsedMs >= kExamMaxMs;
  if (examExpiredOnRestore_) return;
  exam_ = true;
  examMs_ = elapsedMs;
  if (mode_ == Mode::Ai) leaveAi();
  mode_ = Mode::Comp;
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  online_ = false;
  app_.setOnline(false);
  const View home = modeView();  // the calculator: COMP, STAT or TABLE
  if (view_ != View::Off && view_ != home) view_ = home;  // no app stays open
  back_ = home;  // nor waits behind a menu (MODE then AC would open it)
  infoBack_ = View::Setup;
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
      // AI SOLVE's home screen, for photos: a typed problem or a saved answer makes way
      // (else = there would resend the typed text instead of taking a photo).
      if (aiFrom_ != AiFrom::Photo || app_.typed() || app_.readOnly()) leaveAi();
      else app_.onKey(Key::AC);
      app_.clearTyped();
      aiFrom_ = AiFrom::Photo;
      mode_ = Mode::Ai;
      back_ = View::Ai;
    } else {
      view_ = modeView();  // a CALC / SOLVE screen closes too
      if (view_ == View::Calc) clearEntry();
      if (view_ == View::Stat || view_ == View::Table || view_ == View::Eqn) {  // like AC there (never leaves the mode)
        syncModes();
        if (view_ == View::Stat) {
          stat_.onKey(DKey::AC, false, false);
          stat_.resume();
        } else if (view_ == View::Table) {
          table_.onKey(DKey::AC, false, false);
          table_.resume();
        } else {
          eqn_.onKey(DKey::AC, false, false);
          eqn_.resume();
        }
      }
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
    if (view_ == View::CalcRun || view_ == View::SolveRun) endRunner();  // MODE ends CALC / SOLVE (as on the Casio)
    setupPage_ = 0;
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
        view_ = backView();
        return;
      }
      switch (pick(1, 9)) {
        case 1:
          if (mode_ == Mode::Ai) leaveAi();
          mode_ = Mode::Comp;
          setCalcMode(Mode::Comp);
          view_ = back_ = View::Calc;
          clearEntry();
          return;
        case 2:  // STAT: the type list, the data cleared (as the Casio does on MODE 2)
          return enterStat();
        case 3:  // TABLE: an empty f(X)=
          return enterTable();
        case 4:
          if (exam_) {
            back_ = modeView();
            return notice({"APPS ARE OFF", "Exam mode is on.", "", "", "", "Press [AC] key"});
          }
          // The Apps menu is outside AI SOLVE: leave it cleanly (a scan in flight is
          // cancelled), so the calculator shows no "AI" and ON goes to the calculator
          // (in its mode: STAT and TABLE keep their data under the apps).
          if (mode_ == Mode::Ai) leaveAi();
          app_.clearTyped();
          aiFrom_ = AiFrom::Photo;
          mode_ = Mode::Comp;
          back_ = modeView();
          view_ = View::Apps;
          return;
        case 5:  // EQN: the type list (a calculator mode: also in exam mode)
          return enterEqn();
        case 6:  // CMPLX: the calculator with complex numbers (also in exam mode)
          return enterCmplx();
        case 7:  // BASE-N: the calculator with integers in DEC / HEX / BIN / OCT (also in exam mode)
          return enterBaseN();
        case 8:  // MATRIX / VECTOR: Dim's list first, as the Casio (also in exam mode)
        case 9:
          return enterMatVec(d == 9);
      }
      return;
    case View::Apps:
      return keyApps(k, d);
    case View::Keyboard:
      return keyKeyboard(k, shift);
    case View::History:
    case View::Notes:
    case View::NoteView:
    case View::Settings:
      return keyList(k, d);
    case View::Confirm:
      return keyConfirm(k);
    case View::Geo:
    case View::Periodic:
      return keyModule(k, shift, alpha);
    case View::Link:
      if (k == DKey::AC || k == DKey::Eq) view_ = infoBack_;
      return;
    case View::Stat:
      return keyStat(k, shift, alpha);
    case View::Table:
      return keyTable(k, shift, alpha);
    case View::Eqn:
      return keyEqn(k, shift, alpha);
    case View::MatVec:
      return keyMatVec(k, shift, alpha);
    case View::CalcRun:
    case View::SolveRun:
      return keyRunner(k, shift, alpha);
    case View::SetupFreq:  // SETUP ▼ 1:STAT: Frequency? 1:ON 2:OFF
      if (k == DKey::AC) {
        view_ = View::Setup;
      } else if (pick(1, 2)) {
        stat_.setFrequency(d == 1);  // a change clears the STAT data, as on the Casio
        setupPage_ = 0;
        syncModes();
        view_ = backView();
      }
      return;
    case View::Setup:
      if (k == DKey::AC) {
        view_ = backView();
        return;
      }
      if (k == DKey::Down || k == DKey::Up) {  // the second page, like the Casio's SETUP ▼
        setupPage_ = k == DKey::Down ? 1 : 0;
        return;
      }
      if (setupPage_ == 1) {
        if (d == 1) view_ = View::SetupFreq;
        if (d == 2) view_ = View::SetupCmplx;
        return;
      }
      // STAT and TABLE get the new unit / Norm at once (syncModes), so the screen they go back
      // to is drawn with it, not only after the next key.
      switch (pick(1, 8)) {
        case 1: angle_ = AngleUnit::Deg; syncModes(); view_ = backView(); return;
        case 2: angle_ = AngleUnit::Rad; syncModes(); view_ = backView(); return;
        case 3: angle_ = AngleUnit::Gra; syncModes(); view_ = backView(); return;
        case 4: norm_ = NormMode::Norm1; syncModes(); view_ = backView(); return;
        case 5: norm_ = NormMode::Norm2; syncModes(); view_ = backView(); return;
        case 6: infoBack_ = View::Setup; view_ = View::NetInfo; return;
        case 7: infoBack_ = View::Setup; view_ = View::Look; return;
        case 8: infoBack_ = View::Setup; view_ = View::ExamInfo; return;
      }
      return;
    case View::Look:  // from SETUP 7 or Settings > Screen look (infoBack_)
      if (k == DKey::AC) {
        view_ = infoBack_;
      } else if (pick(1, 2)) {
        dotMatrix_ = d == 2;
        view_ = infoBack_ == View::Settings ? View::Settings : backView();
      }
      return;
    case View::NetInfo:
      if (k == DKey::AC || k == DKey::Eq) view_ = infoBack_;
      return;
    case View::ExamInfo:
      if (k == DKey::AC) view_ = infoBack_;
      else if (k == DKey::Eq && !exam_) startExam();
      return;
    case View::SetupCmplx:  // SETUP ▼ 2:CMPLX: 1:a+bi 2:r∠θ (how a non-real result shows)
      if (k == DKey::AC) {
        view_ = View::Setup;
      } else if (pick(1, 2)) {
        cmplxPolar_ = d == 2;
        setupPage_ = 0;
        cxShowIm_ = false;
        view_ = backView();
      }
      return;
    case View::CmplxMenu: {  // SHIFT 2 in CMPLX
      static const Tok kCx[] = {Tok::Arg, Tok::Conjg, Tok::ToPolar, Tok::ToRect};
      if (k == DKey::AC) {
        view_ = View::Calc;
      } else if (pick(1, 4)) {
        view_ = View::Calc;
        insert(kCx[d - 1]);
      }
      return;
    }
    case View::BaseMenu: {  // SHIFT 3 in BASE-N: the logic page, ▼ the base prefixes (the Casio's two pages)
      static const Tok kLogic[] = {Tok::And, Tok::Or, Tok::Xnor, Tok::Xor, Tok::Not, Tok::BNeg};
      static const Tok kPrefix[] = {Tok::PreD, Tok::PreH, Tok::PreB, Tok::PreO};
      if (k == DKey::AC) {
        view_ = View::Calc;
      } else if (k == DKey::Down || k == DKey::Up) {
        basePage_ = k == DKey::Down ? 1 : 0;
      } else if (basePage_ == 0 && pick(1, 6)) {
        view_ = View::Calc;
        insert(kLogic[d - 1]);
      } else if (basePage_ == 1 && pick(1, 4)) {
        view_ = View::Calc;
        insert(kPrefix[d - 1]);
      }
      return;
    }
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
          cmplxPolar_ = false;
          syncModes();
        }
        if (clrChoice_ != 1) {  // memory (MATRIX / VECTOR: the variables too)
          vars_ = Vars();
          imag_ = ImagVars();
          history_.clear();
          mv_.reset(mv_.vectors());
        }
        clearEntry();
        back_ = View::Calc;
        notice({clrChoice_ == 1 ? "Reset Setup" : clrChoice_ == 2 ? "Clear Memory" : "Reset All", "",
                "Press [AC] key"});
      }
      return;
    case View::Notice:
      if (k == DKey::AC || k == DKey::Eq) view_ = backView();
      return;
  }
}

void Device::openMenu(View v) {
  // MODE / SETUP / hyp / CLR come back here with AC (an app keeps its place too).
  switch (view_) {
    case View::Calc: case View::Ai: case View::Apps: case View::Keyboard: case View::History:
    case View::Notes: case View::NoteView: case View::Settings: case View::Geo: case View::Periodic:
    case View::Link: case View::Confirm: case View::Error: case View::Stat: case View::Table: case View::Eqn:
    case View::MatVec:
      back_ = view_;
      break;
    default:
      break;
  }
  view_ = v;
}

View Device::backView() const {
  const bool noteOk = noteOpen_ >= 0 && noteOpen_ < static_cast<int>(notes_.size());
  if (exam_ && isAppView(back_)) return modeView();  // no app in exam mode (STAT / TABLE are fine)
  switch (back_) {
    case View::NoteView:
      return noteOk ? back_ : View::Notes;
    case View::Confirm:
      if (ask_ == Ask::DeleteNote && !noteOk) return View::Notes;
      if (ask_ == Ask::ClearHistory && saved_.empty()) return View::History;
      return back_;
    case View::Ai:
      return exam_ ? modeView() : back_;
    case View::Calc:  // the calculator's home: in the mode it is in now
    case View::Stat:
    case View::Table:
    case View::Eqn:
      return mode_ == Mode::Ai ? back_ : modeView();
    default:
      return back_;
  }
}

View Device::modeView() const {
  if (mode_ == Mode::Ai) return View::Ai;
  if (calcMode_ == Mode::Stat) return View::Stat;
  if (calcMode_ == Mode::Table) return View::Table;
  if (calcMode_ == Mode::Eqn) return View::Eqn;
  return View::Calc;
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
      if (aiFrom_ == AiFrom::History) {  // a saved answer: back to the History list
        leaveAi();
        aiFrom_ = AiFrom::Photo;
        mode_ = Mode::Comp;
        view_ = back_ = View::History;
      } else if (app_.screen() == Screen::Result && scanNote_ != ScanNote::None && app_.activeRequest() == 0) {
        app_.onKey(Key::AC);  // the scan waits in the calculator (COMP): go and check it
        app_.clearTyped();
        aiFrom_ = AiFrom::Photo;
        mode_ = Mode::Comp;
        setCalcMode(Mode::Comp);
        view_ = back_ = View::Calc;
      } else if (aiFrom_ == AiFrom::Typed) {  // a typed problem: back to its text, to edit or resend
        leaveAi();
        app_.clearTyped();
        aiFrom_ = AiFrom::Photo;
        mode_ = Mode::Comp;
        kbForNote_ = false;
        kbText_ = typedDraft_;
        kbOriginal_.clear();
        view_ = back_ = View::Keyboard;
      } else if (app_.screen() == Screen::Ready) {  // AC on the AI home screen: back to the Apps menu
        mode_ = Mode::Comp;
        back_ = View::Calc;
        view_ = View::Apps;
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
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Ai;
  view_ = back_ = View::Ai;
  app_.onKey(Key::Eq);
  return app_.activeRequest() != 0;
}

void Device::leaveAi() {
  for (int i = 0; i < 3 && app_.screen() != Screen::Ready; ++i) app_.onKey(Key::AC);
}

void Device::onReply(int id, const std::string& replyJson) {
  const bool live = id != 0 && id == app_.activeRequest();
  app_.onReply(id, replyJson);
  if (live && app_.screen() == Screen::Result) recordAnswer(replyJson);  // History keeps it
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
  if (cmplx()) return equalsCmplx();
  if (baseN()) return equalsBaseN();
  if (isMatVec()) return equalsMatVec();
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
  resultIm_ = Num::ofInt(0);
  resultForm_ = -1;
  cxShowIm_ = false;
  label_ = label;
  showingResult_ = true;
  showFraction_ = true;
  eng_ = false;
  continueWith_ = Tok::Ans;
}

bool Device::valueForMemory(Num& out) {
  if (matrixResultShown()) {  // a matrix / vector can't go into a memory
    showError(CalcError::Syntax, 0);
    return false;
  }
  if (showingResult_) {
    out = result_;
    return true;
  }
  if (expr_.empty()) return false;
  if (baseN()) {  // BASE-N: an integer in the current base's range
    const BaseResult b = evaluateBaseN(expr_, vars_, base_);
    if (b.error != CalcError::None) {
      showError(b.error, b.errorPos);
      return false;
    }
    out = Num::ofInt(b.value);
    return true;
  }
  if (isMatVec()) {  // MATRIX / VECTOR: a scalar (det(MatA), VctA·VctB ...)
    const MvResult m = evaluateMatVec(expr_, vars_, mv_.vars(), calcMode_ == Mode::Vector, angle_, random_);
    if (m.error != CalcError::None || m.kind != MvKind::Scalar) {
      showError(m.error != CalcError::None ? m.error : CalcError::Syntax, m.error != CalcError::None ? m.errorPos : 0);
      return false;
    }
    out = m.scalar;
    return true;
  }
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
    if (cmplx()) {  // the memories hold complex values in CMPLX
      if (store) {
        Cx v;
        if (!valueForMemoryCx(v)) return;
        vars_.get(var) = v.re;
        imag_.get(var) = v.im;
        vars_.ans = v.re;
        imag_.ans = v.im;
        showCx(v, -1, "Ans\xE2\x86\x92" + name);
      } else {
        Cx v;
        v.re = vars_.get(var);
        v.im = imag_.get(var);
        showCx(v, -1, name + "=");
        continueWith_ = var;
      }
      return;
    }
    if (store) {
      Num v;
      if (!valueForMemory(v)) return;
      vars_.get(var) = v;
      vars_.ans = v;
      showValue(v, "Ans\xE2\x86\x92" + name);  // Ans→A
    } else if (baseN()) {  // BASE-N shows the memory's integer part, if it fits the base
      const BaseResult b = evaluateBaseN({var}, vars_, base_);
      if (b.error != CalcError::None) return showError(b.error, 0);
      showValue(Num::ofInt(b.value), name + "=");
      continueWith_ = var;
    } else {
      showValue(vars_.get(var), name + "=");
      continueWith_ = var;  // "A=" then + gives "A+"
    }
    return;
  }

  if (cmplx() && !alpha) {  // MODE 6's own keys (the fx-115ES: ENG is i, SHIFT (−) is ∠, SHIFT 2 the menu)
    if (k == DKey::Eng) return shift ? void() : insert(Tok::ImagI);
    if (k == DKey::Neg && shift) return insert(Tok::Angle);
    if (k == DKey::D2 && shift) return openMenu(View::CmplxMenu);
    if (k == DKey::Integral || k == DKey::Calc) return;  // ∫dx, d/dx, CALC, SOLVE: real only
  }
  if (isMatVec() && !alpha) {  // MODE 8 / 9's own keys: SHIFT 4 (MATRIX) / SHIFT 5 (VECTOR) the menu
    if (shift && k == (calcMode_ == Mode::Vector ? DKey::D5 : DKey::D4)) {
      mv_.openMenu();
      return openMenu(View::MatVec);
    }
    if (k == DKey::Integral || k == DKey::Calc) return;  // ∫dx, d/dx, CALC, SOLVE: COMP's
  }
  if (baseN()) {  // MODE 7's own keys (the fx-115ES's DEC HEX BIN OCT and A-F legends, SHIFT 3 BASE)
    if (!alpha && !shift) {
      switch (k) {
        case DKey::Sq: return setBase(Base::Dec);
        case DKey::Pow: return setBase(Base::Hex);
        case DKey::Log: return setBase(Base::Bin);
        case DKey::Ln: return setBase(Base::Oct);
        case DKey::Neg: return insert(Tok::HexA);
        case DKey::Dms: return insert(Tok::HexB);
        case DKey::Hyp: return insert(Tok::HexC);
        case DKey::Sin: return insert(Tok::HexD);
        case DKey::Cos: return insert(Tok::HexE);
        case DKey::Tan: return insert(Tok::HexF);
        default: break;
      }
    }
    if (shift && k == DKey::D3) {
      basePage_ = 0;
      return openMenu(View::BaseMenu);
    }
    if (!baseNKey(k, shift, alpha)) return;
  }
  Tok t;
  if (tokOf(k, shift, alpha, t)) return insert(t);
  // ALPHA then a key with no letter on it: ALPHA is dropped and the key does its
  // usual job (ALPHA AC still clears, ALPHA = still evaluates).
  if (alpha && k != DKey::AC && k != DKey::Del && k != DKey::Eq && k != DKey::Left && k != DKey::Right &&
      k != DKey::Up && k != DKey::Down)
    return;

  const int n = static_cast<int>(expr_.size());
  switch (k) {
    case DKey::Eq:
      if (shift && complexSplit()) {  // SHIFT = (Re⇔Im): the other half of a long complex result
        cxShowIm_ = !cxShowIm_;
        return;
      }
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
      // Replay: browse earlier calculations and their results. SHIFT ▲ ▼ is the
      // platform's (screen brightness on the v15): here it only clears SHIFT.
      if (shift) return;
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
      const HistoryItem& h = history_[static_cast<size_t>(pos)];
      if (cmplx()) {
        Cx v;
        v.re = h.value;
        v.im = h.im;
        showCx(v, h.form, "");
      } else {
        showValue(h.value, "");
        if (h.form == 2) {  // MATRIX / VECTOR: a matrix result (= shows it again)
          resultForm_ = 2;
          continueWith_ = calcMode_ == Mode::Vector ? Tok::VctAns : Tok::MatAns;
        }
        // BASE-N: a value from before a change to BIN that its 16 bits can't show
        if (baseN() && !baseFits(static_cast<int64_t>(h.value.v), base_)) showError(CalcError::Math, 0);
      }
      return;
    }
    case DKey::SD:  // S⇔D: fraction <-> decimal
      if (showingResult_) {
        showFraction_ = !showFraction_;
        eng_ = false;
      }
      return;
    case DKey::Eng:  // ENG: exponent in steps of 3; SHIFT ENG goes the other way
      if (!showingResult_ || result_.v == 0 || !std::isfinite(result_.v)) return;
      {
        const int e = static_cast<int>(std::floor(std::log10(std::fabs(result_.v))));
        int next;
        if (!eng_) next = (e >= 0 ? e / 3 : -((-e + 2) / 3)) * 3 + (shift ? 3 : 0);
        else next = engExp_ + (shift ? 3 : -3);
        // As on the Casio, it stops where the mantissa would no longer fit the 10-digit
        // display (engText()): at most 10 digits before the point (me <= 9), and a first
        // digit no further than the 9th after it (me >= -9). Before: wrong digits, then inf.
        const int me = e - next;  // the mantissa's own power of ten
        if (me > 9 || me < -9) return;
        engExp_ = next;
        eng_ = true;
      }
      return;
    case DKey::Rcl:
      if (shift) sto_ = true;
      else rcl_ = true;
      return;
    case DKey::MPlus: {  // M+ adds the result (or the evaluated expression) to M; SHIFT: M−
      if (cmplx()) {  // M holds a complex value in CMPLX
        Cx v;
        if (!valueForMemoryCx(v)) return;
        ImagVars im = imag_;
        Vars tmp = vars_;
        tmp.ans = v.re;
        im.ans = v.im;
        const CxResult r =
            evaluateComplex({Tok::VarM, shift ? Tok::Sub : Tok::Add, Tok::Ans}, tmp, im, angle_, random_);
        if (r.error != CalcError::None) return showError(r.error, 0);
        vars_.m = r.value.re;
        imag_.m = r.value.im;
        vars_.ans = v.re;
        imag_.ans = v.im;
        showCx(v, -1, "");
        return;
      }
      Num v;
      if (!valueForMemory(v)) return;
      Vars tmp = vars_;
      tmp.ans = v;
      if (baseN()) {  // BASE-N: M ± the value in integers (Math ERROR outside the base's range)
        const BaseResult b = evaluateBaseN({Tok::VarM, shift ? Tok::Sub : Tok::Add, Tok::Ans}, tmp, base_);
        if (b.error != CalcError::None) return showError(b.error, 0);
        vars_.m = Num::ofInt(b.value);
        vars_.ans = v;
        showValue(v, "");
        return;
      }
      const EvalResult r = evaluate({Tok::VarM, shift ? Tok::Sub : Tok::Add, Tok::Ans}, tmp, angle_, random_);
      if (r.error != CalcError::None) return showError(r.error, 0);
      vars_.m = r.value;
      vars_.ans = v;
      showValue(v, "");
      return;
    }
    case DKey::Hyp:
      return openMenu(View::Hyp);
    case DKey::Calc:  // CALC; SHIFT CALC = SOLVE (∫dx / d/dx type tokens: tokOf())
      return startRunner(shift);
    case DKey::D9:
      if (shift) return openMenu(View::Clr);
      return;
    default:
      return;  // Pol, °'" and the other SHIFT functions of this model: later
  }
}

// ---------------------------------------------------------------- apps (MODE 4)

namespace {

// The Apps menu, in carousel order. Adding an app: a row here and a case in
// Device::openApp() (and its view's keys and drawing).
struct AppInfo {
  const char* name;  // shown in the carousel's title bar
  const char* description;
  View view;
};
const AppInfo kApps[] = {
    {"AI SOLVE", "Photo a problem, solve it", View::Ai},
    {"TYPE IT", "Type a problem, solve it", View::Keyboard},
    {"HISTORY", "Your last 20 answers", View::History},
    {"NOTES", "Up to 10 short notes", View::Notes},
    {"GEOGRAPHY", "Countries and capitals", View::Geo},
    {"PERIODIC TABLE", "The elements", View::Periodic},
    {"SETTINGS", "Screen, Wi-Fi, battery", View::Settings},
};
static_assert(sizeof(kApps) / sizeof(kApps[0]) == Device::kAppCount, "one row per app");

// Settings, in list order (openSetting() and the drawing follow it).
const char* const kSettings[] = {"Brightness", "Sleep timer", "Wi-Fi & key", "Link calculator",
                                 "Screen look", "Exam mode", "Battery", "About"};
constexpr int kSetBattery = 6;  // the rows openSetting() and the drawing treat specially
constexpr int kSettingCount = sizeof(kSettings) / sizeof(kSettings[0]);
const int kSleepChoices[] = {1, 2, 5, 10, 15, 30};
constexpr int kSleepCount = sizeof(kSleepChoices) / sizeof(kSleepChoices[0]);

// The keyboard: row 0 A-Y, row 1 Z, digits and maths, row 2 SPACE, DEL, SEND.
const std::vector<KeyCell>& keyCells() {
  static std::vector<KeyCell> cells;
  if (cells.empty()) {
    static const char* const kRow0[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
                                        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y"};
    static const char* const kRow1[] = {"Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "+", "-",
                                        "\xC3\x97", "\xC3\xB7", "=", "(", ")", "^", "\xE2\x88\x9A",
                                        "\xCF\x80", "x", "y", ".", ","};  // × ÷ √ π
    for (int i = 0; i < 25; ++i) cells.push_back({kRow0[i], kRow0[i], 0, uint8_t(i), 1, KeyCell::Type});
    for (int i = 0; i < 25; ++i) cells.push_back({kRow1[i], kRow1[i], 1, uint8_t(i), 1, KeyCell::Type});
    cells.push_back({"SPACE", " ", 2, 1, 5, KeyCell::Type});
    cells.push_back({"DEL", "", 2, 10, 3, KeyCell::Erase});
    cells.push_back({"SEND", "", 2, 18, 4, KeyCell::Send});
  }
  return cells;
}

// What a calculator key types on the keyboard screen ("" = nothing).
std::string keyText(DKey k, bool shift) {
  if (shift) return k == DKey::Close ? "," : k == DKey::Exp10 ? "\xCF\x80" : "";  // as on the keypad
  const int d = digitOf(k);
  if (d >= 0) return std::string(1, static_cast<char>('0' + d));
  switch (k) {
    case DKey::Add: return "+";
    case DKey::Sub: return "-";
    case DKey::Mul: return "\xC3\x97";
    case DKey::Div: return "\xC3\xB7";
    case DKey::Open: return "(";
    case DKey::Close: return ")";
    case DKey::Pow: return "^";
    case DKey::Sqrt: return "\xE2\x88\x9A";
    case DKey::Dot: return ".";
    default: return "";
  }
}

// At most n characters: longer text ends in "…".
std::string fit(const std::string& s, int n) {
  std::vector<uint32_t> cps = decodeUtf8(s);
  if (static_cast<int>(cps.size()) <= n) return s;
  cps.resize(static_cast<size_t>(n - 1));
  cps.push_back(0x2026);
  return encodeUtf8(cps);
}

// Text cut into lines of exactly kCols characters (the keyboard's text box, where
// the cursor sits after the last character).
std::vector<std::string> hardWrap(const std::string& s) {
  const std::vector<uint32_t> cps = decodeUtf8(s);
  std::vector<std::string> out;
  for (size_t a = 0; a < cps.size(); a += kCols)
    out.push_back(encodeUtf8(std::vector<uint32_t>(cps.begin() + a, cps.begin() + std::min(cps.size(), a + kCols))));
  return out;
}

std::string jsonQuote(const std::string& s) {
  std::string o = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += c;
    else if (static_cast<unsigned char>(c) < 0x20) o += ' ';
    else o += c;
  }
  return o + "\"";
}

constexpr int kNoteLines = 4;  // a note's text rows in View::NoteView

// A reply's short form, for History: just the question and the answer, each cut (at a
// character boundary) until the whole JSON fits in `limit` bytes.
std::string shortReply(const SolveResult& r, size_t limit) {
  for (size_t cut = 1000;; cut /= 2) {
    std::string j = "{\"readable\":true,\"confidence\":1,\"answer\":" + jsonQuote(utf8Prefix(r.answer, cut)) +
                    ",\"choice\":" + jsonQuote(utf8Prefix(r.choice, 16)) +
                    ",\"read_as\":" + jsonQuote(utf8Prefix(r.readAs, cut)) + "}";
    if (j.size() <= limit || cut < 16) return j;
  }
}

// The reply as History keeps it: itself when it fits in `limit` bytes, else its short
// form; "" when it can't be read (then there is nothing to show again).
std::string compactReply(const std::string& json, size_t limit) {
  if (json.size() <= limit) return json;
  SolveResult r;
  std::string err;
  if (!parseSolveResult(json, r, err) || !r.readable) return "";
  return shortReply(r, limit);
}

constexpr size_t kShortSavedJson = 768;  // saveApps(): an older answer in its short form
constexpr size_t kMaxLoadJson = 6000;    // restoreApps(): what older firmware saved per answer
}  // namespace

const char* Device::appName(int i) { return i >= 0 && i < kAppCount ? kApps[i].name : ""; }
const char* Device::appDescription(int i) { return i >= 0 && i < kAppCount ? kApps[i].description : ""; }
View Device::appView(int i) { return i >= 0 && i < kAppCount ? kApps[i].view : View::Off; }
int Device::keyboardCellCount() { return static_cast<int>(keyCells().size()); }
const KeyCell& Device::keyboardCellAt(int i) {
  const auto& c = keyCells();
  return c[static_cast<size_t>(i < 0 ? 0 : i >= static_cast<int>(c.size()) ? c.size() - 1 : i)];
}
int Device::settingsCount() { return kSettingCount; }
const char* Device::settingsItem(int i) { return i >= 0 && i < kSettingCount ? kSettings[i] : ""; }

void Device::toast(const std::string& text) {
  toast_ = fit(text, kCols - 2);
  toastOn_ = true;
  toastSinceMs_ = nowMs_;
}

bool Device::toastVisible() const { return toastOn_ && view_ != View::Off && nowMs_ - toastSinceMs_ < kToastMs; }

void Device::setBrightness(int percent) {
  percent = (percent + 5) / 10 * 10;
  brightness_ = percent < 10 ? 10 : percent > 100 ? 100 : percent;
}

void Device::setSleepMinutes(int minutes) {
  int best = kSleepChoices[0];
  for (int c : kSleepChoices)
    if (std::abs(c - minutes) < std::abs(best - minutes)) best = c;
  sleepMin_ = best;
}

void Device::clearHistory() {
  if (saved_.empty()) return;
  saved_.clear();
  histSel_ = 0;
  ++appsRev_;
}

void Device::recordAnswer(const std::string& replyJson) {
  if (exam_) return;
  const SolveResult& r = app_.result();
  std::string answer = r.answer;
  if (!r.choice.empty() && answer.rfind("(" + r.choice + ")", 0) != 0) answer = "(" + r.choice + ") " + answer;
  const std::string json = compactReply(replyJson, kMaxSavedJson);  // a big one: question and answer only
  if (json.empty()) return;
  const int before = static_cast<int>(saved_.size());
  saved_.insert(saved_.begin(), {utf8Prefix(answer, kMaxSavedAnswer), json});
  if (static_cast<int>(saved_.size()) > kMaxHistory) saved_.pop_back();
  // The list moved down a row under the selection: keep it on the same answer (or on
  // "Clear history"), not on whatever slid under it.
  const int n = static_cast<int>(saved_.size());
  if (before > 0) histSel_ = histSel_ < before ? std::min(histSel_ + 1, n - 1) : n;
  // "Clear history?" was asked about the list before this answer: ask again.
  if (ask_ == Ask::ClearHistory) {
    if (view_ == View::Confirm) view_ = View::History;
    if (back_ == View::Confirm) back_ = View::History;
  }
  ++appsRev_;
}

int Device::listRows() const {
  switch (view_) {
    case View::History: return saved_.empty() ? 0 : static_cast<int>(saved_.size()) + 1;  // + "Clear history"
    case View::Notes: return 1 + static_cast<int>(notes_.size());                         // "+ New note" first
    case View::Settings: return kSettingCount;
    default: return 0;
  }
}

int Device::listTop() const {
  const int sel = view_ == View::History ? histSel_ : view_ == View::Notes ? notesSel_ : view_ == View::Settings ? setSel_ : 0;
  return sel < kListRows ? 0 : sel - (kListRows - 1);  // the selection stays on screen
}

std::string Device::confirmQuestion() const {
  switch (ask_) {
    case Ask::DiscardText: return !kbForNote_ ? "Discard text?" : editNote_ >= 0 ? "Discard changes?" : "Discard note?";
    case Ask::DeleteNote: return "Delete this note?";
    case Ask::ClearHistory: return "Clear history?";
  }
  return "";
}

void Device::toCalc() {
  if (mode_ == Mode::Ai) leaveAi();
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Comp;
  view_ = back_ = modeView();  // the expression (or STAT's data, TABLE's f(X)) stays as it was
}

// ---------------------------------------------------------------- STAT, TABLE, CALC, SOLVE

void Device::syncModes() {
  stat_.setEnv(&vars_, angle_, norm_, random_);
  table_.setEnv(&vars_, angle_, norm_, random_);
  eqn_.setEnv(&vars_, angle_, norm_, random_);
  mv_.setEnv(&vars_, angle_, norm_, random_);
}

void Device::enterStat() {
  if (mode_ == Mode::Ai) leaveAi();
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Comp;
  setCalcMode(Mode::Stat);
  syncModes();
  stat_.reset();
  clearEntry();
  view_ = back_ = View::Stat;
}

void Device::enterTable() {
  if (mode_ == Mode::Ai) leaveAi();
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Comp;
  setCalcMode(Mode::Table);
  syncModes();
  table_.reset();
  clearEntry();
  view_ = back_ = View::Table;
}

void Device::enterEqn() {
  if (mode_ == Mode::Ai) leaveAi();
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Comp;
  setCalcMode(Mode::Eqn);
  syncModes();
  eqn_.reset();
  clearEntry();
  view_ = back_ = View::Eqn;
}

void Device::enterCmplx() {
  if (mode_ == Mode::Ai) leaveAi();
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Comp;
  setCalcMode(Mode::Cmplx);
  clearEntry();
  view_ = back_ = View::Calc;
}

void Device::enterBaseN() {
  if (mode_ == Mode::Ai) leaveAi();
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Comp;
  setCalcMode(Mode::BaseN);
  base_ = Base::Dec;  // "The initial default number mode when you enter the BASE-N Mode is decimal"
  clearEntry();
  view_ = back_ = View::Calc;
}

void Device::enterMatVec(bool vectors) {
  if (mode_ == Mode::Ai) leaveAi();
  app_.clearTyped();
  aiFrom_ = AiFrom::Photo;
  mode_ = Mode::Comp;
  setCalcMode(vectors ? Mode::Vector : Mode::Matrix);
  syncModes();
  mv_.reset(vectors);  // the variables cleared; Dim's "Matrix?" / "Vector?" list
  clearEntry();
  back_ = View::Calc;
  view_ = View::MatVec;
}

bool Device::matrixResultShown() const { return showingResult_ && isMatVec() && resultForm_ == 2; }

void Device::equalsMatVec() {
  const bool vec = calcMode_ == Mode::Vector;
  const MvResult r = evaluateMatVec(expr_, vars_, mv_.vars(), vec, angle_, random_);
  if (r.error != CalcError::None) return showError(r.error, r.errorPos);
  historyPos_ = -1;
  scanNote_ = ScanNote::None;
  if (r.kind == MvKind::Scalar) {  // det(, ·, Abs( of a vector ...: as in COMP, to Ans
    vars_.preAns = vars_.ans;
    vars_.ans = r.scalar;
    history_.push_back({expr_, r.scalar});
    if (history_.size() > 20) history_.erase(history_.begin());
    return showValue(r.scalar, "");
  }
  // A matrix / vector: MatAns (VctAns) and its screen; the calculation screen keeps the
  // expression (no value), and + − × ... after it carry on from MatAns.
  history_.push_back({expr_, Num(), Num(), 2});
  if (history_.size() > 20) history_.erase(history_.begin());
  showValue(Num(), "");
  resultForm_ = 2;
  continueWith_ = vec ? Tok::VctAns : Tok::MatAns;
  mv_.showAns(r.m);
  openMenu(View::MatVec);
}

void Device::keyMatVec(DKey k, bool shift, bool alpha) {
  syncModes();
  const bool onAns = mv_.screen() == MatVecApp::Screen::Editor && mv_.editing() == 3;
  mv_.onKey(k, shift, alpha);
  Tok t;
  const MatVecApp::Action a = mv_.takeAction(t);
  if (a == MatVecApp::Action::None) return;
  view_ = View::Calc;
  if (a != MatVecApp::Action::Insert) return;
  if (onAns && continuesAns(t)) {  // the MatAns screen's + − × ÷ x⁻¹ x² x³: "MatAns" and the key
    expr_ = {calcMode_ == Mode::Vector ? Tok::VctAns : Tok::MatAns, t};
    cursor_ = 2;
    showingResult_ = false;
    label_.clear();
    historyPos_ = -1;
    return;
  }
  insert(t);  // a variable, det( Trn( · from the menu
}

void Device::setCalcMode(Mode m) {
  const bool was = calcMode_ == Mode::Cmplx, now = m == Mode::Cmplx;
  const bool wasN = calcMode_ == Mode::BaseN, nowN = m == Mode::BaseN;
  const bool wasM = isMatVec(), nowM = m == Mode::Matrix || m == Mode::Vector;
  if (was && !now) {  // the Casio keeps only the real parts
    imag_ = ImagVars();
    resultIm_ = Num::ofInt(0);
    resultForm_ = -1;
  }
  if (wasM && m != calcMode_) {  // a matrix result means nothing outside the mode
    if (resultForm_ == 2) showingResult_ = false;
    resultForm_ = -1;
    mv_.reset(m == Mode::Vector);
  }
  if (was != now || wasN != nowN || wasM != nowM || (wasM && m != calcMode_)) {
    // A replay of complex results means nothing in COMP, nor one of integers in another base
    // (and the Casio's is per mode).
    history_.clear();
    historyPos_ = -1;
  }
  if (wasN && !nowN) base_ = Base::Dec;
  calcMode_ = m;
}

void Device::equalsBaseN() {
  const BaseResult r = evaluateBaseN(expr_, vars_, base_);
  if (r.error != CalcError::None) return showError(r.error, r.errorPos);
  const Num v = Num::ofInt(r.value);
  vars_.preAns = vars_.ans;
  vars_.ans = v;
  history_.push_back({expr_, v});
  if (history_.size() > 20) history_.erase(history_.begin());
  historyPos_ = -1;
  showValue(v, "");
  scanNote_ = ScanNote::None;
}

void Device::setBase(Base b) {
  base_ = b;
  // The shown result in the new base; one BIN's 16 bits can't show is a Math ERROR (the
  // expression stays for Goto / AC, as after any error).
  if (showingResult_ && !baseFits(static_cast<int64_t>(result_.v), b)) showError(CalcError::Math, 0);
}

bool Device::baseNKey(DKey k, bool shift, bool alpha) const {
  if (alpha) {  // the memories, and the keys ALPHA leaves alone (AC DEL = and the arrows)
    Tok t;
    return varOf(k, t) || k == DKey::AC || k == DKey::Del || k == DKey::Eq || k == DKey::Left || k == DKey::Right ||
           k == DKey::Up || k == DKey::Down;
  }
  if (digitOf(k) >= 0) return !shift || k == DKey::D9;  // digits; SHIFT 9 CLR (SHIFT 0 Rnd( is not BASE-N's)
  switch (k) {
    case DKey::Add: case DKey::Sub: case DKey::Mul: case DKey::Div: case DKey::Open: case DKey::Close:
    case DKey::Ans: case DKey::Eq:
      return !shift;  // not nPr nCr % , : real functions
    case DKey::AC: case DKey::Del: case DKey::Left: case DKey::Right: case DKey::Up: case DKey::Down:
    case DKey::Rcl: case DKey::MPlus:
      return true;  // SHIFT: INS, STO, M−
    default:
      return false;  // functions, ".", ×10ˣ, S⇔D, ENG, CALC, SOLVE, ∫dx, d/dx, hyp's menu ...
  }
}

void Device::equalsCmplx() {
  const CxResult r = evaluateComplex(expr_, vars_, imag_, angle_, random_);
  if (r.error != CalcError::None) return showError(r.error, r.errorPos);
  vars_.preAns = vars_.ans;
  imag_.preAns = imag_.ans;
  vars_.ans = r.value.re;
  imag_.ans = r.value.im;
  history_.push_back({expr_, r.value.re, r.value.im, static_cast<int8_t>(r.forced)});
  if (history_.size() > 20) history_.erase(history_.begin());
  historyPos_ = -1;
  showCx(r.value, r.forced, "");
  scanNote_ = ScanNote::None;
}

bool Device::valueForMemoryCx(Cx& out) {
  if (showingResult_) {
    out.re = result_;
    out.im = resultIm_;
    return true;
  }
  if (expr_.empty()) return false;
  const CxResult r = evaluateComplex(expr_, vars_, imag_, angle_, random_);
  if (r.error != CalcError::None) {
    showError(r.error, r.errorPos);
    return false;
  }
  out = r.value;
  return true;
}

void Device::showCx(const Cx& v, int form, const std::string& label) {
  showValue(v.re, label);
  resultIm_ = v.im;
  resultForm_ = static_cast<int8_t>(form);
}

int Device::shownForm() const {
  if (resultForm_ >= 0) return resultForm_;      // ▶a+bi / ▶r∠θ at the end
  if (resultIm_.v == 0) return -1;               // a real value shows as in COMP
  return cmplxPolar_ ? 1 : 0;                    // SETUP ▼ 2:CMPLX
}

std::string Device::cxFullText() const {
  Cx z;
  z.re = result_;
  z.im = resultIm_;
  return complexText(z, shownForm() == 1 ? CxForm::Polar : CxForm::Rect, showFraction_, norm_, angle_);
}

bool Device::complexSplit() const {
  return showingResult_ && cmplx() && shownForm() >= 0 && textLength(cxFullText()) > kCols;
}

void Device::keyStat(DKey k, bool shift, bool alpha) {
  syncModes();  // SETUP may have changed the angle unit or Norm mode
  stat_.onKey(k, shift, alpha);
  if (!stat_.wantsExit()) return;
  stat_.resume();  // AC on the type list: back to COMP
  setCalcMode(Mode::Comp);
  view_ = back_ = View::Calc;
  clearEntry();
}

void Device::keyTable(DKey k, bool shift, bool alpha) {
  syncModes();
  table_.onKey(k, shift, alpha);
  if (!table_.wantsExit()) return;
  table_.resume();  // AC on an empty f(X)=: back to COMP
  setCalcMode(Mode::Comp);
  view_ = back_ = View::Calc;
  clearEntry();
}

void Device::keyEqn(DKey k, bool shift, bool alpha) {
  syncModes();
  eqn_.onKey(k, shift, alpha);
  if (!eqn_.wantsExit()) return;
  eqn_.resume();  // AC on the type list before any type was chosen: back to COMP
  setCalcMode(Mode::Comp);
  view_ = back_ = View::Calc;
  clearEntry();
}

void Device::startRunner(bool solve) {
  // CALC / SOLVE run on the expression, also on the one whose result is shown.
  const bool ok = solve ? solveRun_.start(expr_, vars_, angle_, norm_, random_)
                        : calcRun_.start(expr_, vars_, angle_, norm_, random_);
  if (!ok) return;  // an empty line: nothing to do
  showingResult_ = false;
  label_.clear();
  historyPos_ = -1;
  scanNote_ = ScanNote::None;
  view_ = solve ? View::SolveRun : View::CalcRun;
}

void Device::keyRunner(DKey k, bool shift, bool alpha) {
  // A key that types (with SHIFT / ALPHA) goes into the prompt's input; the rest are the runner's.
  numeric::RunnerBase& r = view_ == View::SolveRun ? static_cast<numeric::RunnerBase&>(solveRun_)
                                                   : static_cast<numeric::RunnerBase&>(calcRun_);
  Tok t;
  if (tokOf(k, shift, alpha, t)) r.insert(t);
  else if (view_ == View::SolveRun) solveRun_.onKey(k, shift);
  else calcRun_.onKey(k, shift);
  if (r.wantsExit()) endRunner();
}

void Device::endRunner() {
  const numeric::RunnerBase& r = view_ == View::SolveRun ? static_cast<const numeric::RunnerBase&>(solveRun_)
                                                         : static_cast<const numeric::RunnerBase&>(calcRun_);
  const int n = static_cast<int>(expr_.size());
  int pos = r.exitKind() == numeric::Exit::Goto ? r.gotoPos() : n;
  if (pos < 0) pos = 0;
  if (pos > n) pos = n;
  view_ = View::Calc;
  showingResult_ = false;
  label_.clear();
  historyPos_ = -1;
  cursor_ = pos;
}


void Device::openApp(int i) {
  if (i < 0 || i >= kAppCount) return;
  appsSel_ = i;
  if (exam_) {  // never reached through the menu (MODE 4 refuses first); stored notes stay unreadable
    back_ = View::Calc;
    return notice({"APPS ARE OFF", "Exam mode is on.", "", "", "", "AC:back"});
  }
  switch (kApps[i].view) {
    case View::Ai:  // AI SOLVE, as it was (a typed problem or a saved answer makes way for the camera)
      if (aiFrom_ != AiFrom::Photo || app_.typed() || app_.readOnly()) leaveAi();
      app_.clearTyped();
      aiFrom_ = AiFrom::Photo;
      mode_ = Mode::Ai;
      view_ = back_ = View::Ai;
      return;
    case View::Keyboard:  // Type it: the text typed last time is still there
      if (!typedSolve_) {   // this platform would send a photo instead (setTypedSolveSupported())
        back_ = View::Apps;
        return notice({"TYPE IT", "Not available on this", "model.", "", "", "AC:back"});
      }
      kbForNote_ = false;
      kbText_ = typedDraft_;
      kbOriginal_.clear();
      view_ = View::Keyboard;
      return;
    case View::History:
      histSel_ = 0;
      view_ = View::History;
      return;
    case View::Notes:
      notesSel_ = 0;
      view_ = View::Notes;
      return;
    case View::Geo:  // its own controller (core/geo.*), where it was left last time
      if (!geoStarted_) {
        if (randomSet_ && random_) {  // the quiz's questions; without a source GeoApp's fixed seed
          double r = random_();
          r = r < 0 ? 0 : r >= 1 ? 0.999999 : r;
          geo_.setSeed(static_cast<uint32_t>(r * 4294967296.0));
        }
        geo_.reset();
        geoStarted_ = true;
      }
      geo_.resume();
      view_ = View::Geo;
      return;
    case View::Periodic:  // likewise (core/periodic.*)
      if (!periodicStarted_) {
        periodic_.reset();
        periodicStarted_ = true;
      }
      periodic_.resume();
      view_ = View::Periodic;
      return;
    default:  // Settings
      view_ = kApps[i].view;
      return;
  }
}

void Device::keyModule(DKey k, bool shift, bool alpha) {
  // SHIFT / ALPHA were latched by onKey() (and show in the status bar); the app gets them with the key.
  if (view_ == View::Geo) {
    geo_.onKey(k, shift, alpha);
    if (geo_.wantsExit()) view_ = View::Apps;
  } else {
    periodic_.onKey(k, shift, alpha);
    if (periodic_.wantsExit()) view_ = View::Apps;
  }
}

void Device::setLinkInfo(const std::string& code, const std::string& url, bool linked) {
  linkCode_ = code;
  linkUrl_ = url;
  linked_ = linked;
}

void Device::keyApps(DKey k, int d) {
  if (k == DKey::Left) appsSel_ = (appsSel_ + kAppCount - 1) % kAppCount;
  else if (k == DKey::Right) appsSel_ = (appsSel_ + 1) % kAppCount;
  else if (k == DKey::Eq) openApp(appsSel_);
  else if (d >= 1 && d <= kAppCount) openApp(d - 1);
  else if (k == DKey::AC) toCalc();
}

void Device::typeText(const std::string& s) {
  if (s.empty()) return;
  if (textLength(kbText_) + textLength(s) > kMaxText) return toast("160 characters max");
  kbText_ += s;
  if (!kbForNote_) typedDraft_ = kbText_;
}

void Device::eraseChar() {
  std::vector<uint32_t> cps = decodeUtf8(kbText_);
  if (cps.empty()) return;
  cps.pop_back();
  kbText_ = encodeUtf8(cps);
  if (!kbForNote_) typedDraft_ = kbText_;
}

void Device::pressCell() {
  const KeyCell& c = keyboardCellAt(kbCell_);
  switch (c.kind) {
    case KeyCell::Type: return typeText(c.text);
    case KeyCell::Erase: return eraseChar();
    case KeyCell::Send: return kbForNote_ ? saveNote() : sendTyped();
  }
}

void Device::keyKeyboard(DKey k, bool shift) {
  const auto& cells = keyCells();
  const int n = static_cast<int>(cells.size());
  const KeyCell& cur = cells[static_cast<size_t>(kbCell_)];
  switch (k) {
    case DKey::Left:
    case DKey::Right: {  // along the row, wrapping at its ends
      int i = kbCell_;
      do {
        i = (i + (k == DKey::Left ? n - 1 : 1)) % n;
      } while (cells[static_cast<size_t>(i)].row != cur.row);
      kbCell_ = i;
      return;
    }
    case DKey::Up:
    case DKey::Down: {  // the nearest cell in the row above / below, wrapping top to bottom
      const int row = (cur.row + (k == DKey::Up ? 2 : 1)) % 3;
      const int centre = cur.col * 2 + cur.width;  // in half characters
      int best = -1, bestDist = 1000;
      for (int i = 0; i < n; ++i) {
        const KeyCell& c = cells[static_cast<size_t>(i)];
        const int dist = std::abs(c.col * 2 + c.width - centre);
        if (c.row == row && dist < bestDist) best = i, bestDist = dist;
      }
      kbCell_ = best;
      return;
    }
    case DKey::Eq: return pressCell();
    case DKey::Del: return eraseChar();
    case DKey::AC:
      if (kbText_ != kbOriginal_) return ask(Ask::DiscardText);  // typed something: ask first
      return leaveKeyboard();
    default:
      return typeText(keyText(k, shift));
  }
}

void Device::leaveKeyboard() {
  kbText_.clear();
  if (!kbForNote_) {
    typedDraft_.clear();
    view_ = View::Apps;
  } else if (editNote_ >= 0 && editNote_ < static_cast<int>(notes_.size())) {
    noteOpen_ = editNote_;
    view_ = View::NoteView;
  } else {
    view_ = View::Notes;
  }
  editNote_ = -1;
}

void Device::sendTyped() {
  if (exam_ || !typedSolve_) return;
  const std::string png = renderTextPng(kbText_);
  if (png.empty()) return toast("Type a problem first");  // blank text is never sent
  if (mode_ == Mode::Ai) leaveAi();
  aiFrom_ = AiFrom::Typed;
  mode_ = Mode::Ai;
  view_ = back_ = View::Ai;
  app_.startTyped(png);  // the platform: takeRequest(), then takeTypedImage()
}

void Device::saveNote() {
  if (blankText(kbText_)) return toast("Type a note first");
  if (editNote_ >= 0 && editNote_ < static_cast<int>(notes_.size())) {
    notes_[static_cast<size_t>(editNote_)] = kbText_;
    notesSel_ = editNote_ + 1;
  } else {
    if (static_cast<int>(notes_.size()) >= kMaxNotes) return toast("Notes full");
    notes_.insert(notes_.begin(), kbText_);  // newest first
    notesSel_ = 1;
  }
  ++appsRev_;
  kbText_.clear();
  editNote_ = -1;
  view_ = View::Notes;
  toast("Saved");
}

void Device::ask(Ask a) {
  ask_ = a;
  view_ = View::Confirm;
}

void Device::keyConfirm(DKey k) {
  const bool yes = k == DKey::Eq;
  if (!yes && k != DKey::AC) return;
  switch (ask_) {
    case Ask::DiscardText:
      if (yes) leaveKeyboard();
      else view_ = View::Keyboard;
      return;
    case Ask::DeleteNote:
      if (yes && noteOpen_ >= 0 && noteOpen_ < static_cast<int>(notes_.size())) {
        notes_.erase(notes_.begin() + noteOpen_);
        ++appsRev_;
        noteOpen_ = -1;
        notesSel_ = std::min(notesSel_, static_cast<int>(notes_.size()));
        if (back_ == View::NoteView || back_ == View::Confirm) back_ = View::Notes;  // that note has gone
        view_ = View::Notes;
        toast("Deleted");
      } else {
        view_ = View::NoteView;
      }
      return;
    case Ask::ClearHistory:
      if (yes) {
        clearHistory();
        toast("History cleared");
      }
      view_ = View::History;
      return;
  }
}

void Device::keyList(DKey k, int d) {
  if (view_ == View::NoteView) {
    const int lines = noteOpen_ >= 0 && noteOpen_ < static_cast<int>(notes_.size())
                          ? static_cast<int>(wrapText(notes_[static_cast<size_t>(noteOpen_)], kCols).size())
                          : 0;
    if (k == DKey::AC) {
      view_ = View::Notes;
    } else if (k == DKey::Up && noteTop_ > 0) {
      --noteTop_;
    } else if (k == DKey::Down && noteTop_ + kNoteLines < lines) {
      ++noteTop_;
    } else if (d == 1 && lines > 0) {  // Edit
      kbForNote_ = true;
      editNote_ = noteOpen_;
      kbText_ = kbOriginal_ = notes_[static_cast<size_t>(noteOpen_)];
      view_ = View::Keyboard;
    } else if (d == 2 && lines > 0) {  // Delete, after asking
      ask(Ask::DeleteNote);
    }
    return;
  }
  if (k == DKey::AC) {
    view_ = View::Apps;
    return;
  }
  int& sel = view_ == View::History ? histSel_ : view_ == View::Notes ? notesSel_ : setSel_;
  const int rows = listRows();
  if (k == DKey::Up && sel > 0) --sel;
  else if (k == DKey::Down && sel + 1 < rows) ++sel;
  if (view_ == View::History) {
    if (k != DKey::Eq || rows == 0) return;
    if (sel == rows - 1) return ask(Ask::ClearHistory);
    if (mode_ == Mode::Ai) leaveAi();
    app_.clearTyped();
    if (!app_.showSaved(saved_[static_cast<size_t>(sel)].replyJson)) return toast("Can't show this one");
    aiFrom_ = AiFrom::History;
    mode_ = Mode::Ai;
    view_ = back_ = View::Ai;
  } else if (view_ == View::Notes) {
    if (k != DKey::Eq) return;
    if (sel == 0) {  // + New note
      if (static_cast<int>(notes_.size()) >= kMaxNotes) return toast("Notes full");
      kbForNote_ = true;
      editNote_ = -1;
      kbText_.clear();
      kbOriginal_.clear();
      view_ = View::Keyboard;
    } else {
      noteOpen_ = sel - 1;
      noteTop_ = 0;
      view_ = View::NoteView;
    }
  } else {  // Settings
    if (sel == 0 && (k == DKey::Left || k == DKey::Right)) setBrightness(brightness_ + (k == DKey::Left ? -10 : 10));
    if (sel == 1 && (k == DKey::Left || k == DKey::Right)) {
      int i = 0;
      while (i < kSleepCount - 1 && kSleepChoices[i] != sleepMin_) ++i;
      i += k == DKey::Left ? (i > 0 ? -1 : 0) : (i < kSleepCount - 1 ? 1 : 0);
      sleepMin_ = kSleepChoices[i];
    }
    if (d >= 1 && d <= kSettingCount) {
      sel = d - 1;
      openSetting(sel);
    } else if (k == DKey::Eq) {
      openSetting(sel);
    }
  }
}

void Device::openSetting(int i) {
  switch (i) {
    case 2: infoBack_ = View::Settings; view_ = View::NetInfo; return;
    case 3: infoBack_ = View::Settings; view_ = View::Link; return;
    case 4: infoBack_ = View::Settings; view_ = View::Look; return;
    case 5: infoBack_ = View::Settings; view_ = View::ExamInfo; return;
    case kSetBattery:
      back_ = View::Settings;
      if (battery_ < 0) return notice({"BATTERY", "No battery (powered", "by USB).", "", "", "AC:back"});
      return notice({"BATTERY", std::to_string(battery_ > 100 ? 100 : battery_) + "%" + (charging_ ? ", charging" : ""),
                     "", "", "", "AC:back"});
    case kSetBattery + 1:
      back_ = View::Settings;
      return notice({"ABOUT", "Proofline", "Version " + (version_.empty() ? std::string("-") : version_), "",
                     "", "AC:back"});
    default:  // Brightness, Sleep timer: ◀ ▶ change them on the list
      return;
  }
}

// ---------------------------------------------------------------- drawing

std::string Device::resultText() const {
  if (!showingResult_ || matrixResultShown()) return "";  // a matrix / vector: its own screen
  if (baseN()) return baseText(static_cast<int32_t>(result_.v), base_);  // an integer (checked to fit the base)
  if (cmplx() && shownForm() >= 0) {  // a+bi / r∠θ: one row, or the half R⇔I shows
    const std::string full = cxFullText();
    if (textLength(full) <= kCols) return full;
    Cx z;
    z.re = result_;
    z.im = resultIm_;
    std::string a, b;
    complexParts(z, shownForm() == 1 ? CxForm::Polar : CxForm::Rect, showFraction_, norm_, angle_, a, b);
    return cxShowIm_ ? b : a;
  }
  if (eng_) return engText();
  return formatResult(result_, showFraction_, norm_);
}

std::string Device::engText() const {
  const double m = result_.v / std::pow(10.0, engExp_);
  // 10 significant digits, also for a mantissa below 1 (SHIFT ENG: 0.3333333333×10⁰), but
  // at most 10 decimals: 0.0000001234×10⁹ (the Eng key keeps the mantissa in that range).
  const int e = static_cast<int>(std::floor(std::log10(std::fabs(m))));
  int decimals = 9 - e;
  if (decimals < 0) decimals = 0;
  if (decimals > 10) decimals = 10;
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
  if (vars_.m.v != 0 || imag_.m.v != 0) fb.drawText(3, 0, "M");
  if (sto_) fb.drawText(5, 0, "STO");
  if (rcl_) fb.drawText(5, 0, "RCL");
  fb.drawText(9, 0, angle_ == AngleUnit::Deg ? "D" : angle_ == AngleUnit::Rad ? "R" : "G");
  // The mode's indicator, as the Casio shows STAT (and here TABLE) on every calculator screen.
  // EQN has none: the fx-ES indicator row has no EQN (only STAT CMPLX MAT VCT). BASE-N shows
  // its number mode there (Dec / Hex / Bin / Oct), the way the fx-ES names it.
  // MATRIX / VECTOR show the fx-ES's MAT / VCT.
  const bool modeTag = mode_ != Mode::Ai &&
                       (calcMode_ == Mode::Stat || calcMode_ == Mode::Table || calcMode_ == Mode::Cmplx ||
                        calcMode_ == Mode::BaseN || isMatVec()) &&
                       !isAppView(view_);
  if (modeTag)
    fb.drawText(11, 0, calcMode_ == Mode::Stat     ? "STAT"
                       : calcMode_ == Mode::Table  ? "TABLE"
                       : calcMode_ == Mode::BaseN  ? baseName(base_)
                       : calcMode_ == Mode::Matrix ? "MAT"
                       : calcMode_ == Mode::Vector ? "VCT"
                                                   : "CMPLX");
  if (exam_) {
    if (modeTag) fb.drawText(17, 0, "\xEE\x80\x80" + clockText(examMs_));  // padlock h:mm after it
    else fb.drawText(12, 0, "\xEE\x80\x80" "EXAM " + clockText(examMs_));  // padlock
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
  } else if (view_ == View::History || view_ == View::Notes || view_ == View::Settings) {
    up = listTop() > 0;
    down = listTop() + kListRows < listRows();
  } else if (view_ == View::Setup) {  // two pages: ▼ to the second, ▲ back
    up = setupPage_ == 1;
    down = setupPage_ == 0;
  } else if (view_ == View::BaseMenu) {  // likewise: the logic page, ▼ the prefixes
    up = basePage_ == 1;
    down = basePage_ == 0;
  } else if (view_ == View::NoteView && noteOpen_ >= 0 && noteOpen_ < static_cast<int>(notes_.size())) {
    up = noteTop_ > 0;
    down = noteTop_ + kNoteLines < static_cast<int>(wrapText(notes_[static_cast<size_t>(noteOpen_)], kCols).size());
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
    if (complexSplit()) fb.drawText(0, 5, "R\xE2\x87\x94I");  // R⇔I: SHIFT = shows the other half
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
  // Calculator screens look like the Casio's: only the numbered items, no key hints (the
  // fx-ES MODE / SETUP menus have none). The info pages shared with the Settings app keep
  // their hint line there (an app screen).
  const bool fromApps = infoBack_ == View::Settings;
  switch (view_) {
    case View::ModeMenu:
      fb.drawText(0, 1, two("1:COMP", "2:STAT"));
      fb.drawText(0, 2, two("3:TABLE", "4:APPS"));
      fb.drawText(0, 3, two("5:EQN", "6:CMPLX"));
      fb.drawText(0, 4, two("7:BASE-N", "8:MATRIX"));
      fb.drawText(0, 5, "9:VECTOR");
      if (exam_) fb.drawText(0, 6, "(Apps are off: exam mode)");
      break;
    case View::BaseMenu:  // SHIFT 3 in BASE-N, two pages (▲▼ in the status bar)
      if (basePage_ == 1) {
        fb.drawText(0, 1, two("1:d", "2:h"));
        fb.drawText(0, 2, two("3:b", "4:o"));
        break;
      }
      fb.drawText(0, 1, two("1:and", "2:or"));
      fb.drawText(0, 2, two("3:xnor", "4:xor"));
      fb.drawText(0, 3, two("5:Not", "6:Neg"));
      break;
    case View::Setup:
      if (setupPage_ == 1) {  // the second page (▼), like the Casio's: STAT's frequency column, CMPLX's form
        fb.drawText(0, 1, two("1:STAT", "2:CMPLX"));
        break;
      }
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
    case View::SetupCmplx:  // the Casio's CMPLX choice, the current one marked
      fb.drawText(0, 1, two("1:a+bi", "2:r\xE2\x88\xA0\xCE\xB8"));
      if (cmplxPolar_) mark(13, 1, 3);
      else mark(2, 1, 4);
      break;
    case View::CmplxMenu:  // SHIFT 2 in CMPLX
      fb.drawText(0, 1, two("1:arg", "2:Conjg"));
      fb.drawText(0, 2, two("3:\xE2\x96\xB6r\xE2\x88\xA0\xCE\xB8", "4:\xE2\x96\xB6" "a+bi"));
      break;
    case View::SetupFreq:  // the Casio's "Frequency?" with the current choice marked
      fb.drawText(0, 1, "Frequency?");
      fb.drawText(0, 2, two("1:ON", "2:OFF"));
      if (stat_.frequency()) mark(2, 2, 2);
      else mark(13, 2, 3);
      break;
    case View::Look:
      fb.drawText(0, 1, "SCREEN LOOK", true);
      fb.drawText(0, 2, two("1:Plain", "2:LCD dots"));
      if (dotMatrix_) mark(13, 2, 8);
      else mark(2, 2, 5);
      if (fromApps) fb.drawText(0, 6, "AC:back");
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
      if (fromApps) fb.drawText(0, 6, "AC:back");
      break;
    case View::ExamInfo:
      if (exam_) {
        fb.drawText(0, 1, "EXAM MODE IS ON", true);
        fb.drawText(0, 2, "On for " + clockText(examMs_));
        fb.drawText(0, 3, "Unlock: USB cable, or");
        fb.drawText(0, 4, "it ends in " + clockText(kExamMaxMs - examMs_));
        if (fromApps) fb.drawText(0, 6, "AC:back");
      } else {
        fb.drawText(0, 1, "EXAM MODE", true);
        fb.drawText(0, 2, "Turns off AI, camera and");
        fb.drawText(0, 3, "Wi-Fi until a USB cable");
        fb.drawText(0, 4, "unlocks it or 12 h pass.");
        if (fromApps) {
          fb.drawText(0, 6, two("=:Start", "AC:Cancel"));
        } else {  // from SETUP 8: the Casio's own confirm lines (as CLR's)
          fb.drawText(0, 5, "[=]  :Start");
          fb.drawText(0, 6, "[AC] :Cancel");
        }
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
    case View::Apps: case View::Keyboard: case View::History: case View::Notes: case View::NoteView:
    case View::Settings: case View::Confirm: case View::Geo: case View::Periodic: case View::Link:
      drawApps(fb);
      break;
    case View::Stat:
    case View::Table:
    case View::Eqn:
    case View::MatVec:
      // The mode draws rows 1-6 and says whether it scrolls; the arrows go in the status bar.
      if (view_ == View::Stat) stat_.render(fb);
      else if (view_ == View::Table) table_.render(fb);
      else if (view_ == View::Eqn) eqn_.render(fb);
      else mv_.render(fb);
      // In exam mode the status row is already inverted (drawStatus): the arrow cell is put
      // back to plain, drawn, and inverted again, so it shows (and reads back) like the rest.
      for (int i = 0; i < 2; ++i) {
        if (!fb.icon(i == 0 ? Icon::Up : Icon::Down)) continue;
        const int col = 23 + i;
        if (exam_) fb.invertRect(col * 5, 0, 5, 8);
        fb.drawText(col, 0, i == 0 ? "\xE2\x96\xB2" : "\xE2\x96\xBC");
        if (exam_) fb.invertRect(col * 5, 0, 5, 8);
      }
      break;
    case View::CalcRun: calcRun_.render(fb); break;
    case View::SolveRun: solveRun_.render(fb); break;
    default: drawMenu(fb); break;
  }
  if (toastVisible()) drawToast(fb);
}

void Device::drawToast(Framebuffer& fb) const {
  // A box round one line in the middle of the screen, the text on the character grid.
  const int n = textLength(toast_);
  const int col = (kCols - n) / 2, x = col * 5 - 3, y = 3 * 8 - 3, w = n * 5 + 5, h = 13;
  fb.fillRect(x, y, w, h, false);
  fb.fillRect(x, y, w, 1, true);
  fb.fillRect(x, y + h - 1, w, 1, true);
  fb.fillRect(x, y, 1, h, true);
  fb.fillRect(x + w - 1, y, 1, h, true);
  fb.drawText(col, 3, toast_);
}

void Device::drawApps(Framebuffer& fb) const {
  auto right = [&](int row, const std::string& s) { fb.drawText(kCols - textLength(s), row, s); };
  auto centre = [&](int row, const std::string& s) { fb.drawText((kCols - textLength(s)) / 2, row, s); };
  auto selectRow = [&](int row) { fb.invertRect(0, row * 8, Framebuffer::kWidth, 8); };  // keeps the title's rule
  // A list on rows 2-6 under a title, the selection inverted.
  auto list = [&](const std::vector<std::string>& rows, int sel) {
    const int top = listTop();
    for (int r = 0; r < kListRows && top + r < static_cast<int>(rows.size()); ++r) {
      fb.drawText(0, 2 + r, rows[static_cast<size_t>(top + r)]);
      if (top + r == sel) selectRow(2 + r);
    }
  };
  switch (view_) {
    case View::Apps: {
      fb.drawText(0, 1, "APPS");
      right(1, std::to_string(appsSel_ + 1) + "/" + std::to_string(kAppCount));
      centre(2, appName(appsSel_));
      fb.drawText(0, 2, "\xE2\x97\x80");  // ◀ ▶
      fb.drawText(kCols - 1, 2, "\xE2\x96\xB6");
      fb.invertRect(0, 15, Framebuffer::kWidth, 9);
      centre(4, appDescription(appsSel_));
      const int x0 = (Framebuffer::kWidth - (kAppCount * 7 - 4)) / 2;  // page dots: 3x3, 4 apart
      for (int i = 0; i < kAppCount; ++i) {
        fb.fillRect(x0 + i * 7, 42, 3, 3, true);
        if (i != appsSel_) fb.set(x0 + i * 7 + 1, 43, false);
      }
      fb.drawText(0, 6, "<> move  =open  AC back");
      return;
    }
    case View::Keyboard: {
      const std::string title = !kbForNote_ ? "TYPE IT" : editNote_ >= 0 ? "EDIT NOTE" : "NEW NOTE";
      fb.drawText(0, 1, title, true);
      right(1, std::to_string(textLength(kbText_)) + "/" + std::to_string(kMaxText));
      // The text: the two lines up to the cursor, which follows the last character.
      const std::vector<std::string> lines = hardWrap(kbText_);
      const int n = textLength(kbText_), cursorLine = n / kCols;
      const int first = cursorLine > 0 ? cursorLine - 1 : 0;
      for (int l = first; l < first + 2 && l < static_cast<int>(lines.size()); ++l)
        fb.drawText(0, 2 + l - first, lines[static_cast<size_t>(l)]);
      if (n == 0) fb.drawText(0, 3, "=:type  DEL:erase");
      if (!blink_ || ((nowMs_ - lastKeyMs_) / 500) % 2 == 0) {
        const int col = n % kCols, row = 2 + cursorLine - first;
        fb.fillRect(col == 0 ? 0 : col * 5 - 1, row * 8, 1, 7, true);
      }
      // The keys, the cursor cell inverted.
      for (int i = 0; i < keyboardCellCount(); ++i) {
        const KeyCell& c = keyboardCellAt(i);
        const char* label = c.kind == KeyCell::Send && kbForNote_ ? "SAVE" : c.label;
        fb.drawText(c.col, 4 + c.row, label);
        if (i == kbCell_) fb.invertRect(c.col * 5 - 1, (4 + c.row) * 8 - 1, c.width * 5 + 1, 9);
      }
      return;
    }
    case View::History: {
      fb.drawText(0, 1, "HISTORY", true);
      if (saved_.empty()) {
        fb.drawText(0, 2, "No answers yet. AI");
        fb.drawText(0, 3, "answers are kept here.");
        fb.drawText(0, 6, "AC:back");
        return;
      }
      right(1, std::to_string(saved_.size()) + "/" + std::to_string(kMaxHistory));
      std::vector<std::string> rows;
      for (const auto& s : saved_) rows.push_back(fit(s.answer, kCols));
      rows.push_back("Clear history");
      list(rows, histSel_);
      return;
    }
    case View::Notes: {
      fb.drawText(0, 1, "NOTES", true);
      right(1, std::to_string(notes_.size()) + "/" + std::to_string(kMaxNotes));
      std::vector<std::string> rows = {"+ New note"};
      for (const auto& note : notes_) rows.push_back(fit(note, kCols));
      list(rows, notesSel_);
      return;
    }
    case View::NoteView: {
      if (noteOpen_ < 0 || noteOpen_ >= static_cast<int>(notes_.size())) return;
      fb.drawText(0, 1, "NOTE " + std::to_string(noteOpen_ + 1), true);
      const auto lines = wrapText(notes_[static_cast<size_t>(noteOpen_)], kCols);
      for (int r = 0; r < kNoteLines && noteTop_ + r < static_cast<int>(lines.size()); ++r)
        fb.drawText(0, 2 + r, lines[static_cast<size_t>(noteTop_ + r)]);
      fb.drawText(0, 6, "1:Edit 2:Delete AC:back");
      return;
    }
    case View::Settings: {
      fb.drawText(0, 1, "SETTINGS", true);
      std::vector<std::string> rows;
      for (int i = 0; i < kSettingCount; ++i) {
        std::string row = std::to_string(i + 1) + ":" + kSettings[i];
        std::string value;
        if (i == 0) value = std::to_string(brightness_) + "%";
        if (i == 1) value = std::to_string(sleepMin_) + " min";
        if (i == kSetBattery && battery_ >= 0) value = std::to_string(battery_ > 100 ? 100 : battery_) + "%";
        if (i <= 1) value = "\xE2\x97\x80" + value + "\xE2\x96\xB6";  // ◀ ▶ change it
        rows.push_back(pad(row, kCols - textLength(value)) + value);
      }
      list(rows, setSel_);
      return;
    }
    case View::Confirm:
      fb.drawText(0, 1, confirmQuestion());
      fb.drawText(0, 3, "[=]  :Yes");
      fb.drawText(0, 4, "[AC] :No");
      return;
    case View::Geo:
    case View::Periodic:
      // The app draws rows 1-6 and says whether it scrolls; the arrows go in the status bar.
      if (view_ == View::Geo) geo_.render(fb);
      else periodic_.render(fb);
      if (fb.icon(Icon::Up)) fb.drawText(23, 0, "\xE2\x96\xB2");
      if (fb.icon(Icon::Down)) fb.drawText(24, 0, "\xE2\x96\xBC");
      return;
    case View::Link: {
      fb.drawText(0, 1, "LINK CALCULATOR", true);
      std::string text;
      int row = 3;
      if (linkCode_.empty()) {
        text = linked_ ? "Already linked." : "Connect to Wi-Fi to get a link code.";
      } else {
        const int n = textLength(linkCode_);  // big when it fits: 10 px a character
        if (n * 10 <= Framebuffer::kWidth) fb.drawTextPx((Framebuffer::kWidth - n * 10) / 2, 18, linkCode_, 2);
        else centre(2, linkCode_);
        std::string url = linkUrl_;  // shown without the scheme: it fits, and nobody types it
        for (const char* scheme : {"https://", "http://"})
          if (url.rfind(scheme, 0) == 0) url.erase(0, std::strlen(scheme));
        while (!url.empty() && url.back() == '/') url.pop_back();
        text = url.empty() ? "Type this code on the website's link page." : "Go to " + url + "/link and type this code.";
        row = 4;
      }
      std::vector<std::string> lines = wrapText(text, kCols);
      if (linked_ && linkCode_.empty() && !linkPlan_.empty())  // the plan, under "Already linked."
        for (const std::string& l : wrapText(linkPlan_, kCols)) lines.push_back(l);
      for (size_t i = 0; i < lines.size() && row + static_cast<int>(i) <= 6; ++i)
        fb.drawText(0, row + static_cast<int>(i), lines[i]);
      if (row + static_cast<int>(lines.size()) <= 6) fb.drawText(0, 6, "AC:back");
      return;
    }
    default:
      return;
  }
}

}  // namespace calc

// ---------------------------------------------------------------- state across deep sleep

namespace calc {

namespace {
// Version 2 = version 1's layout + an Adler-32 of everything before it (the last 4 bytes): RTC
// memory that lost a bit while asleep is refused, not restored. Version 1 (no checksum, older
// firmware) is still read.
constexpr uint8_t kStateVersion = 2, kStateVersionNoSum = 1;
// After the replay history, optionally: the calculator mode's own data. 'S': STAT's type
// (0xFF = none chosen yet), rows, and each row's X [Y] [FREQ]; 'T': TABLE's f(X) and range;
// 'E': EQN's type (0xFF = none chosen yet) and its rows x columns of coefficients (2x3, 3x4,
// 1x3 or 1x4 doubles); 'C' (CMPLX): the 11 memories' imaginary parts (A-F X Y M Ans PreAns),
// then a count equal to the history's and, newest first, each replay item's imaginary part and
// its form byte (0xFF none, 0 ▶a+bi, 1 ▶r∠θ). Older states end after the history (COMP), and
// older firmware refuses a mode it doesn't know (EQN's mode byte 4 came with the 'E' block,
// CMPLX's 5 with 'C', BASE-N's 6 with 'N'; the version stays 2). Flag 64: SETUP ▼ 2:CMPLX is
// r∠θ. 'N' (BASE-N): the number mode byte (0 DEC, 1 HEX, 2 BIN, 3 OCT); its replay values are
// integers in the 32-bit range (else the state is refused).
// 'X' (MATRIX) / 'V' (VECTOR): MatA MatB MatC MatAns (VctA ... VctAns), each a size byte (rows
// << 4 | columns, 0 = no size yet) and its elements row by row (Nums), then a count equal to the
// history's and each replay item's form byte (0xFF a scalar, 2 a matrix / vector), newest first.
// At most 1 + 4 x (1 + 9 x 25) + 1 + 20 = 926 bytes: with the fixed part (~290) it fits the 2 KB
// RTC state, the replay taking what is left (PORT_NOTES 2026-10-12).
constexpr uint8_t kStatTag = 'S', kTableTag = 'T', kEqnTag = 'E', kCmplxTag = 'C', kBaseTag = 'N',
                  kMatTag = 'X', kVctTag = 'V', kNoStatType = 0xFF;

void putU8(std::string& o, uint8_t v) { o += static_cast<char>(v); }
void putU32(std::string& o, uint32_t v) {
  for (int i = 0; i < 4; ++i) o += static_cast<char>((v >> (8 * i)) & 0xFF);
}
void putU64(std::string& o, uint64_t v) {
  for (int i = 0; i < 8; ++i) o += static_cast<char>((v >> (8 * i)) & 0xFF);
}
void putDouble(std::string& o, double v) {
  uint64_t bits;
  std::memcpy(&bits, &v, sizeof bits);
  putU64(o, bits);
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
  std::string text(size_t maxLen) {  // a 16-bit length, then that many bytes
    size_t n = u8();
    n |= size_t(u8()) << 8;
    if (!ok || n > maxLen || pos + n > s.size()) return ok = false, std::string();
    pos += n;
    return s.substr(pos - n, n);
  }
  uint64_t u64() {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= uint64_t(u8()) << (8 * i);
    return v;
  }
  // A value the calculator could hold: finite, |v| < 1E100, and 0 or |v| >= 1E-99 (the
  // calculator makes 0 of anything smaller; denormals are smaller still).
  static bool holdable(double v) { return std::fabs(v) < 1e100 && (v == 0 || std::fabs(v) >= 1e-99); }
  double dbl() {
    const uint64_t bits = u64();
    double v;
    std::memcpy(&v, &bits, sizeof v);
    if (!holdable(v)) ok = false;
    return ok ? v : 0.0;
  }
  Num num() {
    Num n;
    const uint64_t bits = u64();
    std::memcpy(&n.v, &bits, sizeof bits);
    n.exact = u8() != 0;
    n.num = static_cast<long long>(u64());
    n.den = static_cast<long long>(u64());
    // Only a value the calculator could have made, else the state is damaged (a NaN shows
    // as "nan", and an exact LLONG_MIN or a negative denominator crashes the maths): finite
    // and inside the display's range (|x| < 1E100), and an exact one a fraction in the
    // engine's limits (0 < den, |num| and den <= 1E15) that agrees with its value.
    if (!(std::fabs(n.v) < 1e100)) ok = false;
    if (!n.exact && !holdable(n.v) && std::fabs(n.v) < 1) n.v = 0;  // below 1E-99: 0, as check() does
    if (n.exact) {
      constexpr long long kMax = 1000000000000000LL;
      if (n.den <= 0 || n.den > kMax || n.num > kMax || n.num < -kMax)
        ok = false;
      else if (std::fabs(static_cast<double>(n.num) / static_cast<double>(n.den) - n.v) >
               1e-9 * std::max(1.0, std::fabs(n.v)))
        ok = false;
    } else if (n.den == 0) {
      n.den = 1;
    }
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
  putU8(o, static_cast<uint8_t>(mode()));  // Comp, Ai, Stat, Table or Eqn
  putU8(o, static_cast<uint8_t>(angle_));
  putU8(o, static_cast<uint8_t>(norm_));
  putU8(o, static_cast<uint8_t>((dotMatrix_ ? 1 : 0) | (showFraction_ ? 2 : 0) | (exam_ ? 4 : 0) |
                                (app_.tutor() ? 8 : 0) | (view_ == View::Off ? 16 : 0) |
                                (stat_.frequency() ? 32 : 0) | (cmplxPolar_ ? 64 : 0)));
  putU8(o, static_cast<uint8_t>(app_.effort()));
  putU8(o, static_cast<uint8_t>(offSequence_));
  putU32(o, examMs_);
  Vars v = vars_;
  for (int i = 0; i < kVarSlots; ++i) putNum(o, *varSlots(v, i));
  // The mode's data (STAT's table of values, TABLE's f(X)): kept like the Casio keeps it.
  // At most 1 + 1 + 1 + 80 x 8 bytes (STAT), 1 + 1 + 99 + 24 (TABLE) or 1 + 1 + 12 x 8 (EQN).
  std::string ext;
  if (calcMode_ == Mode::Stat) {
    putU8(ext, kStatTag);
    const bool chosen = stat_.screen() != StatApp::Screen::TypeSelect || stat_.rows() > 0;
    putU8(ext, chosen ? static_cast<uint8_t>(stat_.type()) : kNoStatType);
    const int rows = chosen ? stat_.rows() : 0;
    putU8(ext, static_cast<uint8_t>(rows));
    for (int i = 0; i < rows; ++i) {
      putDouble(ext, stat_.x(i));
      if (stat_.paired()) putDouble(ext, stat_.y(i));
      if (stat_.frequency()) putDouble(ext, stat_.freq(i));
    }
  } else if (calcMode_ == Mode::Table) {
    putU8(ext, kTableTag);
    const std::vector<Tok>& f = table_.function();
    const size_t n = std::min<size_t>(f.size(), kMaxTokens);
    putU8(ext, static_cast<uint8_t>(n));
    for (size_t t = 0; t < n; ++t) putU8(ext, static_cast<uint8_t>(f[t]));
    putDouble(ext, table_.start());
    putDouble(ext, table_.end());
    putDouble(ext, table_.step());
  } else if (calcMode_ == Mode::Eqn) {
    putU8(ext, kEqnTag);
    putU8(ext, eqn_.typeChosen() ? static_cast<uint8_t>(eqn_.type()) : kNoStatType);
    if (eqn_.typeChosen())
      for (int r = 0; r < eqn_.rows(); ++r)
        for (int c = 0; c < eqn_.cols(); ++c) putDouble(ext, eqn_.coef(r, c));
  } else if (calcMode_ == Mode::Cmplx) {
    putU8(ext, kCmplxTag);
    ImagVars im = imag_;
    const Num* slots[] = {&im.a, &im.b, &im.c, &im.d, &im.e, &im.f, &im.x, &im.y, &im.m, &im.ans, &im.preAns};
    for (const Num* n : slots) putNum(ext, *n);
  } else if (calcMode_ == Mode::BaseN) {
    putU8(ext, kBaseTag);
    putU8(ext, static_cast<uint8_t>(base_));
  } else if (isMatVec()) {
    putU8(ext, calcMode_ == Mode::Vector ? kVctTag : kMatTag);
    for (int i = 0; i < 4; ++i) {
      const Mat& m = mv_.var(i);
      putU8(ext, static_cast<uint8_t>(m.rows << 4 | m.cols));
      for (int r = 0; r < m.rows; ++r)
        for (int c = 0; c < m.cols; ++c) putNum(ext, m.at(r, c));
    }
  }
  // CMPLX: each saved replay item's imaginary part and form, after the count (sized in the loop);
  // MATRIX / VECTOR: each item's form byte.
  const size_t perItemCx = calcMode_ == Mode::Cmplx ? 25 + 1 : isMatVec() ? 1 : 0;
  std::string histCx;
  // Newest history first, as many as fit (the Casio keeps its replay too).
  std::string hist;
  int count = 0;
  for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
    std::string item;
    putU8(item, static_cast<uint8_t>(std::min<size_t>(it->expr.size(), kMaxTokens)));
    for (size_t t = 0; t < it->expr.size() && t < size_t(kMaxTokens); ++t) putU8(item, static_cast<uint8_t>(it->expr[t]));
    putNum(item, it->value);
    const size_t cxBytes = perItemCx ? 1 + histCx.size() + perItemCx : 0;
    if (o.size() + 1 + hist.size() + item.size() + ext.size() + cxBytes + 4 > maxBytes) break;
    hist += item;
    if (perItemCx) {
      if (perItemCx > 1) putNum(histCx, it->im);
      putU8(histCx, it->form < 0 ? kNoStatType : static_cast<uint8_t>(it->form));
    }
    ++count;
  }
  putU8(o, static_cast<uint8_t>(count));
  o += hist;
  if (perItemCx) {
    putU8(ext, static_cast<uint8_t>(count));
    ext += histCx;
  }
  o += ext;
  putU32(o, pngAdler32(reinterpret_cast<const uint8_t*>(o.data()), o.size()));
  return o;
}

bool Device::restoreState(const std::string& saved, uint32_t asleepMs) {
  if (saved.empty()) return false;
  std::string body;
  if (static_cast<uint8_t>(saved[0]) == kStateVersion) {
    if (saved.size() < 5) return false;
    const size_t n = saved.size() - 4;
    Reader tail{saved, n};
    if (tail.u32() != pngAdler32(reinterpret_cast<const uint8_t*>(saved.data()), n)) return false;
    body = saved.substr(0, n);
  } else if (static_cast<uint8_t>(saved[0]) == kStateVersionNoSum) {
    body = saved;
  } else {
    return false;
  }
  const std::string& state = body;
  Reader r{state};
  r.u8();  // the version, checked above
  const uint8_t mode = r.u8(), angle = r.u8(), norm = r.u8(), flags = r.u8(), effort = r.u8(), offSeq = r.u8();
  const uint32_t examMs = r.u32();
  Vars v;
  for (int i = 0; i < kVarSlots; ++i) *varSlots(v, i) = r.num();
  const int count = r.u8();
  std::vector<HistoryItem> hist;
  for (int i = 0; i < count && r.ok; ++i) {
    HistoryItem h;
    const int n = r.u8();
    if (n > kMaxTokens) return false;  // never saved: more than the Casio's 99 steps
    for (int t = 0; t < n; ++t) {
      const uint8_t tok = r.u8();
      if (tok >= static_cast<uint8_t>(Tok::Count)) return false;
      h.expr.push_back(static_cast<Tok>(tok));
    }
    h.value = r.num();
    hist.insert(hist.begin(), h);  // stored newest first
  }
  if (!r.ok || mode > static_cast<uint8_t>(Mode::Vector) || angle > 2 || norm > 1 || effort > 2) return false;
  Base base = Base::Dec;  // BASE-N's 'N' block (DEC without one)
  ImagVars imag;  // CMPLX's 'C' block (all 0 without one)
  // The mode's data (saveState()): read into fresh copies, used only if all of it is sound.
  const bool freq = (flags & 32) != 0;
  std::unique_ptr<StatApp> statP(new StatApp);  // ~2 KB: off the (small ESP32) stack
  std::unique_ptr<TableApp> tableP(new TableApp);
  std::unique_ptr<EqnApp> eqnP(new EqnApp);
  std::unique_ptr<MatVecApp> mvP(new MatVecApp);
  StatApp& stat = *statP;
  TableApp& table = *tableP;
  EqnApp& eqn = *eqnP;
  stat.setFrequency(freq);
  // The calculator mode: the mode byte, or (under AI SOLVE, mode Ai) the mode block's tag.
  Mode calcMode = mode >= static_cast<uint8_t>(Mode::Stat) ? static_cast<Mode>(mode) : Mode::Comp;
  const bool ai = mode == static_cast<uint8_t>(Mode::Ai);
  mvP->reset(calcMode == Mode::Vector);  // no 'X' / 'V' block: no variables yet
  if (r.ok && r.pos < state.size()) {
    const uint8_t tag = r.u8();
    if (tag == kStatTag && (ai || calcMode == Mode::Stat)) {
      calcMode = Mode::Stat;
      const uint8_t type = r.u8();
      const int rows = r.u8();
      if (type != kNoStatType) {
        if (type >= static_cast<uint8_t>(StatType::Count)) return false;
        stat.setType(static_cast<StatType>(type));
        if (rows > stat.maxRows()) return false;
        for (int i = 0; i < rows && r.ok; ++i) {
          const double x = r.dbl();
          const double y = stat.paired() ? r.dbl() : 0.0;
          const double f = freq ? r.dbl() : 1.0;
          if (r.ok && !stat.addRow(x, y, f)) return false;
        }
        stat.setExpression({});  // back on the stat calculation screen
      } else if (rows != 0) {
        return false;
      }
    } else if (tag == kTableTag && (ai || calcMode == Mode::Table)) {
      calcMode = Mode::Table;
      const int n = r.u8();
      if (n > kMaxTokens) return false;
      std::vector<Tok> f;
      for (int t = 0; t < n; ++t) {
        const uint8_t tok = r.u8();
        if (tok >= static_cast<uint8_t>(Tok::Count)) return false;
        f.push_back(static_cast<Tok>(tok));
      }
      const double a = r.dbl(), b = r.dbl(), c = r.dbl();
      table.setFunction(f);
      table.setRange(a, b, c);
    } else if (tag == kEqnTag && (ai || calcMode == Mode::Eqn)) {
      calcMode = Mode::Eqn;
      const uint8_t type = r.u8();
      if (type != kNoStatType) {
        if (type >= static_cast<uint8_t>(EqnType::Count)) return false;
        eqn.setType(static_cast<EqnType>(type));  // the coefficient editor
        for (int row = 0; row < eqn.rows(); ++row)
          for (int col = 0; col < eqn.cols(); ++col) {
            const double v = r.dbl();
            if (r.ok && !eqn.setCoef(row, col, v)) return false;
          }
      }
    } else if (tag == kCmplxTag && (ai || calcMode == Mode::Cmplx)) {
      calcMode = Mode::Cmplx;
      Num* slots[] = {&imag.a, &imag.b, &imag.c, &imag.d, &imag.e, &imag.f, &imag.x, &imag.y, &imag.m, &imag.ans,
                      &imag.preAns};
      for (Num* n : slots) *n = r.num();
      if (r.u8() != hist.size()) return false;
      for (size_t i = 0; i < hist.size() && r.ok; ++i) {
        HistoryItem& h = hist[hist.size() - 1 - i];  // stored newest first
        h.im = r.num();
        const uint8_t form = r.u8();
        if (form != kNoStatType && form > 1) return false;
        h.form = form == kNoStatType ? -1 : static_cast<int8_t>(form);
      }
    } else if (tag == kBaseTag && (ai || calcMode == Mode::BaseN)) {
      calcMode = Mode::BaseN;
      const uint8_t b = r.u8();
      if (b > static_cast<uint8_t>(Base::Oct)) return false;
      base = static_cast<Base>(b);
    } else if ((tag == kMatTag && (ai || calcMode == Mode::Matrix)) || (tag == kVctTag && (ai || calcMode == Mode::Vector))) {
      const bool vec = tag == kVctTag;
      calcMode = vec ? Mode::Vector : Mode::Matrix;
      mvP->reset(vec);
      for (int i = 0; i < 4 && r.ok; ++i) {
        const uint8_t size = r.u8();
        const int rows = size >> 4, cols = size & 15;
        if (rows > Mat::kMax || cols > Mat::kMax) return false;
        Mat m = rows ? Mat::zero(rows, cols) : Mat();
        m.cols = static_cast<uint8_t>(cols);  // no rows but columns: refused by setVar()
        for (int row = 0; row < rows; ++row)
          for (int col = 0; col < cols; ++col) m.at(row, col) = r.num();
        if (!r.ok || !mvP->setVar(i, m)) return false;
      }
      if (r.u8() != hist.size()) return false;
      for (size_t i = 0; i < hist.size() && r.ok; ++i) {
        const uint8_t form = r.u8();
        if (form != kNoStatType && form != 2) return false;
        hist[hist.size() - 1 - i].form = form == kNoStatType ? -1 : 2;  // stored newest first
      }
    } else {
      return false;
    }
    if (!r.ok || r.pos != state.size()) return false;
  }
  if (calcMode == Mode::BaseN)  // its replay holds integers BASE-N made (resultText() relies on it)
    for (const HistoryItem& h : hist)
      if (h.value.v != std::trunc(h.value.v) || h.value.v < -2147483648.0 || h.value.v > 2147483647.0) return false;
  mode_ = ai ? Mode::Ai : Mode::Comp;
  calcMode_ = calcMode;
  base_ = base;
  imag_ = imag;
  cmplxPolar_ = (flags & 64) != 0;
  stat_ = stat;
  table_ = table;
  eqn_ = eqn;
  mv_ = *mvP;
  resultForm_ = -1;
  angle_ = static_cast<AngleUnit>(angle);
  norm_ = static_cast<NormMode>(norm);
  syncModes();  // after angle_ / norm_: STAT and TABLE draw with the restored ones
  dotMatrix_ = (flags & 1) != 0;
  showFraction_ = (flags & 2) != 0;
  app_.setTutor((flags & 8) != 0);
  app_.setEffort(static_cast<Effort>(effort));
  vars_ = v;
  history_ = hist;
  offSequence_ = offSeq <= 2 ? offSeq : 0;
  exam_ = false;
  examMs_ = 0;
  examExpiredOnRestore_ = false;
  if (flags & 4) {  // ends itself if 12 h passed while asleep (the sum saturates: no wrap after 49 days)
    const uint64_t elapsed = uint64_t(examMs) + asleepMs;
    restoreExam(elapsed > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(elapsed));
  }
  if (flags & 16) {
    if (view_ != View::Off) powerOff();
    offSequence_ = offSeq <= 2 ? offSeq : 0;  // powerOff() cleared it
  } else {
    view_ = back_ = modeView();  // restoreExam() has closed AI SOLVE in exam mode
  }
  return true;
}


// ---------------------------------------------------------------- apps data in flash

namespace {
// version, last app, answers (answer, reply), notes, then an Adler-32 of all of it.
constexpr uint8_t kAppsVersion = 1;
constexpr size_t kMaxNoteBytes = Device::kMaxText * 4;  // UTF-8: up to 4 bytes a character

void putText(std::string& o, const std::string& s) {
  o += static_cast<char>(s.size() & 0xFF);
  o += static_cast<char>((s.size() >> 8) & 0xFF);
  o += s;
}
}  // namespace

std::string Device::saveApps() const {
  // Notes always; answers newest first. If the whole is over kMaxAppsBytes, the oldest
  // answers go in their short form (question and answer), and if it still doesn't fit the
  // oldest are left out. Every answer short and 10 full notes of 4-byte characters is
  // ~27 KB, so nothing is left out in practice (tests: worst case ~35 KB, under budget).
  std::string notes;
  putU8(notes, static_cast<uint8_t>(notes_.size()));
  for (const auto& n : notes_) putText(notes, n);
  const size_t n = std::min(saved_.size(), static_cast<size_t>(kMaxHistory));
  std::vector<std::string> items(n);
  size_t total = 3 + notes.size() + 4;
  for (size_t i = 0; i < n; ++i) {
    putText(items[i], utf8Prefix(saved_[i].answer, kMaxSavedAnswer));
    putText(items[i], utf8Prefix(saved_[i].replyJson, 0xFFFF));  // at most kMaxSavedJson (recordAnswer())
    total += items[i].size();
  }
  for (size_t i = n; i-- > 0 && total > kMaxAppsBytes;) {
    const std::string json = compactReply(saved_[i].replyJson, kShortSavedJson);
    if (json.empty()) continue;  // unreadable: can't be shortened (left out below if need be)
    std::string item;
    putText(item, utf8Prefix(saved_[i].answer, kMaxSavedAnswer));
    putText(item, json);
    total = total - items[i].size() + item.size();
    items[i] = item;
  }
  size_t count = n;
  while (count > 0 && total > kMaxAppsBytes) total -= items[--count].size();
  std::string o;
  putU8(o, kAppsVersion);
  putU8(o, static_cast<uint8_t>(appsSel_));
  putU8(o, static_cast<uint8_t>(count));
  for (size_t i = 0; i < count; ++i) o += items[i];
  o += notes;
  putU32(o, pngAdler32(reinterpret_cast<const uint8_t*>(o.data()), o.size()));
  return o;
}

bool Device::restoreApps(const std::string& blob) {
  if (blob.size() < 7) return false;
  const size_t body = blob.size() - 4;
  Reader tail{blob, body};
  if (tail.u32() != pngAdler32(reinterpret_cast<const uint8_t*>(blob.data()), body)) return false;
  const std::string data = blob.substr(0, body);
  Reader r{data};
  if (r.u8() != kAppsVersion) return false;
  const int sel = r.u8();
  const int nSaved = r.u8();
  if (sel >= kAppCount || nSaved > kMaxHistory) return false;
  std::vector<SavedAnswer> saved;
  for (int i = 0; i < nSaved && r.ok; ++i) {
    SavedAnswer a;
    a.answer = utf8Prefix(r.text(1000), kMaxSavedAnswer);
    // Older firmware kept replies up to kMaxLoadJson: those are shortened as recordAnswer()
    // does now (one that can't be read is dropped: it couldn't be shown anyway).
    a.replyJson = compactReply(r.text(kMaxLoadJson), kMaxSavedJson);
    if (!a.replyJson.empty()) saved.push_back(a);
  }
  const int nNotes = r.u8();
  if (nNotes > kMaxNotes) return false;
  std::vector<std::string> notes;
  for (int i = 0; i < nNotes && r.ok; ++i) {
    notes.push_back(r.text(kMaxNoteBytes));
    if (textLength(notes.back()) > kMaxText || blankText(notes.back())) return false;
  }
  if (!r.ok || r.pos != data.size()) return false;
  appsSel_ = sel;
  saved_ = saved;
  notes_ = notes;
  histSel_ = notesSel_ = 0;
  noteOpen_ = -1;
  editNote_ = -1;  // the note being edited may be another one now (or gone)
  if (kbForNote_) {
    kbForNote_ = false;
    kbText_ = typedDraft_;
    kbOriginal_.clear();
    if (view_ == View::Keyboard) view_ = View::Apps;
    if (back_ == View::Keyboard) back_ = View::Apps;
  }
  if (view_ == View::NoteView || view_ == View::Confirm || view_ == View::History) view_ = View::Apps;
  if (back_ == View::NoteView || back_ == View::Confirm || back_ == View::History) back_ = View::Apps;
  return true;
}

}  // namespace calc
