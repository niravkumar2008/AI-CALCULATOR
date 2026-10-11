// The whole calculator: a Casio fx-300ES PLUS style scientific calculator
// (COMP mode) with the AI photo solver as mode 4 in the MODE menu, SETUP
// options, and an exam mode that switches the AI off.
//
// Platform code (Windows simulator, web simulator, ESP32 firmware) feeds key
// presses, time and network results in, and draws what render() produces.
// Nothing here touches hardware, so every behaviour is unit-tested on a PC.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "app.h"
#include "calc_engine.h"
#include "framebuffer.h"

namespace calc {

// Every key on the keypad, top-left to bottom-right, with the fx-115ES legends
// (the product's shell; stage 14). Row 2 is CALC, ∫dx, x⁻¹, logₐb. On the
// fx-115ES, Abs is SHIFT hyp, x³ is SHIFT x² and ∛ is SHIFT √. CALC / SOLVE and
// ∫dx / d/dx are not implemented yet: they show a "not supported yet" notice.
// nPr / nCr are SHIFT × / SHIFT ÷ (Pol / Rec on SHIFT + / −).
enum class DKey : uint8_t {
  Shift, Alpha, Up, Down, Left, Right, Mode, On,
  Calc, Integral, Inv, LogAB,
  Frac, Sqrt, Sq, Pow, Log, Ln,
  Neg, Dms, Hyp, Sin, Cos, Tan,
  Rcl, Eng, Open, Close, SD, MPlus,
  D7, D8, D9, Del, AC,
  D4, D5, D6, Mul, Div,
  D1, D2, D3, Add, Sub,
  D0, Dot, Exp10, Ans, Eq,
  Count
};
const char* dkeyName(DKey k);  // "SHIFT", "sin", "7" ... for logs and tests

// Typing on a computer keyboard or the Serial Monitor: one character = one
// key (or SHIFT + key). Digits . + - * / ^ ( ) = as printed; s c t sin cos tan,
// l log, n ln, r sqrt, q x², i x⁻¹, b Abs (SHIFT hyp), g logₐb, f fraction, ~ (−), E ×10ˣ, h hyp, k RCL,
// w S⇔D, M M+, a Ans, # DEL, $ AC, m MODE, o ON, [ SHIFT, ] ALPHA,
// < > u d arrows, p π, ! x!, % percent, , comma. Returns false if unmapped.
bool keysForChar(char c, DKey out[2], int& n);

enum class Mode : uint8_t { Comp, Ai };

// What fills the screen below the status bar.
enum class View : uint8_t {
  Off, Calc, Error, ModeMenu, Setup, Hyp, Clr, ClrConfirm, NetInfo, Look, ExamInfo, Notice, Ai
};

class Device {
 public:
  static constexpr uint32_t kAutoOffMs = 10u * 60u * 1000u;     // like the Casio: ~10 min idle
  static constexpr uint32_t kExamMaxMs = 12u * 60u * 60u * 1000u;  // exam mode ends itself after 12 h

  Device();

  void onKey(DKey k);
  void tick(uint32_t nowMs);
  void render(Framebuffer& fb) const;  // clears fb first; off = blank, or "Charging" with a cable in

  // ---- AI solver (same protocol as App: see app.h) ----
  int takeRequest() { return app_.takeRequest(); }
  // Starts an AI scan from any screen (the preview page's Send button): goes
  // to AI SOLVE, drops whatever scan or answer was there, and presses =.
  // False when off or in exam mode. Collect the request with takeRequest().
  bool startScan();
  // Countdown on the busy screen until the camera has its frames (App).
  void setHoldUntil(uint32_t ms) { app_.setHoldUntil(ms); }
  int activeRequest() const { return app_.activeRequest(); }
  void onCaptured(int id) { app_.onCaptured(id); }
  void onPartialAnswer(int id, double confidence, const std::string& answer) {
    app_.onPartialAnswer(id, confidence, answer);
  }
  void onReply(int id, const std::string& replyJson);
  void onFailure(int id, Failure f, const std::string& detail = "") { app_.onFailure(id, f, detail); }

  // ---- platform status ----
  void setOnline(bool online);
  void setHasApiKey(bool has) { app_.setHasApiKey(has); }
  void setNoKeyHelp(const std::string& help) { app_.setNoKeyHelp(help); }
  // Battery too low for the camera and Wi-Fi (the platform decides, with hysteresis).
  void setLowBattery(bool low) { app_.setLowBattery(low); }
  // Shown under SETUP > Wi-Fi & key. ssid "" = not set; keyHint e.g. "saved (...a1b2)".
  void setNetInfo(const std::string& ssid, const std::string& keyHint, const std::string& howToChange);
  void setRandomSource(double (*random)()) { random_ = random; }
  void setBlinkingCursor(bool on) { blink_ = on; }  // simulators blink; e-paper keeps it steady
  // 0-100, or -1 for no battery (tester, simulators). A battery icon shows
  // in the status bar at 20% or below, and always while charging.
  void setBattery(int percent, bool charging = false) {
    battery_ = percent;
    charging_ = charging;
  }

