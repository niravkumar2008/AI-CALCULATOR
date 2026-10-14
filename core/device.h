// The whole calculator: a Casio fx-115ES style scientific calculator (COMP mode
// with CALC, SOLVE, ∫dx and d/dx; STAT as MODE 2; TABLE as MODE 3; EQN as MODE 5; CMPLX as
// MODE 6; BASE-N as MODE 7; MATRIX as MODE 8; VECTOR as MODE 9) with an Apps menu as mode 4 in the
// MODE menu (AI Solve by photo, Type it, History, Notes, Settings), SETUP options, and an exam mode
// that switches the AI and the apps off (STAT, TABLE, EQN, CMPLX, BASE-N, MATRIX, VECTOR, CALC,
// SOLVE, ∫ and d/dx stay: they are calculator features).
//
// Platform code (Windows simulator, web simulator, ESP32 firmware) feeds key
// presses, time and network results in, and draws what render() produces.
// Nothing here touches hardware, so every behaviour is unit-tested on a PC.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "app.h"
#include "basen.h"
#include "calc_engine.h"
#include "cmplx.h"
#include "eqn.h"
#include "framebuffer.h"
#include "geo.h"
#include "keys.h"  // DKey
#include "matvec.h"
#include "numeric.h"
#include "periodic.h"
#include "stat.h"
#include "table.h"

namespace calc {


// Typing on a computer keyboard or the Serial Monitor: one character = one
// key (or SHIFT + key). Digits . + - * / ^ ( ) = as printed; s c t sin cos tan,
// l log, n ln, r sqrt, q x², i x⁻¹, b Abs (SHIFT hyp), g logₐb, f fraction, ~ (−), E ×10ˣ, h hyp, k RCL,
// w S⇔D, M M+, a Ans, # DEL, $ AC, m MODE, o ON, [ SHIFT, ] ALPHA,
// < > u d arrows, p π, ! x!, % percent, , comma, C CALC, S SOLVE, I ∫dx, D d/dx,
// Q the "=" of an equation (ALPHA CALC), X Y the variables, j ENG (i in CMPLX), @ SHIFT (−)
// (∠ in CMPLX), " the °'" key (B in BASE-N). Returns false if unmapped.
bool keysForChar(char c, DKey out[2], int& n);

// The calculator mode (MODE 1-3, MODE 5-7), or Ai while AI SOLVE is open (MODE 4 > AI
// Solve). Stat / Table / Eqn / Cmplx come back after AI SOLVE or the Apps menu closes (mode() is
// then that mode). Eqn (4), Cmplx (5), BaseN (6), Matrix (7) and Vector (8) were added last:
// saveState() writes the value, older firmware refuses it.
enum class Mode : uint8_t { Comp, Ai, Stat, Table, Eqn, Cmplx, BaseN, Matrix, Vector };

// What fills the screen below the status bar.
enum class View : uint8_t {
  Off, Calc, Error, ModeMenu, Setup, Hyp, Clr, ClrConfirm, NetInfo, Look, ExamInfo, Notice, Ai,
  // MODE 4: the Apps menu and its apps (added at the end so the values above keep their numbers).
  Apps,      // carousel, one app per screen: appsSelected()
  Keyboard,  // on-screen keyboard: Type it, or writing a note (keyboardForNote())
  History,   // saved AI answers, newest first, then "Clear history": historySelected()
  Notes,     // "+ New note", then the notes: notesSelected()
  NoteView,  // one note (noteOpen()): 1:Edit 2:Delete
  Settings,  // the Settings app's list: settingsSelected()
  Confirm,   // a yes / no question (confirmQuestion()): = yes, AC no
  Geo,       // Geography app: Device::geo() (core/geo.*) has the state and draws rows 1-6
  Periodic,  // Periodic Table app: Device::periodic() (core/periodic.*) likewise
  Link,      // Settings > Link calculator: the link code (setLinkInfo())
  // fx-115ES calculator modes and tools (core/stat.h, core/table.h, core/numeric.h); calculator
  // screens (the classic LCD look on the v15), allowed in exam mode.
  Stat,      // MODE 2: Device::stat() has the state and draws rows 1-6
  Table,     // MODE 3: Device::table() likewise
  CalcRun,   // CALC: Device::calcRunner() (variable prompts, then the result; AC back to the expression)
  SolveRun,  // SHIFT CALC (SOLVE): Device::solveRunner() ("X?", then "X=" and "L−R=")
  SetupFreq,  // SETUP ▼ 1:STAT: "Frequency?" 1:ON 2:OFF
  Eqn,        // MODE 5: Device::eqn() (core/eqn.h) has the state and draws rows 1-6
  // MODE 6 CMPLX is View::Calc with complex values (core/cmplx.h); its two menus:
  CmplxMenu,   // SHIFT 2 in CMPLX: 1:arg 2:Conjg 3:▶r∠θ 4:▶a+bi
  SetupCmplx,  // SETUP ▼ 2:CMPLX: 1:a+bi 2:r∠θ (Device::complexPolar())
  // MODE 7 BASE-N is View::Calc with integers (core/basen.h); its menu:
  BaseMenu,    // SHIFT 3 in BASE-N: 1:and 2:or 3:xnor 4:xor 5:Not 6:Neg; ▼ 1:d 2:h 3:b 4:o (baseMenuPage())
  // MODE 8 MATRIX / MODE 9 VECTOR are View::Calc with matrices / vectors (core/matvec.h); their own
  // screens (Dim's lists, the editors, the MatAns / VctAns screen, SHIFT 4 / SHIFT 5's menu):
  MatVec       // Device::matVec() has the state and draws rows 1-6
};

// One cell of the on-screen keyboard (Device::keyboardCellAt()). The core draws
// row r (0-2) on text row 4 + r, starting at character column `col`, `width`
// characters wide; the cursor cell is inverted.
struct KeyCell {
  enum Kind : uint8_t { Type, Erase, Send };
  const char* label;  // what the cell shows ("A", "×", "SPACE", "DEL", "SEND"; SEND shows SAVE for notes)
  const char* text;   // what it types (Kind Type only), UTF-8
  uint8_t row, col, width;
  Kind kind;
};

class Device {
 public:
  static constexpr uint32_t kAutoOffMs = 10u * 60u * 1000u;     // like the Casio: ~10 min idle
  static constexpr uint32_t kExamMaxMs = 12u * 60u * 60u * 1000u;  // exam mode ends itself after 12 h

  Device();
  // The STAT / TABLE apps and the CALC / SOLVE runners hold pointers to this Device's
  // variables: a copy would share (and outlive) them.
  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

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
  // Also seeds the Geography quiz when the app first opens (without one: a fixed seed).
  void setRandomSource(double (*random)()) {
    random_ = random;
    randomSet_ = random != nullptr;
  }
  void setBlinkingCursor(bool on) { blink_ = on; }  // simulators blink; e-paper keeps it steady
  // 0-100, or -1 for no battery (tester, simulators). A battery icon shows
  // in the status bar at 20% or below, and always while charging.
  void setBattery(int percent, bool charging = false) {
    battery_ = percent;
    charging_ = charging;
  }

  // ---- Proofline splash: logo, loading bar and the step being done ----
  // startBoot() shows it from a cold start until finishBoot(); setBootStep()
  // moves the bar and names the step ("Starting camera"). With no steps the
  // bar fills with time. It stays up at least kSplashMinMs so the logo is
  // seen, and with setSplash(true) it also shows for kOnSplashMs after ON.
  // The calculator underneath keeps working: keys still reach it, and once
  // loading is done any key also skips the splash.
  static constexpr uint32_t kSplashMinMs = 1500;
  static constexpr uint32_t kSplashHoldMs = 400;  // full bar stays this long after finishBoot()
  static constexpr uint32_t kOnSplashMs = 800;
  void startBoot();
  void setBootStep(int percent, const std::string& step);
  void finishBoot();
  void setSplash(bool onPowerOn) { splashOnPowerOn_ = onPowerOn; }
  bool splashShowing() const;
  int splashPercent() const;  // 0-100, what the bar shows now
  const std::string& splashStep() const { return bootStep_; }  // "" = none