  // ---- exam mode ----
  bool examActive() const { return exam_; }
  uint32_t examElapsedMs() const { return examMs_; }
  bool radiosAllowed() const { return !exam_; }  // platform turns Wi-Fi and camera off when false
  void usbUnlock();                               // a teacher's USB cable ends exam mode
  void restoreExam(uint32_t elapsedMs);           // after a restart: resume exam mode silently

  // ---- state kept while the chip deep-sleeps (RTC memory) ----
  // Memories A-F X Y M, Ans, replay history (newest first, as much as fits in maxBytes), SETUP
  // choices, mode, effort, tutor, exam mode and the off-key sequence. Not the screen contents.
  std::string saveState(size_t maxBytes = 2048) const;
  // False (and nothing changes) for a missing or damaged state. asleepMs counts towards exam mode's 12 h.
  bool restoreState(const std::string& state, uint32_t asleepMs = 0);

  // ---- state, for the platform and tests ----
  bool isOff() const { return view_ == View::Off; }
  bool dotMatrix() const { return dotMatrix_; }
  Mode mode() const { return mode_; }
  View view() const { return view_; }
  AngleUnit angleUnit() const { return angle_; }
  NormMode norm() const { return norm_; }
  const std::vector<Tok>& expression() const { return expr_; }
  int cursor() const { return cursor_; }
  bool showingResult() const { return showingResult_; }
  std::string resultText() const;  // "" when no result is shown
  std::string errorMessage() const { return view_ == View::Error ? errorText(error_) : ""; }
  const Vars& vars() const { return vars_; }
  const App& ai() const { return app_; }

 private:
  struct HistoryItem {
    std::vector<Tok> expr;
    Num value;
  };
  enum class ScanNote : uint8_t { None, Ok, Unclear };

  void powerOn();
  void powerOff();
  void keyCalc(DKey k, bool shift, bool alpha);
  void keyAi(DKey k);
  void insert(Tok t);
  void equals();
  bool valueForMemory(Num& out);  // current result or the evaluated expression
  void showValue(const Num& v, const std::string& label);
  void clearEntry();
  void showError(CalcError e, int pos);
  void openMenu(View v);
  void notice(const std::vector<std::string>& lines);
  void startExam();
  void endExam(const char* why);
  void leaveAi();
  void pullScan();

  void drawStatus(Framebuffer& fb) const;
  void drawCalc(Framebuffer& fb) const;
  void drawMenu(Framebuffer& fb) const;
  std::string engText() const;

  App app_;
  Mode mode_ = Mode::Comp;
  View view_ = View::Calc;
  View back_ = View::Calc;  // where Notice / info screens return to

  // modifiers
  bool shift_ = false, alpha_ = false, sto_ = false, rcl_ = false;
  bool insertMode_ = true;  // SHIFT DEL toggles overwrite

  // calculator
  std::vector<Tok> expr_;
  int cursor_ = 0;
  bool showingResult_ = false;
  Num result_;
  std::string label_;       // "Ans→A", "A=": replaces the expression line
  Tok continueWith_ = Tok::Ans;
  bool showFraction_ = true;  // S⇔D
  bool eng_ = false;
  int engExp_ = 0;
  CalcError error_ = CalcError::None;
  int errorPos_ = 0;
  std::vector<HistoryItem> history_;
  int historyPos_ = -1;  // -1 = not browsing
  Vars vars_;
  ScanNote scanNote_ = ScanNote::None;
  int clrChoice_ = 0;

  // setup
  AngleUnit angle_ = AngleUnit::Deg;
  NormMode norm_ = NormMode::Norm1;
  bool dotMatrix_ = true;

  // exam
  bool exam_ = false;
  uint32_t examMs_ = 0;
  int offSequence_ = 0;  // SHIFT, 7, ON while off starts exam mode

  // platform
  bool online_ = false;
  std::string ssid_, keyHint_ = "not set", howToChange_ = "Connect USB, type wifi or key.";
  double (*random_)() = nullptr;
  bool blink_ = false;
  int battery_ = -1;
  bool charging_ = false;
  uint32_t nowMs_ = 0, lastKeyMs_ = 0;
  bool timeKnown_ = false;
  std::vector<std::string> notice_;
};

}  // namespace calc