  // ---- exam mode ----
  bool examActive() const { return exam_; }
  uint32_t examElapsedMs() const { return examMs_; }
  bool radiosAllowed() const { return !exam_; }  // platform turns Wi-Fi and camera off when false
  void usbUnlock();                               // a teacher's USB cable ends exam mode
  void restoreExam(uint32_t elapsedMs);           // after a restart: resume exam mode silently
  // True after restoreState() / restoreExam() found a saved exam mode whose 12 h ran out
  // (e.g. while asleep): exam mode is off, and the platform should clear its own saved flag.
  bool examExpiredOnRestore() const { return examExpiredOnRestore_; }

  // ---- state kept while the chip deep-sleeps (RTC memory) ----
  // Memories A-F X Y M, Ans, replay history (newest first, as much as fits in maxBytes), SETUP
  // choices, mode, effort, tutor, exam mode and the off-key sequence. Not the screen contents.
  std::string saveState(size_t maxBytes = 2048) const;
  // False (and nothing changes) for a missing or damaged state. asleepMs counts towards exam mode's 12 h.
  bool restoreState(const std::string& state, uint32_t asleepMs = 0);

  // ---- MODE 4: Apps (docs/MODE4_APPS_SPEC.md) ----
  // MODE 4 opens the Apps menu: ◀ ▶ move (wrapping), 1-7 open that app, = opens
  // the selected one, AC goes back to the calculator with its expression intact.
  // In exam mode MODE 4 only shows "APPS ARE OFF" and nothing opens.
  // 0 AI Solve, 1 Type it, 2 History, 3 Notes, 4 Geography, 5 Periodic Table, 6 Settings (one table in device.cpp)
  static constexpr int kAppCount = 7;
  int appsSelected() const { return appsSel_; }  // 0..6, remembered (and saved by saveApps())
  static const char* appName(int i);              // "AI SOLVE" ... "" when out of range
  static const char* appDescription(int i);       // one line, at most 25 characters
  static View appView(int i);                      // the view the app opens (View::Off when out of range)

  // A one-line pop-up ("Saved", "Deleted", "Wi-Fi connected") boxed over any
  // screen for 1.5 s (by tick() time). A new one replaces the old one.
  static constexpr uint32_t kToastMs = 1500;
  void toast(const std::string& text);
  bool toastVisible() const;
  const std::string& toastText() const { return toast_; }  // the last one (check toastVisible())

  // Type it / notes keyboard: arrows move the cursor cell (wrapping), = presses
  // it, DEL erases, digit and operator keys type themselves, AC leaves (asking
  // first if the text changed). SEND (SAVE for a note) never sends an empty text.
  static constexpr int kMaxText = 160;  // characters, for typed problems and notes
  static int keyboardCellCount();
  static const KeyCell& keyboardCellAt(int i);
  int keyboardCell() const { return kbCell_; }               // the cursor cell's index
  const std::string& keyboardText() const { return kbText_; }  // UTF-8
  bool keyboardForNote() const { return kbForNote_; }         // false: Type it (SEND)
  // Type it's SEND renders the text as a PNG (png.h) and starts an AI request
  // like a photo: takeRequest() gives the id, then this gives the PNG to send
  // instead of a camera JPEG (true once per request; false = take a photo).
  bool takeTypedImage(std::string& png) { return app_.takeTypedImage(png); }
  // Only a platform that calls takeTypedImage() for every request may offer Type it: one
  // that doesn't would take a camera photo instead. Off by default: TYPE IT then shows
  // "Not available on this model" and nothing is typed or sent.
  void setTypedSolveSupported(bool on) { typedSolve_ = on; }
  bool typedSolveSupported() const { return typedSolve_; }

  // History: the last kMaxHistory AI answers (photo or typed), newest first.
  // = on one shows it again on the AI answer screen, read-only; AC goes back to the list.
  static constexpr int kMaxHistory = 20;
  struct SavedAnswer {
    std::string answer;     // the short answer, for the list
    std::string replyJson;  // Claude's reply (App::showSaved() shows it again)
  };
  // What one saved answer may take: a reply bigger than kMaxSavedJson is kept as just its
  // question and answer; the list text at most kMaxSavedAnswer bytes. saveApps() keeps its
  // whole blob within kMaxAppsBytes (the v15 keeps it in a 128 KB flash file system and
  // writes a new copy before deleting the old one): older answers are saved in that short
  // form, and the oldest are left out if it still doesn't fit.
  static constexpr size_t kMaxSavedJson = 2048;
  static constexpr size_t kMaxSavedAnswer = 256;
  static constexpr size_t kMaxAppsBytes = 40 * 1024;
  const std::vector<SavedAnswer>& savedAnswers() const { return saved_; }
  int historySelected() const { return histSel_; }  // 0..n-1 an answer, n = "Clear history"
  void clearHistory();  // e.g. when the calculator is unlinked; no confirm, no toast

  // Notes: up to kMaxNotes of up to kMaxText characters, newest first.
  static constexpr int kMaxNotes = 10;
  const std::vector<std::string>& notes() const { return notes_; }
  int notesSelected() const { return notesSel_; }  // 0 = "+ New note", i = notes()[i - 1]
  int noteOpen() const { return noteOpen_; }       // the note in View::NoteView (index into notes())
  int noteScroll() const { return noteTop_; }      // first wrapped line shown in View::NoteView

  // Settings: brightness and the sleep timer are the platform's (it reads them
  // whenever it likes and applies them); the rest reuse the SETUP screens.
  static int settingsCount();
  static const char* settingsItem(int i);  // "Brightness", "Sleep timer", "Wi-Fi & key", ...
  int settingsSelected() const { return setSel_; }
  int brightness() const { return brightness_; }  // 10-100 %, ◀ ▶ in steps of 10
  void setBrightness(int percent);                 // clamped to 10-100, rounded to 10
  int sleepMinutes() const { return sleepMin_; }  // screen sleep timer: 1, 2, 5, 10, 15 or 30
  void setSleepMinutes(int minutes);               // the nearest of those
  void setVersion(const std::string& v) { version_ = v; }  // shown in Settings > About
  // Settings > Link calculator (View::Link) shows `code` big, with "Go to <url>/link and
  // type this code." code "" = none to show: "Already linked" when linked, else "Connect
  // to Wi-Fi to get a link code". The platform calls it whenever the code changes (the
  // screen always shows the latest).
  void setLinkInfo(const std::string& code, const std::string& url, bool linked = false);
  const std::string& linkCode() const { return linkCode_; }
  const std::string& linkUrl() const { return linkUrl_; }
  bool linked() const { return linked_; }
  // A short plan line under "Already linked" on the Link screen ("Free month active",
  // "Plan ended: subscribe at proofline.app"; worker_api.h planStatusText()). Shown only
  // while linked; "" = none. Kept across setLinkInfo() calls.
  void setLinkPlan(const std::string& text) { linkPlan_ = text; }
  const std::string& linkPlan() const { return linkPlan_; }
  // First list row on screen in History / Notes / Settings (5 rows show at once).
  int listTop() const;
  static constexpr int kListRows = 5;

  std::string confirmQuestion() const;  // View::Confirm's question, e.g. "Delete this note?"

  // History, notes and the last app, for flash (NVS), separate from saveState().
  // restoreApps() returns false and changes nothing for missing or damaged data.
  // appsRevision() changes whenever history or notes change: save when it moves.
  std::string saveApps() const;
  bool restoreApps(const std::string& blob);
  uint32_t appsRevision() const { return appsRev_; }

  // ---- state, for the platform and tests ----
  bool isOff() const { return view_ == View::Off; }
  bool dotMatrix() const { return dotMatrix_; }
  Mode mode() const { return mode_ == Mode::Ai ? Mode::Ai : calcMode_; }  // Comp / Ai / Stat / ... / Matrix / Vector
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
  // The Geography and Periodic Table apps' state, for the v15 colour UI (View::Geo /
  // View::Periodic). They keep their place between visits (until power is lost).
  const geo::GeoApp& geo() const { return geo_; }
  const PeriodicApp& periodic() const { return periodic_; }
  // MODE 2 / MODE 3 and the CALC / SOLVE screens (View::Stat, Table, CalcRun, SolveRun).
  // STAT's data and TABLE's f(X) and range are kept across deep sleep (saveState()).
  const StatApp& stat() const { return stat_; }
  const TableApp& table() const { return table_; }
  // MODE 5 (View::Eqn): the type, the coefficients (kept across deep sleep) and the results.
  const EqnApp& eqn() const { return eqn_; }
  const numeric::CalcRunner& calcRunner() const { return calcRun_; }
  const numeric::SolveRunner& solveRunner() const { return solveRun_; }
  bool statFrequency() const { return stat_.frequency(); }  // SETUP ▼ 1:STAT (Frequency? ON / OFF)
  // MODE 6 CMPLX (core/cmplx.h): the COMP screen with complex values. Keys there: ENG types i,
  // SHIFT (−) ∠, SHIFT 2 opens arg / Conjg / ▶r∠θ / ▶a+bi (View::CmplxMenu), SHIFT hyp is Abs;
  // ∫dx, d/dx, CALC and SOLVE do nothing (choice: the real-only runners can't take i). Ans, the
  // memories and the replay keep complex values; leaving CMPLX keeps only their real parts and
  // clears the replay (as the Casio, whose replay is per mode). A non-real result shows as
  // complexPolar() says (or as a trailing ▶r∠θ / ▶a+bi asks); one too long for a row
  // (complexSplit()) shows its real part (or r) with "R⇔I", and SHIFT = swaps to the
  // imaginary part (or ∠θ): complexImagShown().
  bool complexPolar() const { return cmplxPolar_; }  // SETUP ▼ 2:CMPLX: false a+bi, true r∠θ
  const ImagVars& imagVars() const { return imag_; }  // the memories' imaginary parts (CMPLX)
  const Num& resultImag() const { return resultIm_; }  // the shown result's imaginary part (0 outside CMPLX)
  bool complexSplit() const;                           // the R⇔I indicator is on
  bool complexImagShown() const { return complexSplit() && cxShowIm_; }
  int setupPage() const { return setupPage_; }               // SETUP: 0, or 1 after ▼
  // MODE 7 BASE-N (core/basen.h): the COMP screen with 32-bit integers. Keys there: x² DEC,
  // x^□ HEX, log BIN, ln OCT (numberBase(); a shown result is converted, Math ERROR if it
  // doesn't fit BIN's 16 bits), (−) °'" hyp sin cos tan type A-F, SHIFT 3 opens the BASE menu
  // (View::BaseMenu, two pages). The keys BASE-N has no use for (functions, ".", ×10ˣ, S⇔D,
  // ENG, CALC, SOLVE, ∫dx ...) do nothing. Ans, the memories and the replay hold integers;
  // entering or leaving BASE-N clears the replay (the Casio's is per mode). Results show in
  // the current base, zero-padded (baseText()); the status bar shows Dec / Hex / Bin / Oct.
  Base numberBase() const { return base_; }  // DEC whenever BASE-N is entered (as the Casio)
  int baseMenuPage() const { return basePage_; }  // View::BaseMenu: 0 the logic page, 1 the prefixes
  // MODE 8 MATRIX / MODE 9 VECTOR (core/matvec.h): the COMP screen with matrices / vectors.
  // Entering opens Dim's "Matrix?" / "Vector?" list (View::MatVec; the variables cleared). SHIFT 4
  // (MATRIX) / SHIFT 5 (VECTOR) opens the menu; a matrix / vector result opens the MatAns / VctAns
  // screen (its expression stays on the calculation screen with no value: = shows it again, and
  // + − × ... carry on from MatAns). Scalar results (det, ·, Abs of a vector) show as in COMP and go
  // to Ans. ∫dx, d/dx, CALC and SOLVE do nothing; entering or leaving clears the replay.
  const MatVecApp& matVec() const { return mv_; }
  bool matrixResultShown() const;  // the calculation screen's result is a matrix / vector (no value shown)

 private:
  struct HistoryItem {
    std::vector<Tok> expr;
    Num value;
    Num im;           // CMPLX: the imaginary part (0 otherwise)
    int8_t form = -1;  // CMPLX: a trailing ▶a+bi (0) / ▶r∠θ (1); MATRIX / VECTOR: 2 a matrix / vector; else -1
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
  View backView() const;  // back_, unless what it shows has gone (a deleted note, exam mode)
  // The calculator's home view for the mode: Ai, Stat, Table or Calc.
  View modeView() const;
  // MODE 2 / MODE 3 (fresh, as on the Casio), and their keys.
  void enterStat();
  void enterTable();
  void enterEqn();  // MODE 5: the type list, the coefficients cleared
  void enterCmplx();  // MODE 6: the COMP screen with complex values, an empty expression
  // Every change of the calculator mode goes through here: leaving CMPLX drops the imaginary
  // parts (Ans, PreAns, A-M), and entering or leaving it clears the replay.
  void setCalcMode(Mode m);
  bool cmplx() const { return calcMode_ == Mode::Cmplx; }
  void enterBaseN();  // MODE 7: the COMP screen with integers, DEC, an empty expression
  bool baseN() const { return calcMode_ == Mode::BaseN; }
  void enterMatVec(bool vectors);  // MODE 8 / 9: the variables cleared, Dim's list
  bool isMatVec() const { return calcMode_ == Mode::Matrix || calcMode_ == Mode::Vector; }
  void equalsMatVec();
  void keyMatVec(DKey k, bool shift, bool alpha);
  void equalsBaseN();
  void setBase(Base b);  // DEC / HEX / BIN / OCT: a shown result is converted (Math ERROR if it doesn't fit)
  bool baseNKey(DKey k, bool shift, bool alpha) const;  // a key BASE-N uses (the rest do nothing)
  void equalsCmplx();
  bool valueForMemoryCx(Cx& out);  // CMPLX: the shown result or the evaluated expression
  void showCx(const Cx& v, int form, const std::string& label);  // form: -1 setting, 0 a+bi, 1 r∠θ
  int shownForm() const;  // the shown result's form: -1 a plain real, 0 a+bi, 1 r∠θ
  std::string cxFullText() const;
  void syncModes();  // the calculator's variables, angle unit, Norm mode, Ran# to STAT / TABLE
  void keyStat(DKey k, bool shift, bool alpha);
  void keyTable(DKey k, bool shift, bool alpha);
  void keyEqn(DKey k, bool shift, bool alpha);
  // CALC / SOLVE: start on the expression (nothing for an empty one), keys while one runs.
  void startRunner(bool solve);
  void keyRunner(DKey k, bool shift, bool alpha);
  void endRunner();  // back to editing the expression (Goto: the cursor where it went wrong)

  // Apps
  enum class AiFrom : uint8_t { Photo, Typed, History };     // what View::Ai is showing
  enum class Ask : uint8_t { DiscardText, DeleteNote, ClearHistory };
  void toCalc();               // AC from the Apps menu: the calculator, expression intact
  void openApp(int i);
  void keyApps(DKey k, int d);
  void keyKeyboard(DKey k, bool shift);
  void keyList(DKey k, int d);  // History, Notes, NoteView, Settings
  void keyConfirm(DKey k);
  void keyModule(DKey k, bool shift, bool alpha);  // View::Geo / View::Periodic
  void ask(Ask a);
  void typeText(const std::string& s);
  void eraseChar();
  void pressCell();
  void leaveKeyboard();         // after AC (or "Discard? =yes")
  void sendTyped();
  void saveNote();
  void openSetting(int i);
  void recordAnswer(const std::string& replyJson);
  int listRows() const;         // rows of the list on screen

  void drawStatus(Framebuffer& fb) const;
  void drawCalc(Framebuffer& fb) const;
  void drawMenu(Framebuffer& fb) const;
  void drawApps(Framebuffer& fb) const;
  void drawToast(Framebuffer& fb) const;
  void drawSplash(Framebuffer& fb) const;
  std::string engText() const;

  App app_;
  StatApp stat_;
  TableApp table_;
  EqnApp eqn_;
  MatVecApp mv_;
  numeric::CalcRunner calcRun_;
  numeric::SolveRunner solveRun_;
  Mode calcMode_ = Mode::Comp;  // Comp / Stat / Table / Eqn: the calculator mode under AI SOLVE and the apps
  int setupPage_ = 0;
  geo::GeoApp geo_;
  PeriodicApp periodic_;
  bool geoStarted_ = false, periodicStarted_ = false;  // reset() on the first open only
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
  Num resultIm_;         // CMPLX: the shown result's imaginary part
  int8_t resultForm_ = -1;  // CMPLX: forced form of the shown result (-1 = the SETUP choice); 2: a matrix / vector
  bool cxShowIm_ = false;   // R⇔I: the imaginary part (or ∠θ) is shown
  ImagVars imag_;           // CMPLX: the memories' imaginary parts
  bool cmplxPolar_ = false;  // SETUP ▼ 2:CMPLX
  Base base_ = Base::Dec;    // BASE-N's number mode
  int basePage_ = 0;         // View::BaseMenu's page
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
  bool examExpiredOnRestore_ = false;

  // platform
  bool online_ = false;
  std::string ssid_, keyHint_ = "not set", howToChange_ = "Connect USB, type wifi or key.";
  double (*random_)() = nullptr;
  bool randomSet_ = false;  // the platform gave a source (setRandomSource())
  bool blink_ = false;
  int battery_ = -1;
  bool charging_ = false;
  uint32_t nowMs_ = 0, lastKeyMs_ = 0;
  bool timeKnown_ = false;
  std::vector<std::string> notice_;

  // apps
  int appsSel_ = 0;
  AiFrom aiFrom_ = AiFrom::Photo;
  View infoBack_ = View::Setup;  // where Wi-Fi & key / Screen look / Exam mode go back to
  Ask ask_ = Ask::DiscardText;
  std::string toast_;
  bool toastOn_ = false;
  uint32_t toastSinceMs_ = 0;
  uint32_t appsRev_ = 0;
  bool typedSolve_ = false;  // setTypedSolveSupported()
  // keyboard
  bool kbForNote_ = false;
  int kbCell_ = 0;
  std::string kbText_, kbOriginal_;  // what's typed; what it was when the keyboard opened
  std::string typedDraft_;           // Type it's text, kept while solving it
  int editNote_ = -1;                // the note being edited, -1 = a new one
  // history, notes, settings
  std::vector<SavedAnswer> saved_;
  int histSel_ = 0;
  std::vector<std::string> notes_;
  int notesSel_ = 0, noteOpen_ = -1, noteTop_ = 0;
  int setSel_ = 0;
  int brightness_ = 70;
  int sleepMin_ = 5;
  std::string version_;
  std::string linkCode_, linkUrl_, linkPlan_;
  bool linked_ = false;

  // splash
  bool booting_ = false;
  bool splashOnPowerOn_ = false;
  uint32_t splashStart_ = 0, splashLen_ = 0;  // splashLen_ 0 = not showing (unless booting_)
  int bootPercent_ = -1;                      // -1 = no steps given: the bar follows time
  std::string bootStep_;
};

}  // namespace calc
