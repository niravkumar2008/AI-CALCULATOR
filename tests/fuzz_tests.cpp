// The fuzz campaign (2026-10-11), kept as a bounded, deterministic test: fixed seeds, the
// same invariants as the original harnesses, small enough for every build_and_run.bat
// (a few seconds). tools/fuzz/run_long.bat runs the same code with millions of steps.
//   device   random keys, time, network results, persistence and platform calls on a
//            Device; every invariant checked after every step (incl. B-1 ... B-11)
//   persist  mutated saveApps() / saveState() blobs: refused, or accepted and usable
//   png      encodePngGray() / renderTextPng() decoded by an independent decoder here
//   modules  Geography and Periodic Table on their own: random keys, every quiz filter
//   modes    STAT, TABLE and the CALC / SOLVE runners on their own (random keys and equations);
//            the device fuzzer also drives them (MODE 2 / 3, CALC, SHIFT CALC, ∫dx, SETUP ▼)
//   engine   random token lists: finite or an error, exact fractions consistent
// Hooked into tests.cpp by calling runFuzzTests(); returns the number of failures.
#include "fuzz_tests.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "../core/device.h"
#include "../core/font.h"
#include "../core/png.h"
#include "../core/solve_result.h"
#include "../core/text.h"

using namespace calc;

namespace fuzz {
namespace {

// xorshift64: the same sequence on every compiler, so a failure replays from its seed.
struct Rng {
  uint64_t s;
  explicit Rng(uint64_t seed) : s(seed ? seed : 88172645463325252ull) {}
  uint64_t next() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
  }
  int R(int n) { return n <= 0 ? 0 : static_cast<int>(next() % static_cast<uint64_t>(n)); }
  bool P(int pct) { return R(100) < pct; }
};

// Failures: each distinct one printed once (with the actions before it), all counted.
struct Report {
  bool verbose = false;
  std::map<std::string, int> fails;
  std::deque<std::string> trace;
  int count = 0;
  void tr(const std::string& s) {  // the last 60 actions (verbose: 400)
    trace.push_back(s);
    if (trace.size() > (verbose ? 400u : 60u)) trace.pop_front();
  }
  void fail(const std::string& what) {
    ++count;
    if (fails[what]++ == 0) {
      std::printf("FUZZ FAIL: %s\n", what.c_str());
      for (const auto& t : trace) std::printf("    %s\n", t.c_str());
    }
  }
};

Report g_rep;

bool validUtf8(const std::string& s) { return encodeUtf8(decodeUtf8(s)) == s; }

const char* viewName(View v) {
  static const char* n[] = {"Off",      "Calc",     "Error",    "ModeMenu", "Setup",   "Hyp",     "Clr",
                            "ClrConfirm", "NetInfo", "Look",    "ExamInfo", "Notice",  "Ai",      "Apps",
                            "Keyboard", "History",  "Notes",    "NoteView", "Settings", "Confirm", "Geo",
                            "Periodic", "Link",     "Stat",     "Table",    "CalcRun", "SolveRun", "SetupFreq",
                            "Eqn",      "CmplxMenu", "SetupCmplx", "BaseMenu", "MatVec"};
  static_assert(sizeof(n) / sizeof(n[0]) == static_cast<size_t>(View::MatVec) + 1, "one name per view");
  const int i = static_cast<int>(v);
  return i >= 0 && i <= static_cast<int>(View::MatVec) ? n[i] : "BAD";
}

bool finiteImag(const ImagVars& v) {
  for (const Num* n : {&v.a, &v.b, &v.c, &v.d, &v.e, &v.f, &v.x, &v.y, &v.m, &v.ans, &v.preAns})
    if (!std::isfinite(n->v) || !(std::fabs(n->v) < 1e100)) return false;
  return true;
}

// A BASE-N result as the base shows it: HEX 8, BIN 16, OCT 11 digits of the base, DEC an
// optional − and 1-10 digits.
bool baseTextOk(const std::string& s, Base b) {
  if (b == Base::Dec) {
    const std::string t = s.rfind("\xE2\x88\x92", 0) == 0 ? s.substr(3) : s;
    if (t.empty() || t.size() > 10 || (t.size() > 1 && t[0] == '0')) return false;
    for (char c : t)
      if (c < '0' || c > '9') return false;
    return true;
  }
  const size_t n = b == Base::Hex ? 8 : b == Base::Bin ? 16 : 11;
  const char* digits = b == Base::Hex ? "0123456789ABCDEF" : b == Base::Bin ? "01" : "01234567";
  if (s.size() != n || s.find_first_not_of(digits) != std::string::npos) return false;
  return b != Base::Oct || s[0] <= '3';  // 32 bits: 37777777777 at most
}

bool finiteVars(const Vars& v) {
  for (const Num* n : {&v.a, &v.b, &v.c, &v.d, &v.e, &v.f, &v.x, &v.y, &v.m, &v.ans, &v.preAns})
    if (!std::isfinite(n->v)) return false;
  return true;
}

bool badText(const std::string& s) {  // nan / inf, broken UTF-8, or wider than a row
  return s.find("nan") != std::string::npos || s.find("inf") != std::string::npos || !validUtf8(s) ||
         textLength(s) > Framebuffer::kCols;
}
bool isAppView(View v) {
  const int i = static_cast<int>(v);
  return i >= static_cast<int>(View::Apps) && i <= static_cast<int>(View::Link);
}

// ---------------------------------------------------------------- random inputs

std::string randText(Rng& g) {
  static const char* pool[] = {"", " ", "A", "x = 4.2 m", "\xC3\x97\xC3\xB7\xE2\x88\x9A\xCF\x80",
                               "\xF0\x9F\x98\x80 emoji", "\xC3", "\xFF\xFE\xFD", "\xE2\x88", "line1\nline2",
                               "quote \" back \\ slash", "tab\there", "(C) 42", "-3/4", "1,200 kg",
                               "\xE2\x88\x92" "5\xC3\x97" "10^-3", "NaN", "1e308*10", "\xC3\x9Cn\xC3\xAF" "c\xC3\xB6" "d\xC3\xA9"};
  const int k = g.R(10);
  if (k < 6) return pool[g.R(sizeof(pool) / sizeof(pool[0]))];
  std::string s;
  const int n = k == 9 ? g.R(9000) : g.R(300);
  for (int i = 0; i < n; ++i) {
    const int c = g.R(10);
    if (c < 6) s += static_cast<char>('a' + g.R(26));
    else if (c < 7) s += ' ';
    else if (c < 8) s += static_cast<char>(g.R(256));
    else if (c < 9) s += "\xC3\x97";
    else s += static_cast<char>('0' + g.R(10));
  }
  return s;
}

std::string jstr(const std::string& s) {
  std::string o = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += static_cast<char>(c);
    else if (c == '\n') o += "\\n";
    else if (c < 0x20) {
      char b[8];
      std::snprintf(b, sizeof b, "\\u%04x", c);
      o += b;
    } else o += static_cast<char>(c);
  }
  return o + "\"";
}

std::string randExpr(Rng& g) {
  static const char* pool[] = {"2*sqrt(3)+sin(30)^2", "1/0", "((((((((((1", "2^3^2", "", "abc", "-5", "sin(",
                               "3!", "10^400", "1/3", "2(3)", "sqrt(-1)", "ln(0)", "5C2", "e^pi", "1.5E3",
                               "1+", ")(", "x+1"};
  if (g.P(85)) return pool[g.R(sizeof(pool) / sizeof(pool[0]))];
  std::string s;
  const int n = g.R(400);
  for (int i = 0; i < n; ++i) s += "1+"[i % 2];
  return s;
}

std::string randReply(Rng& g) {
  const int k = g.R(20);
  if (k == 0) return randText(g);
  if (k == 1) return "{";
  if (k == 2) return std::string(200, '[');
  if (k == 3) return "{\"readable\":true}";
  if (k == 4) return "[1,2]";
  std::string o = "{";
  bool first = true;
  auto add = [&](const std::string& key, const std::string& val) {
    if (!first) o += ",";
    first = false;
    o += jstr(key) + ":" + val;
  };
  if (g.P(80)) add("readable", g.P(85) ? "true" : (g.P(50) ? "false" : "\"yes\""));
  if (g.P(80)) {
    char b[64];
    const double c = g.P(10) ? -5 : g.P(10) ? 7 : (g.R(1001) / 1000.0);
    std::snprintf(b, sizeof b, "%g", c);
    add("confidence", g.P(5) ? "1e999" : b);
  }
  if (g.P(70)) add("read_as", jstr(g.P(10) ? std::string(5000, 'Q') : randText(g)));
  if (g.P(90)) add("answer", jstr(g.P(5) ? std::string(3000, 'Z') + "\xC3\x97" : randText(g)));
  if (g.P(30)) add("choice", jstr(g.P(50) ? std::string(1, static_cast<char>('A' + g.R(5))) : randText(g)));
  if (g.P(50)) add("expression", jstr(randExpr(g)));
  if (g.P(50)) add("check", jstr(randExpr(g)));
  if (g.P(50)) {
    std::string a = "[";
    const int n = g.R(g.P(5) ? 200 : 6);
    for (int i = 0; i < n; ++i) a += (i ? "," : "") + jstr(randText(g));
    add("steps", a + "]");
  }
  if (g.P(30)) {
    std::string a = "[";
    const int n = g.R(4);
    for (int i = 0; i < n; ++i) a += (i ? "," : "") + jstr(randText(g));
    add("unclear", a + "]");
  }
  o += "}";
  if (g.P(3)) o.resize(o.size() / 2);
  return o;
}

DKey randKey(Rng& g) {
  static const DKey hot[] = {DKey::Mode, DKey::D4, DKey::Eq, DKey::AC, DKey::Up, DKey::Down, DKey::Left,
                             DKey::Right, DKey::D1, DKey::D2, DKey::D3, DKey::D5, DKey::D6, DKey::D7,
                             DKey::Del, DKey::Shift};
  if (g.P(60)) return hot[g.R(sizeof(hot) / sizeof(hot[0]))];
  return static_cast<DKey>(g.R(static_cast<int>(DKey::Count)));
}

// Device::setRandomSource() takes a plain function: these read the session's generator.
Rng* g_devRng = nullptr;
double goodRandom() { return (g_devRng->next() >> 11) * (1.0 / 9007199254740992.0); }
double badRandom() { return g_devRng->P(50) ? -1.0 : 2.0; }

bool validPng(const std::string& png) {
  return png.size() > 8 + 25 + 12 && std::memcmp(png.data(), "\x89PNG\r\n\x1a\n", 8) == 0;
}

// ---------------------------------------------------------------- device: invariants

struct Ctx {
  Device d;
  uint32_t now = 0;
  uint32_t lastToastOrigin = 0;  // the last action that could raise a toast
};

// A result: an a+bi may take two rows (the core splits it), not more.
bool eqnBadText(const std::string& s) {
  return s.empty() || s.find("nan") != std::string::npos || s.find("inf") != std::string::npos || !validUtf8(s) ||
         textLength(s) > 2 * Framebuffer::kCols;
}

// EQN's state: the cursor in the grid, the coefficients holdable, the results shown sound.
void checkEqn(const EqnApp& e, const std::string& where) {
  if (e.curRow() < 0 || e.curRow() >= e.rows() || e.curCol() < 0 || e.curCol() >= e.cols())
    g_rep.fail(where + "EQN cursor out of range");
  for (int r = 0; r < e.rows(); ++r)
    for (int c = 0; c < e.cols(); ++c)
      if (!(std::fabs(e.coef(r, c)) < 1e100)) g_rep.fail(where + "EQN coefficient not holdable");
  if (e.resultCount() < 0 || e.resultCount() > EqnApp::kMaxResults) g_rep.fail(where + "EQN result count out of range");
  if (e.screen() == EqnApp::Screen::Result) {
    if (e.resultCount() == 0 || e.resultIndex() < 0 || e.resultIndex() >= e.resultCount())
      g_rep.fail(where + "EQN result index out of range");
    for (int i = 0; i < e.resultCount(); ++i)
      if (eqnBadText(e.resultText(i)) || !std::isfinite(e.result(i).value.re) || !std::isfinite(e.result(i).value.im))
        g_rep.fail(where + "EQN result bad: " + e.resultText(i));
  }
}

// MATRIX / VECTOR's state: the variables' sizes the mode can hold, elements in range, the editor's
// cursor inside its variable.
void checkMatVec(const MatVecApp& m, const std::string& where) {
  for (int i = 0; i < 4; ++i) {
    const Mat& x = m.var(i);
    if (!x.defined()) {
      if (x.cols != 0) g_rep.fail(where + "MATRIX / VECTOR: columns without rows");
      continue;
    }
    if (x.rows > 3 || x.cols < 1 || x.cols > 3) g_rep.fail(where + "MATRIX / VECTOR: a size out of range");
    if (m.vectors() && (x.rows != 1 || x.cols < 2)) g_rep.fail(where + "VECTOR: not a 2D / 3D vector");
    for (int r = 0; r < x.rows; ++r)
      for (int c = 0; c < x.cols; ++c) {
        const Num& n = x.at(r, c);
        if (!std::isfinite(n.v) || !(std::fabs(n.v) < 1e100) || (n.exact && n.den <= 0))
          g_rep.fail(where + "MATRIX / VECTOR: an element out of range");
      }
  }
  if (m.screen() == MatVecApp::Screen::Editor) {
    const Mat& x = m.var(m.editing());
    if (!x.defined() || m.curRow() < 0 || m.curRow() >= x.rows || m.curCol() < 0 || m.curCol() >= x.cols)
      g_rep.fail(where + "MATRIX / VECTOR: editor cursor out of range");
  }
  if (m.dimPage() < 0 || m.dimPage() > 1) g_rep.fail(where + "MATRIX: dimPage out of range");
  if (static_cast<int>(m.entry().size()) > kMaxTokens) g_rep.fail(where + "MATRIX / VECTOR: entry > 99 tokens");
}

void checkInvariants(Ctx& c) {
  Device& d = c.d;
  const View v = d.view();
  if (static_cast<int>(v) > static_cast<int>(View::MatVec)) g_rep.fail("view out of enum range");
  // MATRIX / VECTOR: their screens only in their mode; nothing of theirs outside it; a matrix
  // result (no value on the calculation screen) only there.
  const bool mv = d.mode() == Mode::Matrix || d.mode() == Mode::Vector;
  if (v == View::MatVec && !mv) g_rep.fail("MatVec view outside MATRIX / VECTOR");
  if (mv && d.matVec().vectors() != (d.mode() == Mode::Vector)) g_rep.fail("MATRIX / VECTOR flag disagrees with the mode");
  if (!mv && d.mode() != Mode::Ai && d.matVec().var(0).defined()) g_rep.fail("a matrix kept outside MATRIX / VECTOR");
  if (d.matrixResultShown() && !mv) g_rep.fail("a matrix result outside MATRIX / VECTOR");
  checkMatVec(d.matVec(), "");
  // BASE-N: its menu only in BASE-N; a shown result is the base's zero-padded digits (an integer
  // that fits it); the number mode is DEC outside BASE-N.
  if (v == View::BaseMenu && d.mode() != Mode::BaseN) g_rep.fail("BaseMenu view outside BASE-N mode");
  if (d.baseMenuPage() < 0 || d.baseMenuPage() > 1) g_rep.fail("baseMenuPage out of range");
  if (d.mode() != Mode::BaseN && d.mode() != Mode::Ai && d.numberBase() != Base::Dec) g_rep.fail("a number mode outside BASE-N");
  if (d.mode() == Mode::BaseN && v == View::Calc && d.showingResult() && !baseTextOk(d.resultText(), d.numberBase()))
    g_rep.fail("BASE-N result text bad: " + d.resultText());
  // CMPLX: its menu only in CMPLX; imaginary parts only while in CMPLX (or AI SOLVE over it).
  if (v == View::CmplxMenu && d.mode() != Mode::Cmplx) g_rep.fail("CmplxMenu view outside CMPLX mode");
  if (d.mode() != Mode::Cmplx && d.mode() != Mode::Ai && (d.imagVars().any() || d.resultImag().v != 0))
    g_rep.fail("an imaginary part outside CMPLX");
  if (!finiteImag(d.imagVars())) g_rep.fail("an imaginary part is not finite");
  // STAT / TABLE / CALC / SOLVE: their views only in their mode / while running.
  if (v == View::Stat && d.mode() != Mode::Stat) g_rep.fail("Stat view outside STAT mode");
  if (v == View::Table && d.mode() != Mode::Table) g_rep.fail("Table view outside TABLE mode");
  if (v == View::Eqn && d.mode() != Mode::Eqn) g_rep.fail("Eqn view outside EQN mode");
  if (v == View::CalcRun && !d.calcRunner().active()) g_rep.fail("CalcRun view without a running CALC");
  if (v == View::SolveRun && !d.solveRunner().active()) g_rep.fail("SolveRun view without a running SOLVE");
  if ((v == View::CalcRun || v == View::SolveRun) && d.mode() != Mode::Comp) g_rep.fail("CALC / SOLVE outside COMP");
  if (d.mode() == Mode::Ai && (v == View::Stat || v == View::Table || v == View::Eqn || v == View::MatVec))
    g_rep.fail("mode Ai on a STAT / TABLE / EQN screen");
  if (d.setupPage() < 0 || d.setupPage() > 1) g_rep.fail("setupPage out of range");
  {
    const StatApp& s = d.stat();
    if (s.rows() < 0 || s.rows() > s.maxRows()) g_rep.fail("STAT rows out of range");
    if (s.curRow() < 0 || s.curRow() > s.rows() || s.curCol() < 0 || s.curCol() >= s.columns())
      g_rep.fail("STAT editor cursor out of range");
    if (s.showingResult() && badText(s.resultText())) g_rep.fail("STAT result text bad: " + s.resultText());
    const TableApp& t = d.table();
    if (t.rows() < 0 || t.rows() > TableApp::kMaxRows) g_rep.fail("TABLE rows out of range");
    if (t.screen() == TableApp::Screen::Table && (t.cursorRow() < 0 || t.cursorRow() >= t.rows()))
      g_rep.fail("TABLE cursor out of range");
    if (static_cast<int>(t.function().size()) > kMaxTokens) g_rep.fail("TABLE f(X) > 99 tokens");
    checkEqn(d.eqn(), "");
    for (const numeric::RunnerBase* r : {static_cast<const numeric::RunnerBase*>(&d.calcRunner()),
                                         static_cast<const numeric::RunnerBase*>(&d.solveRunner())}) {
      if (!r->active()) continue;
      const numeric::Screen sc = r->screen();
      for (const auto& row : sc.rows)
        if (badText(row)) g_rep.fail("CALC / SOLVE row bad: " + row);
      if (sc.big.find("nan") != std::string::npos || sc.big.find("inf") != std::string::npos)
        g_rep.fail("CALC / SOLVE value shows nan / inf");
    }
  }
  if (!finiteVars(d.vars())) g_rep.fail("a variable is not finite (CALC / SOLVE / STAT)");
  if (d.examActive()) {
    if (isAppView(v)) g_rep.fail(std::string("exam mode shows app view ") + viewName(v));
    if (d.mode() == Mode::Ai) g_rep.fail("exam mode with mode Ai");
    if (v == View::Ai) g_rep.fail("exam mode shows AI view");
    if (d.ai().activeRequest() != 0) g_rep.fail("exam mode with an AI request in flight");
  }
  // B-10: AI mode (the status bar's "AI") only on AI SOLVE and the menus over it.
  if (d.mode() == Mode::Ai && isAppView(v)) g_rep.fail(std::string("mode Ai in app view ") + viewName(v));
  if (textLength(d.keyboardText()) > Device::kMaxText) g_rep.fail("keyboardText > 160");
  if (static_cast<int>(d.notes().size()) > Device::kMaxNotes) g_rep.fail("notes > 10");
  for (const auto& n : d.notes()) {
    if (textLength(n) > Device::kMaxText) g_rep.fail("a note > 160 chars");
    if (blankText(n)) g_rep.fail("a blank note was saved");
  }
  if (static_cast<int>(d.savedAnswers().size()) > Device::kMaxHistory) g_rep.fail("history > 20");
  for (const auto& s : d.savedAnswers()) {
    if (s.replyJson.size() > Device::kMaxSavedJson) g_rep.fail("a saved reply > kMaxSavedJson (B-6)");
    if (s.answer.size() > Device::kMaxSavedAnswer) g_rep.fail("a saved answer > kMaxSavedAnswer");
  }
  if (d.appsSelected() < 0 || d.appsSelected() >= Device::kAppCount) g_rep.fail("appsSelected out of range");
  if (d.historySelected() < 0 || d.historySelected() > static_cast<int>(d.savedAnswers().size()))
    g_rep.fail("historySelected out of range");
  if (d.notesSelected() < 0 || d.notesSelected() > static_cast<int>(d.notes().size())) g_rep.fail("notesSelected out of range");
  if (d.settingsSelected() < 0 || d.settingsSelected() >= Device::settingsCount()) g_rep.fail("settingsSelected out of range");
  if (d.keyboardCell() < 0 || d.keyboardCell() >= Device::keyboardCellCount()) g_rep.fail("keyboardCell out of range");
  const bool noteOk = d.noteOpen() >= 0 && d.noteOpen() < static_cast<int>(d.notes().size());
  if (v == View::NoteView && !noteOk) g_rep.fail("NoteView with an invalid noteOpen (B-3)");
  if (v == View::Confirm && d.confirmQuestion() == "Delete this note?" && !noteOk) g_rep.fail("Confirm delete with invalid noteOpen");
  if (v == View::Keyboard && !d.keyboardForNote() && !d.typedSolveSupported()) g_rep.fail("Type it open without support (B-9)");
  if (d.brightness() < 10 || d.brightness() > 100 || d.brightness() % 10) g_rep.fail("brightness out of range");
  {
    const int s = d.sleepMinutes();
    if (s != 1 && s != 2 && s != 5 && s != 10 && s != 15 && s != 30) g_rep.fail("sleep minutes invalid");
  }
  if (d.listTop() < 0) g_rep.fail("listTop < 0");
  if (d.toastVisible()) {
    if (c.now - c.lastToastOrigin >= Device::kToastMs) g_rep.fail("toast still visible after 1.5 s");
    if (textLength(d.toastText()) > Framebuffer::kCols - 2) g_rep.fail("toast text wider than 23");
  }
  if (d.cursor() < 0 || d.cursor() > static_cast<int>(d.expression().size())) g_rep.fail("calc cursor out of range");
  if (static_cast<int>(d.expression().size()) > kMaxTokens) g_rep.fail("expression > 99 tokens");
  const std::string r = d.resultText();
  if (r.find("nan") != std::string::npos || r.find("inf") != std::string::npos) g_rep.fail("result shows nan / inf (B-7)");
  if (textLength(r) > Framebuffer::kCols) g_rep.fail("result wider than the screen (B-4)");
  if (!std::isfinite(d.vars().ans.v)) g_rep.fail("Ans not finite");
}

// One persistence round trip inside a session: both blobs must restore, apps within budget.
void persistTrip(Rng& g, Device& d) {
  const std::string blob = d.saveApps();
  if (blob.size() > Device::kMaxAppsBytes) g_rep.fail("saveApps() over kMaxAppsBytes (B-6)");
  Device d2;
  if (!d2.restoreApps(blob)) {
    g_rep.fail("restoreApps(saveApps()) refused on a fresh device");
  } else {
    if (d2.notes() != d.notes()) g_rep.fail("notes differ after round trip");
    if (d2.savedAnswers().size() != d.savedAnswers().size()) g_rep.fail("history size differs after round trip");
    for (size_t q = 0; q < d.savedAnswers().size() && q < d2.savedAnswers().size(); ++q) {
      const auto& a = d.savedAnswers()[q];
      const auto& b = d2.savedAnswers()[q];
      if (a.answer != b.answer) g_rep.fail("history answer differs after round trip");
      if (a.replyJson != b.replyJson) {  // allowed only as the short form (B-6), still readable
        SolveResult sr;
        std::string err;
        if (!parseSolveResult(b.replyJson, sr, err) || !sr.readable) g_rep.fail("history reply unreadable after round trip");
      }
    }
    if (d2.saveApps() != d2.saveApps()) g_rep.fail("saveApps() not deterministic");
  }
  if (g.P(30) && !d.restoreApps(blob)) g_rep.fail("restoreApps(saveApps()) refused on the same device");
  const std::string st = d.saveState();
  Device d3;
  if (!d3.restoreState(st)) g_rep.fail("restoreState(saveState()) refused");
  if (g.P(20) && !d.restoreState(st, g.P(50) ? 0 : static_cast<uint32_t>(g.R(50000000))))
    g_rep.fail("restoreState(own) refused");
}

void deviceSession(Rng& g, int steps, uint64_t& typedSends) {
  Ctx c;
  g_rep.trace.clear();
  g_devRng = &g;
  static Framebuffer fb;
  Device& d = c.d;
  std::deque<DKey> queue;
  if (g.P(50)) d.setRandomSource(g.P(90) ? goodRandom : badRandom);
  d.setBlinkingCursor(g.P(50));
  const bool typedOk = g.P(70);  // most sessions the v15 (Type it), the rest a v14
  d.setTypedSolveSupported(typedOk);
  const int start = g.R(4);
  c.now = start == 0 ? 0 : start == 1 ? g.R(100000) : start == 2 ? 0xFFFFFFFFu - g.R(20000) : static_cast<uint32_t>(g.next());
  d.tick(c.now);
  c.lastToastOrigin = c.now;
  d.setOnline(g.P(65));
  const int failsAtStart = g_rep.count;
  for (int i = 0; i < steps && g_rep.count == failsAtStart; ++i) {
    const View preView = d.view();
    const Screen preAi = d.ai().screen();
    const bool preNote = d.keyboardForNote();
    const std::string preText = d.keyboardText();
    const int preCell = d.keyboardCell();
    bool keyed = false, scanned = false;
    int a = g.R(100);
    char buf[160];
    if (!queue.empty() || (d.view() == View::Off && g.P(70))) a = 0;
    if (a < 62) {
      DKey k = randKey(g);
      if (!queue.empty()) {
        k = queue.front();
        queue.pop_front();
      } else if (d.view() == View::Off && g.P(70)) {
        k = g.P(10) ? (g.P(50) ? DKey::Shift : DKey::D7) : DKey::On;
      }
      d.onKey(k);
      keyed = true;
      c.lastToastOrigin = c.now;
      std::snprintf(buf, sizeof buf, "key %s (was %s)", dkeyName(k), viewName(preView));
      g_rep.tr(buf);
    } else if (a < 72) {
      const uint32_t dt = g.P(80) ? g.R(2000) : g.P(50) ? g.R(700000) : static_cast<uint32_t>(g.next() % 50000000u);
      c.now += dt;
      d.tick(c.now);
      g_rep.tr("tick +" + std::to_string(dt));
    } else if (a < 74) {
      const bool on = g.P(60);
      d.setOnline(on);
      g_rep.tr(std::string("setOnline ") + (on ? "1" : "0"));
    } else if (a < 76) {
      const int b = g.R(10) == 0 ? g.R(300) - 100 : g.R(101);
      d.setBattery(b, g.P(30));
      g_rep.tr("setBattery " + std::to_string(b));
    } else if (a < 77) {
      const std::string code = g.P(50) ? std::string() : randText(g).substr(0, g.P(50) ? 8 : 400);
      d.setLinkInfo(code, g.P(50) ? "https://proofline.app/" : randText(g), g.P(50));
      g_rep.tr("setLinkInfo");
    } else if (a < 85) {  // the platform answers the request in flight (or a stale / random id)
      const int id = g.P(85) ? d.activeRequest() : g.R(5);
      const int w = g.R(10);
      if (w < 2) {
        d.onCaptured(id);
        g_rep.tr("onCaptured " + std::to_string(id));
      } else if (w < 7) {
        const std::string j = randReply(g);
        d.onReply(id, j);
        g_rep.tr("onReply " + std::to_string(id) + " len " + std::to_string(j.size()));
      } else if (w < 9) {
        d.onFailure(id, static_cast<Failure>(g.R(9)), g.P(50) ? "" : randText(g));
        g_rep.tr("onFailure " + std::to_string(id));
      } else {
        d.onPartialAnswer(id, g.R(101) / 100.0, randText(g));
        g_rep.tr("onPartialAnswer " + std::to_string(id));
      }
    } else if (a < 86) {
      scanned = true;
      const bool ok = d.startScan();
      g_rep.tr(std::string("startScan ") + (ok ? "1" : "0"));
    } else if (a < 87) {
      d.usbUnlock();
      g_rep.tr("usbUnlock");
    } else if (a < 88) {
      const uint32_t ms = g.P(50) ? g.R(1000000) : g.P(50) ? Device::kExamMaxMs - g.R(3) : static_cast<uint32_t>(g.next());
      d.restoreExam(ms);
      g_rep.tr("restoreExam " + std::to_string(ms));
    } else if (a < 90) {
      persistTrip(g, d);
      g_rep.tr("persist round trip");
    } else if (a < 91) {
      d.toast(randText(g));
      c.lastToastOrigin = c.now;
      g_rep.tr("toast()");
    } else if (a < 92) {
      d.setLowBattery(g.P(30));
      d.setHasApiKey(g.P(85));
      g_rep.tr("setLowBattery / setHasApiKey");
    } else if (a < 93) {
      d.setBrightness(g.R(400) - 100);
      d.setSleepMinutes(g.R(100) - 20);
      g_rep.tr("setBrightness / setSleepMinutes");
    } else if (a < 94) {
      d.setHoldUntil(c.now + g.R(5000));
      g_rep.tr("setHoldUntil");
    } else if (a < 95) {
      if (g.P(30)) d.clearHistory();
      d.setNetInfo(randText(g), randText(g), randText(g));
      d.setVersion(randText(g).substr(0, 40));
      g_rep.tr("clearHistory / setNetInfo / setVersion");
    } else {  // a burst of keys deep into the apps: MODE 4 then something
      using K = DKey;
      static const std::vector<std::vector<DKey>> seqs = {
          {K::Mode, K::D4, K::D2, K::Eq}, {K::Mode, K::D4, K::D4, K::Eq}, {K::Mode, K::D4, K::D3, K::Eq},
          {K::Mode, K::D4, K::D7, K::D2}, {K::Mode, K::D4, K::D5, K::Eq}, {K::Mode, K::D4, K::D6, K::D5},
          {K::Mode, K::D4, K::D7, K::D6}, {K::Mode, K::D4, K::D1, K::Eq}, {K::Mode, K::D4, K::D7, K::D4},
          {K::Mode, K::D4, K::D4, K::Eq, K::D1, K::D2, K::Add, K::D3, K::Up, K::Right, K::Right, K::Eq},
          {K::Mode, K::D4, K::D4, K::Down, K::Eq, K::D1, K::D5, K::Up, K::Right, K::Right, K::Eq},
          {K::Mode, K::D4, K::D4, K::Down, K::Eq, K::D2, K::Eq},
          {K::Mode, K::D4, K::D4, K::Down, K::Down, K::Eq, K::Down, K::Down},
          {K::Mode, K::D4, K::D2, K::D1, K::Add, K::D2, K::Up, K::Right, K::Right, K::Eq},
          {K::Mode, K::D4, K::D3, K::Eq, K::Down, K::Down, K::AC},
          {K::Mode, K::D4, K::D3, K::Up, K::Up, K::Eq, K::Eq},
          {K::Mode, K::D4, K::D7, K::D6, K::Eq},
          // the B-3 / B-8 shapes: MODE over a note, delete it, MODE from Link / an error
          {K::Mode, K::D4, K::D4, K::Down, K::Eq, K::Mode, K::AC, K::D2, K::Eq, K::AC, K::D7, K::D4, K::Mode, K::AC},
          {K::On, K::D1, K::Div, K::D0, K::Eq, K::Mode, K::AC},
          {K::D1, K::D2, K::D3, K::Eq, K::Eng, K::Eng, K::Eng, K::Eng, K::Shift, K::Eng},
          // Type it: SEND, then = again / ON then = (B-2) / MODE 4 while it solves (B-10)
          {K::Mode, K::D4, K::D2, K::D5, K::Up, K::Right, K::Right, K::Eq, K::Eq},
          {K::Mode, K::D4, K::D2, K::D7, K::Up, K::Right, K::Right, K::Eq, K::On, K::Eq},
          {K::Mode, K::D4, K::D2, K::D8, K::Up, K::Right, K::Right, K::Eq, K::Mode, K::D4, K::D3},
          // STAT, TABLE, CALC, SOLVE, ∫, d/dx, SETUP ▼ Frequency (and the apps over them)
          {K::Mode, K::D2, K::D1, K::D3, K::Eq, K::D5, K::Eq, K::AC, K::Shift, K::D1, K::D4, K::D2, K::Eq},
          {K::Mode, K::D2, K::D2, K::D1, K::Eq, K::D2, K::Eq, K::Right, K::D4, K::Eq, K::D7, K::Eq, K::AC, K::Shift,
           K::D1, K::D5, K::D3, K::Eq, K::Shift, K::D1, K::D2},
          {K::Mode, K::D2, K::D3, K::Shift, K::D1, K::D3, K::D1, K::Up, K::Del, K::Shift, K::D1, K::D3, K::D2},
          {K::Mode, K::D3, K::Alpha, K::Close, K::Sq, K::Eq, K::Eq, K::Eq, K::Eq, K::Down, K::Down, K::Right, K::AC},
          {K::Mode, K::D3, K::Alpha, K::Close, K::Eq, K::D1, K::Eq, K::D9, K::D9, K::Eq, K::Eq},  // Insufficient MEM
          {K::Alpha, K::Close, K::Sq, K::Sub, K::D2, K::Alpha, K::Calc, K::D0, K::Shift, K::Calc, K::D1, K::Eq, K::Eq,
           K::Neg, K::D1, K::Eq, K::Left},
          {K::Alpha, K::Close, K::Sq, K::Add, K::D1, K::Shift, K::Calc, K::Eq, K::Right},  // Can't Solve
          {K::Alpha, K::SD, K::Alpha, K::Calc, K::Alpha, K::Close, K::Add, K::D1, K::Calc, K::D2, K::Eq, K::Calc},
          {K::Integral, K::Alpha, K::Close, K::Sq, K::Shift, K::Close, K::D0, K::Shift, K::Close, K::D1, K::Eq},
          {K::Shift, K::Integral, K::Alpha, K::Close, K::Shift, K::Sq, K::Shift, K::Close, K::D2, K::Eq},
          {K::Shift, K::Integral, K::Shift, K::Hyp, K::Alpha, K::Close, K::Shift, K::Close, K::D0, K::Eq},  // a corner
          {K::Shift, K::Mode, K::Down, K::D1, K::D1, K::Mode, K::D2, K::D2},
          {K::Shift, K::Mode, K::Down, K::D1, K::D2, K::Mode, K::D2, K::D1, K::D4, K::Eq, K::Mode, K::D4, K::D1},
          {K::Mode, K::D2, K::D1, K::D2, K::Eq, K::Shift, K::Mode, K::D8, K::Eq, K::AC, K::Mode, K::D4},
      };
      const auto& s = seqs[static_cast<size_t>(g.R(static_cast<int>(seqs.size())))];
      g_rep.tr("queue MODE 4 ...");
      for (DKey k : s) queue.push_back(k);
    }
    // The platform's side of a request.
    const int id = d.takeRequest();
    std::string png;
    const bool typed = d.takeTypedImage(png);
    if (id == 0 && typed) g_rep.fail("takeTypedImage true without a request");
    if (id != 0) {
      if (d.examActive()) g_rep.fail("AI request started in exam mode");
      if (typed) {
        ++typedSends;
        if (!typedOk) g_rep.fail("typed image on a platform without Type it (B-9)");
        if (!validPng(png)) g_rep.fail("typed PNG empty or invalid");
        // Legit: SEND on Type it's keyboard, or = (send again) on its answer or error. Not
        // = on the AI home screen (B-2: that takes a photo), not startScan().
        const bool legit = keyed && ((preView == View::Keyboard && !preNote && preCell == Device::keyboardCellCount() - 1) ||
                                     (preView == View::Ai && preAi != Screen::Ready && preAi != Screen::Busy));
        if (scanned) g_rep.fail("startScan() sent a typed image instead of a photo");
        else if (!legit) g_rep.fail(std::string("typed image sent without SEND / resend from ") + viewName(preView));
        if (preView == View::Keyboard && blankText(preText)) g_rep.fail("blank text sent");
      }
      g_rep.tr("  -> request " + std::to_string(id) + (typed ? " (typed)" : " (photo)"));
    }
    d.render(fb);
    checkInvariants(c);
  }
  g_devRng = nullptr;
}

// ---------------------------------------------------------------- persistence mutants

std::string jq(const std::string& s) {
  std::string o = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += static_cast<char>(c);
    else if (c < 0x20) o += ' ';
    else o += static_cast<char>(c);
  }
  return o + "\"";
}

void keys(Device& d, std::initializer_list<DKey> ks) {
  for (DKey k : ks) d.onKey(k);
}

// A device with answers (some long, some multi-byte), notes and memories, via its keys.
void populate(Rng& g, Device& d) {
  d.tick(1000);
  d.setOnline(true);
  d.setTypedSolveSupported(true);
  const int answers = g.R(25);
  for (int i = 0; i < answers; ++i) {
    d.startScan();
    const int id = d.takeRequest();
    d.onCaptured(id);
    const std::string ans = g.P(30) ? std::string(g.R(2000), 'x') + "\xC3\x97\xCF\x80" : "x = " + std::to_string(g.R(1000));
    const std::string ra = g.P(20) ? std::string(4000 + g.R(4000), 'r') : "read";
    d.onReply(id, "{\"readable\":true,\"confidence\":0.9,\"answer\":" + jq(ans) + ",\"choice\":" + jq(g.P(30) ? "B" : "") +
                      ",\"read_as\":" + jq(ra) + ",\"steps\":[\"a\",\"b\"],\"expression\":\"1+2\",\"check\":\"3\"}");
    d.onKey(DKey::AC);
  }
  const int notes = g.R(13);
  for (int i = 0; i < notes; ++i) {
    keys(d, {DKey::On, DKey::Mode, DKey::D4, DKey::D4, DKey::Eq});
    const int n = 1 + g.R(170);
    for (int k = 0; k < n; ++k) d.onKey(g.P(50) ? DKey::D1 : g.P(50) ? DKey::Mul : DKey::Sqrt);  // 1, ×, √
    keys(d, {DKey::Up, DKey::Right, DKey::Right, DKey::Eq});
  }
  keys(d, {DKey::On, DKey::D1, DKey::Add, DKey::D2, DKey::Eq, DKey::Shift, DKey::Rcl, DKey::Neg});
  keys(d, {DKey::D1, DKey::Div, DKey::D3, DKey::Eq, DKey::Shift, DKey::Rcl, DKey::Dms});
  // Now and then STAT with data (paired, sometimes with FREQ), TABLE with an f(X), EQN with
  // coefficients or CMPLX with complex memories and replay: saveState()'s mode block.
  const int m = g.R(8);
  if (m == 1) {
    if (g.P(50)) keys(d, {DKey::Shift, DKey::Mode, DKey::Down, DKey::D1, DKey::D1});  // Frequency ON
    keys(d, {DKey::On, DKey::Mode, DKey::D2, g.P(50) ? DKey::D1 : DKey::D2});
    const int rows = g.R(30);
    for (int i = 0; i < rows; ++i) {
      if (g.P(10)) d.onKey(DKey::Neg);
      d.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(3)));
      if (g.P(20)) keys(d, {DKey::Dot, DKey::D5});
      d.onKey(DKey::Eq);
      if (g.P(30)) d.onKey(g.P(50) ? DKey::Right : DKey::Left);
    }
    if (g.P(50)) d.onKey(DKey::AC);
  } else if (m == 2) {
    keys(d, {DKey::On, DKey::Mode, DKey::D3, DKey::Alpha, DKey::Close, DKey::Sq, DKey::Add, DKey::D1});
    if (g.P(50)) keys(d, {DKey::Eq, DKey::Neg, DKey::D2, DKey::Eq, DKey::D3, DKey::Eq, DKey::Eq});
  } else if (m == 3) {
    keys(d, {DKey::On, DKey::Mode, DKey::D5});
    if (g.P(80)) d.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(4)));  // 1-4: a type
    const int cells = g.R(13);
    for (int i = 0; i < cells; ++i) {
      if (g.P(10)) d.onKey(DKey::Neg);
      d.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(3)));
      if (g.P(15)) keys(d, {DKey::Exp10, DKey::D9, DKey::D9});
      d.onKey(DKey::Eq);
    }
    if (g.P(30)) d.onKey(DKey::Eq);  // solve
  } else if (m == 4) {
    keys(d, {DKey::On, DKey::Mode, DKey::D6});
    if (g.P(30)) keys(d, {DKey::Shift, DKey::Mode, DKey::Down, DKey::D2, g.P(50) ? DKey::D1 : DKey::D2});
    const int items = 1 + g.R(8);
    for (int i = 0; i < items; ++i) {
      d.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(9)));
      if (g.P(50)) keys(d, {DKey::Add, static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(9)), DKey::Eng});
      if (g.P(30)) keys(d, {DKey::Shift, DKey::Neg, DKey::D3, DKey::D0});  // ∠30
      if (g.P(20)) keys(d, {DKey::Shift, DKey::D2, static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(4))});
      d.onKey(DKey::Eq);
      if (g.P(30)) keys(d, {DKey::Shift, DKey::Rcl, static_cast<DKey>(g.R(static_cast<int>(DKey::Count)))});
      if (g.P(20)) d.onKey(DKey::MPlus);
    }
  } else if (m == 5) {  // BASE-N: a number mode, integers (A-F too), logic, a prefix, memories
    keys(d, {DKey::On, DKey::Mode, DKey::D7});
    static const DKey baseKeys[] = {DKey::Sq, DKey::Pow, DKey::Log, DKey::Ln};
    const int items = 1 + g.R(8);
    for (int i = 0; i < items; ++i) {
      if (g.P(40)) d.onKey(baseKeys[g.R(4)]);
      d.onKey(static_cast<DKey>(static_cast<int>(DKey::D0) + g.R(2)));
      if (g.P(40)) d.onKey(g.P(50) ? DKey::Sin : DKey::Neg);  // D, A
      if (g.P(40)) keys(d, {DKey::Shift, DKey::D3, static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(6))});
      if (g.P(20)) keys(d, {DKey::Shift, DKey::D3, DKey::Down, static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(4))});
      d.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(9)));
      d.onKey(DKey::Eq);
      if (g.P(30)) keys(d, {DKey::Shift, DKey::Rcl, static_cast<DKey>(g.R(static_cast<int>(DKey::Count)))});
      if (g.P(20)) d.onKey(DKey::MPlus);
    }
  } else if (m >= 6) {  // MATRIX / VECTOR: variables of random sizes, then calculations (MatAns too)
    const bool vec = m == 7;
    const DKey menu = vec ? DKey::D5 : DKey::D4;
    keys(d, {DKey::On, DKey::Mode, vec ? DKey::D9 : DKey::D8, DKey::AC});
    const int n = g.R(4);
    for (int v = 0; v < n; ++v) {
      keys(d, {DKey::Shift, menu, DKey::D1, static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(3))});
      if (!vec && g.P(30)) d.onKey(DKey::Down);
      d.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(vec ? 2 : 3)));
      for (int i = 0; i < 9; ++i) {
        if (g.P(10)) d.onKey(DKey::Neg);
        d.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(9)));
        if (g.P(15)) keys(d, {DKey::Div, DKey::D7});
        if (g.P(5)) keys(d, {DKey::Exp10, DKey::D9, DKey::D9});
        d.onKey(DKey::Eq);
      }
      d.onKey(DKey::AC);
    }
    const int items = g.R(6);
    for (int i = 0; i < items; ++i) {
      keys(d, {DKey::Shift, menu, static_cast<DKey>(static_cast<int>(DKey::D3) + g.R(4))});
      if (g.P(60)) keys(d, {g.P(50) ? DKey::Mul : DKey::Add, DKey::Shift, menu, static_cast<DKey>(static_cast<int>(DKey::D3) + g.R(4))});
      if (g.P(20)) d.onKey(vec ? DKey::Mul : DKey::Inv);
      d.onKey(DKey::Eq);
      if (g.P(50)) d.onKey(DKey::AC);
    }
  }
}

std::string mutate(Rng& g, std::string s) {
  int k = g.R(6);
  if (s.empty()) k = 3;
  switch (k) {
    case 0: {  // bit flips
      const int n = 1 + g.R(4);
      for (int i = 0; i < n; ++i) s[static_cast<size_t>(g.R(static_cast<int>(s.size())))] ^= static_cast<char>(1 << g.R(8));
      break;
    }
    case 1: s.resize(static_cast<size_t>(g.R(static_cast<int>(s.size())))); break;
    case 2: {
      const int n = 1 + g.R(64);
      for (int i = 0; i < n; ++i) s += static_cast<char>(g.R(256));
      break;
    }
    case 3: {
      s.clear();
      const int n = g.R(200);
      for (int i = 0; i < n; ++i) s += static_cast<char>(g.R(256));
      break;
    }
    case 4: {  // overwrite a run of bytes
      const int at = g.R(static_cast<int>(s.size()));
      const int n = 1 + g.R(8);
      for (int i = 0; i < n && at + i < static_cast<int>(s.size()); ++i) s[static_cast<size_t>(at + i)] = static_cast<char>(g.R(256));
      break;
    }
    default: {  // a byte set to an edge value
      static const unsigned char e[] = {0, 1, 0x7F, 0x80, 0xFF, 20, 21, 10, 11, 7, 6};
      s[static_cast<size_t>(g.R(static_cast<int>(s.size())))] = static_cast<char>(e[g.R(sizeof e)]);
    }
  }
  return s;
}

std::string withAdler(std::string body) {
  const uint32_t a = pngAdler32(reinterpret_cast<const uint8_t*>(body.data()), body.size());
  for (int i = 0; i < 4; ++i) body += static_cast<char>((a >> (8 * i)) & 0xFF);
  return body;
}

void exercise(Rng& g, Device& d, int n) {  // random keys and renders after a restore
  static const DKey hot[] = {DKey::Mode, DKey::D4, DKey::D3, DKey::D2, DKey::Eq, DKey::AC, DKey::Up, DKey::Down,
                             DKey::Left, DKey::Right, DKey::D1, DKey::Shift, DKey::Sq, DKey::Eng, DKey::SD,
                             DKey::Rcl, DKey::Neg, DKey::MPlus, DKey::Add, DKey::Ans, DKey::D5};
  static Framebuffer fb;
  for (int i = 0; i < n; ++i) {
    d.onKey(hot[g.R(sizeof(hot) / sizeof(hot[0]))]);
    d.render(fb);
    const std::string r = d.resultText();
    if (r.find("nan") != std::string::npos || r.find("inf") != std::string::npos) g_rep.fail("restored state shows nan / inf");
  }
}

// Each iteration: one populated device, 20 apps mutants and 20 state mutants.
void persistMutants(Rng& g, int iterations, int& acceptedApps, int& acceptedState) {
  static Framebuffer fb;
  for (int it = 0; it < iterations; ++it) {
    g_rep.trace.clear();
    g_rep.tr("persist iteration " + std::to_string(it));
    Device src;
    populate(g, src);
    const std::string apps = src.saveApps();
    const std::string state = src.saveState(g.P(50) ? 2048 : static_cast<size_t>(g.R(300)));
    if (apps.size() > Device::kMaxAppsBytes) g_rep.fail("saveApps() over kMaxAppsBytes (B-6)");
    {
      Device d;
      if (!d.restoreApps(apps)) g_rep.fail("restoreApps refused its own blob");
      if (d.notes() != src.notes()) g_rep.fail("notes changed in round trip");
      if (d.savedAnswers().size() != src.savedAnswers().size()) g_rep.fail("history size changed in round trip");
      Device s;
      if (!s.restoreState(state)) g_rep.fail("restoreState refused its own state");
    }
    for (int m = 0; m < 20; ++m) {
      std::string bad = mutate(g, apps);
      if (g.P(70) && bad.size() >= 4) bad = withAdler(bad.substr(0, bad.size() - 4));  // reach the parser
      Device d;
      if (g.P(50)) populate(g, d);
      const auto beforeNotes = d.notes();
      const size_t beforeSaved = d.savedAnswers().size();
      if (!d.restoreApps(bad)) {
        if (d.notes() != beforeNotes || d.savedAnswers().size() != beforeSaved) g_rep.fail("restoreApps false but changed state");
        continue;
      }
      ++acceptedApps;
      if (d.notes().size() > 10 || d.savedAnswers().size() > 20) g_rep.fail("restoreApps accepted too many");
      for (const auto& n : d.notes())
        if (textLength(n) > 160 || blankText(n)) g_rep.fail("restoreApps accepted a bad note");
      for (const auto& s : d.savedAnswers())
        if (s.replyJson.size() > Device::kMaxSavedJson || s.answer.size() > Device::kMaxSavedAnswer)
          g_rep.fail("restoreApps kept an oversized answer");
      keys(d, {DKey::On, DKey::Mode, DKey::D4, DKey::D3});  // browse everything restored
      for (size_t i = 0; i <= d.savedAnswers().size(); ++i) {
        d.render(fb);
        d.onKey(DKey::Down);
      }
      keys(d, {DKey::Up, DKey::Eq});
      d.render(fb);
      keys(d, {DKey::AC, DKey::AC, DKey::D4});
      for (size_t i = 0; i <= d.notes().size(); ++i) {
        d.render(fb);
        d.onKey(DKey::Down);
      }
      keys(d, {DKey::Eq});
      for (int k = 0; k < 40; ++k) {
        d.onKey(DKey::Down);
        d.render(fb);
      }
      keys(d, {DKey::D1});
      d.render(fb);
      exercise(g, d, 50);
    }
    for (int m = 0; m < 20; ++m) {
      std::string bad = mutate(g, state);
      // saveState() ends with an Adler-32 (version 2): re-sign most mutants to reach the parser.
      if (g.P(70) && bad.size() >= 4) bad = withAdler(bad.substr(0, bad.size() - 4));
      Device d;
      d.tick(5);
      if (!d.restoreState(bad, g.P(50) ? 0 : static_cast<uint32_t>(g.next()))) continue;
      ++acceptedState;
      if (d.examActive() && d.view() != View::Calc && d.view() != View::Stat && d.view() != View::Table &&
          d.view() != View::Eqn && d.view() != View::Off)
        g_rep.fail("restoreState: exam with an odd view");
      if (!finiteVars(d.vars())) g_rep.fail("restoreState accepted a non-finite variable");
      if (d.stat().rows() > d.stat().maxRows()) g_rep.fail("restoreState accepted too many STAT rows");
      for (int i = 0; i < d.stat().rows(); ++i)
        if (!(std::fabs(d.stat().x(i)) < 1e100) || !(std::fabs(d.stat().y(i)) < 1e100) || !(d.stat().freq(i) >= 0))
          g_rep.fail("restoreState accepted a bad STAT value");
      if (static_cast<int>(d.table().function().size()) > kMaxTokens) g_rep.fail("restoreState accepted a long f(X)");
      checkEqn(d.eqn(), "restoreState: ");
      d.render(fb);
      keys(d, {DKey::On});
      // Recall every memory and do maths with it (B-1: an exact LLONG_MIN trapped here).
      static const DKey vars[] = {DKey::Neg, DKey::Dms, DKey::Hyp, DKey::Sin, DKey::Cos, DKey::Tan,
                                  DKey::Close, DKey::SD, DKey::MPlus};
      for (DKey v : vars) {
        keys(d, {DKey::Rcl, v});
        const std::string r = d.resultText();
        if (r.find("nan") != std::string::npos || r.find("inf") != std::string::npos)
          g_rep.fail("restoreState accepted a non-finite memory (B-7)");
        keys(d, {DKey::AC, DKey::Alpha, v, DKey::Mul, DKey::D3, DKey::Add, DKey::Alpha, v, DKey::Div, DKey::D7, DKey::Eq});
        d.render(fb);
        keys(d, {DKey::SD, DKey::Eng, DKey::AC, DKey::Shift, DKey::Hyp, DKey::Alpha, v, DKey::Eq, DKey::AC});
        d.render(fb);
      }
      keys(d, {DKey::Up, DKey::Up, DKey::Down});
      d.render(fb);
      exercise(g, d, 50);
    }
  }
}

// ---------------------------------------------------------------- PNG: an independent decoder

uint32_t crcBitwise(const std::string& s, size_t from, size_t n) {
  uint32_t c = 0xFFFFFFFFu;
  for (size_t i = from; i < from + n; ++i) {
    c ^= static_cast<uint8_t>(s[i]);
    for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return ~c;
}
uint32_t be32(const std::string& s, size_t p) {
  return (uint32_t(uint8_t(s[p])) << 24) | (uint32_t(uint8_t(s[p + 1])) << 16) | (uint32_t(uint8_t(s[p + 2])) << 8) |
         uint32_t(uint8_t(s[p + 3]));
}

// Decodes an 8-bit grey, non-interlaced PNG whose zlib stream uses stored blocks (what
// encodePngGray() writes): signature, chunk CRCs, IHDR, zlib header, block lengths and
// their complements, Adler-32, filter bytes. "" = fine, else what was wrong.
std::string decodeGray(const std::string& png, int w, int h, std::vector<uint8_t>& px) {
  if (png.size() < 8 || png.compare(0, 8, std::string("\x89PNG\r\n\x1a\n", 8)) != 0) return "signature";
  size_t p = 8;
  std::string idat;
  bool ihdr = false, iend = false;
  while (p + 12 <= png.size()) {
    const uint32_t len = be32(png, p);
    if (p + 12 + len > png.size()) return "chunk overruns the file";
    const std::string type = png.substr(p + 4, 4);
    if (crcBitwise(png, p + 4, 4 + len) != be32(png, p + 8 + len)) return "CRC of " + type;
    if (type == "IHDR") {
      if (ihdr || len != 13) return "IHDR";
      if (int(be32(png, p + 8)) != w || int(be32(png, p + 12)) != h) return "IHDR size";
      if (png.compare(p + 16, 5, std::string("\x08\x00\x00\x00\x00", 5)) != 0) return "IHDR format";
      ihdr = true;
    } else if (type == "IDAT") {
      if (!ihdr) return "IDAT before IHDR";
      idat += png.substr(p + 8, len);
    } else if (type == "IEND") {
      iend = true;
    }
    p += 12 + len;
  }
  if (!ihdr || !iend || p != png.size()) return "chunk order / trailing bytes";
  if (idat.size() < 6) return "zlib too short";
  const uint8_t cmf = uint8_t(idat[0]), flg = uint8_t(idat[1]);
  if ((cmf & 0x0F) != 8 || ((cmf << 8) | flg) % 31 != 0 || (flg & 0x20)) return "zlib header";
  std::string raw;
  size_t q = 2;
  for (bool last = false; !last;) {
    if (q >= idat.size()) return "deflate runs out";
    const uint8_t hdr = uint8_t(idat[q]);
    last = hdr & 1;
    if ((hdr >> 1) & 3) return "not a stored block";
    if (q + 5 > idat.size()) return "stored header";
    const uint32_t n = uint8_t(idat[q + 1]) | (uint32_t(uint8_t(idat[q + 2])) << 8);
    const uint32_t nn = uint8_t(idat[q + 3]) | (uint32_t(uint8_t(idat[q + 4])) << 8);
    if ((n ^ 0xFFFFu) != nn) return "stored LEN / NLEN";
    if (q + 5 + n > idat.size()) return "stored block overruns";
    raw += idat.substr(q + 5, n);
    q += 5 + n;
  }
  if (q + 4 != idat.size()) return "zlib trailer";
  uint32_t a = 1, b = 0;
  for (unsigned char c : raw) {
    a = (a + c) % 65521;
    b = (b + a) % 65521;
  }
  if (((b << 16) | a) != be32(idat, q)) return "Adler-32";
  if (raw.size() != size_t(w + 1) * size_t(h)) return "scanline bytes";
  px.clear();
  px.reserve(size_t(w) * size_t(h));
  for (int y = 0; y < h; ++y) {
    if (raw[size_t(y) * (w + 1)] != 0) return "filter byte";
    px.insert(px.end(), raw.begin() + size_t(y) * (w + 1) + 1, raw.begin() + size_t(y + 1) * (w + 1));
  }
  return "";
}

void pngRoundTrips(Rng& g, int images, int& checked) {
  struct Sz { int w, h; };
  std::vector<Sz> sizes = {{1, 1}, {1, 65534}, {65535, 1}, {65534, 1}, {256, 256}, {3, 21845}, {7, 9363}, {200, 328}};
  for (int i = 0; i < images; ++i) sizes.push_back({1 + g.R(400), 1 + g.R(400)});
  for (const Sz& sz : sizes) {
    std::vector<uint8_t> px(size_t(sz.w) * size_t(sz.h));
    for (auto& v : px) v = static_cast<uint8_t>(g.R(256));
    const std::string png = encodePngGray(px, sz.w, sz.h);
    std::vector<uint8_t> back;
    const std::string why = decodeGray(png, sz.w, sz.h, back);
    if (!why.empty()) g_rep.fail("PNG " + std::to_string(sz.w) + "x" + std::to_string(sz.h) + ": " + why);
    else if (back != px) g_rep.fail("PNG pixels differ after decoding");
    ++checked;
  }
  if (!encodePngGray({}, 0, 0).empty() || !encodePngGray(std::vector<uint8_t>(4), 2, 3).empty() ||
      !encodePngGray(std::vector<uint8_t>(4), -2, -2).empty())
    g_rep.fail("encodePngGray accepted a bad size");
  // Type it's renders: blank text gives nothing; anything else decodes to its own pixels.
  std::vector<std::string> texts = {std::string(160, 'W'), "   ", "", "\n\n", "a", "1+2", "\xC3\x97\xC3\xB7\xE2\x88\x9A\xCF\x80",
                                    "line\nbreak", std::string(80, '\xCF'), "x" + std::string(159, ' '),
                                    "\xF0\x9F\x98\x80\xF0\x9F\x98\x80", "Solve 2x+3=7 for x",
                                    std::string(20, 'A') + " " + std::string(20, 'B')};
  {
    std::string t;
    while (textLength(t) + 12 <= 160) t += "ABCDEFGHIJK ";
    texts.push_back(t);
    std::string r;
    for (int i = 0; i < 160; ++i) r += "\xE2\x88\x9A";
    texts.push_back(r);
  }
  for (const std::string& t : texts) {
    std::vector<uint8_t> want, got;
    int w = 0, h = 0;
    const bool ok = renderTextImage(t, want, w, h, 3);
    const std::string png = renderTextPng(t);
    if (ok == png.empty()) g_rep.fail("renderTextImage / renderTextPng disagree");
    if (ok == blankText(t)) g_rep.fail("renderTextImage ok != !blankText");
    if (!ok) continue;
    const std::string why = decodeGray(png, w, h, got);
    if (!why.empty()) g_rep.fail("typed PNG: " + why);
    else if (got != want) g_rep.fail("typed PNG pixels differ");
    ++checked;
  }
}

// ---------------------------------------------------------------- Geography / Periodic alone

void moduleKeys(Rng& g, int steps) {
  using geo::GeoApp;
  Framebuffer fb;
  GeoApp ga;
  PeriodicApp pa;
  ga.setSeed(static_cast<uint32_t>(g.next()));
  ga.reset();
  pa.reset();
  for (int s = 0; s < steps; ++s) {
    if (s % 3000 == 0) {  // a fresh start now and then
      ga.setSeed(static_cast<uint32_t>(g.next()));
      ga.reset();
      pa.reset();
    }
    const DKey k = static_cast<DKey>(g.R(static_cast<int>(DKey::Count)));
    const bool sh = g.P(15), al = g.P(5);
    ga.onKey(k, sh, al);
    pa.onKey(k, sh, al);
    if (ga.wantsExit()) ga.resume();
    if (pa.wantsExit()) pa.resume();
    fb.clear();
    ga.render(fb);
    fb.clear();
    pa.render(fb);
    if (ga.selected() < 0 || ga.selected() >= geo::countryCount()) g_rep.fail("geo selected out of range");
    if (pa.cursor() < 1 || pa.cursor() > kElementCount) g_rep.fail("periodic cursor out of range");
    if (ga.searchPrefix().size() > 12) g_rep.fail("geo search prefix > 12");
    if (ga.screen() == GeoApp::Screen::Quiz) {
      const auto& q = ga.quiz();
      if (q.correct < 0 || q.correct > 3) g_rep.fail("geo quiz correct out of range");
      for (int c = 0; c < 4; ++c) {
        if (q.choices[c] < 0 || q.choices[c] >= geo::countryCount()) g_rep.fail("geo quiz choice out of range");
        for (int e = c + 1; e < 4; ++e)
          if (q.choices[c] == q.choices[e]) g_rep.fail("geo quiz duplicate choice");
      }
    }
  }
  // Every filter's quiz, many questions each.
  for (int f = 0; f < GeoApp::kFilterCount; ++f) {
    ga.reset();
    for (int k = 0; k < f; ++k) ga.onKey(DKey::D4);
    ga.onKey(DKey::D5);
    for (int qn = 0; qn < steps / 100; ++qn) {
      ga.onKey(static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(3)));
      ga.onKey(DKey::Eq);
      const auto& q = ga.quiz();
      for (int c = 0; c < 4; ++c)
        for (int e = c + 1; e < 4; ++e)
          if (q.choices[c] == q.choices[e]) g_rep.fail("geo quiz duplicate choice (filter " + std::to_string(f) + ")");
    }
  }
}

// ---------------------------------------------------------------- STAT / TABLE / CALC / SOLVE alone

double quarter() { return 0.25; }

// A short random expression for CALC / SOLVE: mostly X, digits and operators, sometimes "=",
// a comma, a bracket or (CALC only: SOLVE would integrate on every iteration) ∫( / d/dx(.
std::vector<Tok> randCalcExpr(Rng& g, bool calculus) {
  static const Tok pool[] = {Tok::VarX, Tok::VarX, Tok::VarY, Tok::VarA, Tok::D1, Tok::D2, Tok::D3, Tok::D0,
                             Tok::Add,  Tok::Sub,  Tok::Mul,  Tok::Div,  Tok::Sq, Tok::Pow, Tok::Sqrt, Tok::Ln,
                             Tok::Sin,  Tok::Close, Tok::Comma, Tok::Eq, Tok::Dot, Tok::Neg, Tok::Abs, Tok::Inv};
  std::vector<Tok> t;
  const int n = 1 + g.R(12);
  for (int i = 0; i < n; ++i) {
    if (calculus && g.P(6)) t.push_back(g.P(50) ? Tok::Integ : Tok::Deriv);
    else t.push_back(pool[g.R(sizeof(pool) / sizeof(pool[0]))]);
  }
  return t;
}

void modeKeys(Rng& g, int steps) {
  static Framebuffer fb;
  Vars vars;
  StatApp st;
  TableApp tb;
  EqnApp eq;
  MatVecApp mv;
  numeric::CalcRunner cr;
  numeric::SolveRunner sr;
  const AngleUnit unit = static_cast<AngleUnit>(g.R(3));
  const NormMode norm = static_cast<NormMode>(g.R(2));
  st.setEnv(&vars, unit, norm, quarter);
  tb.setEnv(&vars, unit, norm, quarter);
  eq.setEnv(&vars, unit, norm, quarter);
  mv.setEnv(&vars, unit, norm, quarter);
  for (int s = 0; s < steps; ++s) {
    if (s % 2000 == 0) {  // a fresh start now and then
      if (g.P(40)) st.setFrequency(g.P(50));
      st.reset();
      tb.reset();
      eq.reset();
      mv.reset(g.P(50));
      vars = Vars();
    }
    const DKey k = g.P(30) ? (g.P(50) ? DKey::Eq : static_cast<DKey>(static_cast<int>(DKey::D1) + g.R(9)))
                           : static_cast<DKey>(g.R(static_cast<int>(DKey::Count)));
    const bool sh = g.P(15), al = g.P(5);
    st.onKey(k, sh, al);
    if (st.wantsExit()) st.resume();
    tb.onKey(k, sh, al);
    if (tb.wantsExit()) tb.resume();
    eq.onKey(k, sh, al);
    if (eq.wantsExit()) eq.resume();
    mv.onKey(k, sh, al);
    {
      Tok t;
      if (mv.takeAction(t) != MatVecApp::Action::None && g.P(30)) mv.openMenu();  // the host's calculation screen
    }
    for (int which = 0; which < 2; ++which) {
      numeric::RunnerBase& r = which ? static_cast<numeric::RunnerBase&>(sr) : static_cast<numeric::RunnerBase&>(cr);
      if (!r.active()) {
        if (g.P(8)) {
          const std::vector<Tok> e = randCalcExpr(g, which == 0);
          if (which) sr.start(e, vars, unit, norm, quarter);
          else cr.start(e, vars, unit, norm, quarter);
        }
        continue;
      }
      if (g.P(25)) {
        static const Tok typed[] = {Tok::D1, Tok::D2, Tok::Neg, Tok::Dot, Tok::Add, Tok::VarX, Tok::Sq, Tok::Exp10};
        r.insert(typed[g.R(sizeof(typed) / sizeof(typed[0]))]);
      } else if (which) {
        sr.onKey(k, sh);
      } else {
        cr.onKey(k, sh);
      }
    }
    fb.clear();
    st.render(fb);
    fb.clear();
    tb.render(fb);
    fb.clear();
    eq.render(fb);
    fb.clear();
    mv.render(fb);
    fb.clear();
    cr.render(fb);
    fb.clear();
    sr.render(fb);
    if (st.rows() < 0 || st.rows() > st.maxRows()) g_rep.fail("modes: STAT rows out of range");
    if (st.curRow() < 0 || st.curRow() > st.rows()) g_rep.fail("modes: STAT cursor out of range");
    if (st.showingResult() && badText(st.resultText())) g_rep.fail("modes: STAT result bad");
    if (tb.rows() < 0 || tb.rows() > TableApp::kMaxRows) g_rep.fail("modes: TABLE rows out of range");
    checkEqn(eq, "modes: ");
    checkMatVec(mv, "modes: ");
    if (!finiteVars(vars)) g_rep.fail("modes: a variable is not finite");
    for (const numeric::RunnerBase* r : {static_cast<const numeric::RunnerBase*>(&cr), static_cast<const numeric::RunnerBase*>(&sr)}) {
      if (!r->active()) continue;
      const numeric::Screen sc = r->screen();
      for (const auto& row : sc.rows)
        if (badText(row)) g_rep.fail("modes: CALC / SOLVE row bad: " + row);
      if (sc.big.find("nan") != std::string::npos || sc.big.find("inf") != std::string::npos)
        g_rep.fail("modes: CALC / SOLVE shows nan / inf");
    }
  }
}

// ---------------------------------------------------------------- calculator engine soup

double half() { return 0.5; }

void engineSoup(Rng& g, int lists) {
  for (int i = 0; i < lists; ++i) {
    std::vector<Tok> t;
    const int len = 1 + g.R(99);
    for (int k = 0; k < len; ++k) t.push_back(static_cast<Tok>(g.R(static_cast<int>(Tok::Count))));
    Vars v;
    v.a = Num::ofInt(g.R(1000) - 500);
    v.b = Num::ofDouble(1e99);
    v.c = Num::ofDouble(-1e-99);
    const EvalResult r = evaluate(t, v, static_cast<AngleUnit>(g.R(3)), half);
    if (r.error != CalcError::None) continue;
    if (!std::isfinite(r.value.v)) g_rep.fail("evaluate: non-finite value without an error");
    if (r.value.exact && r.value.den <= 0) g_rep.fail("evaluate: exact value with den <= 0");
    if (r.value.exact && std::fabs(static_cast<double>(r.value.num) / r.value.den - r.value.v) >
                             1e-6 * std::max(1.0, std::fabs(r.value.v)))
      g_rep.fail("evaluate: exact num/den disagrees with v: " + exprText(t));
    const std::string s = formatResult(r.value, g.P(50), static_cast<NormMode>(g.R(2)));
    if (s.empty() || s.find("nan") != std::string::npos || s.find("inf") != std::string::npos) g_rep.fail("bad format '" + s + "'");
  }
}

// CMPLX: random token lists (i, angle, arg(, Conjg(, the two conversions ... among them) and
// complex memories through evaluateComplex(); every result in range and formatted soundly.
void complexSoup(Rng& g, int lists) {
  static const Tok cx[] = {Tok::ImagI, Tok::Angle, Tok::Arg, Tok::Conjg, Tok::ToPolar, Tok::ToRect, Tok::Abs,
                           Tok::Sq, Tok::Inv, Tok::Pow, Tok::Div, Tok::Mul, Tok::Open, Tok::Close};
  for (int i = 0; i < lists; ++i) {
    std::vector<Tok> t;
    const int len = 1 + g.R(99);
    for (int k = 0; k < len; ++k)
      t.push_back(g.P(40) ? cx[g.R(sizeof(cx) / sizeof(cx[0]))] : static_cast<Tok>(g.R(static_cast<int>(Tok::Count))));
    Vars v;
    ImagVars im;
    v.a = Num::ofInt(g.R(1000) - 500);
    im.a = Num::ofInt(g.R(7) - 3);
    v.b = Num::ofDouble(1e99);
    im.b = Num::ofDouble(-1e99);
    im.ans = Num::ofDouble(1e-99);
    const AngleUnit u = static_cast<AngleUnit>(g.R(3));
    const CxResult r = evaluateComplex(t, v, im, u, half);
    if (r.error != CalcError::None) continue;
    const Cx& z = r.value;
    if (!std::isfinite(z.re.v) || !std::isfinite(z.im.v) || !(std::fabs(z.re.v) < 1e100) || !(std::fabs(z.im.v) < 1e100))
      g_rep.fail("evaluateComplex: a part out of range without an error: " + exprText(t));
    for (const Num* n : {&z.re, &z.im})
      if (n->exact && (n->den <= 0 || std::fabs(static_cast<double>(n->num) / n->den - n->v) >
                                          1e-6 * std::max(1.0, std::fabs(n->v))))
        g_rep.fail("evaluateComplex: exact part disagrees with v: " + exprText(t));
    for (CxForm f : {CxForm::Rect, CxForm::Polar}) {
      const std::string s = complexText(z, f, g.P(50), static_cast<NormMode>(g.R(2)), u);
      std::string a, b;
      complexParts(z, f, g.P(50), NormMode::Norm1, u, a, b);
      const std::string* texts[] = {&s, &a, &b};
      for (const std::string* x : texts)
        if (x->empty() || x->find("nan") != std::string::npos || x->find("inf") != std::string::npos ||
            !validUtf8(*x) || textLength(a) > Framebuffer::kCols || textLength(b) > Framebuffer::kCols)
          g_rep.fail("complex format bad '" + *x + "'");
    }
  }
}

// MATRIX / VECTOR: random token lists (the variables, det( Trn( ·, x⁻¹ x² x³, brackets, + − × ÷ and
// the rest) through evaluateMatVec() with variables of random sizes (some without one); a result
// is a size the mode holds with every element in range, an error has a position in the list.
void matvecSoup(Rng& g, int lists) {
  static const Tok mt[] = {Tok::MatA, Tok::MatB, Tok::MatC, Tok::MatAns, Tok::VctA, Tok::VctB, Tok::VctC,
                           Tok::VctAns, Tok::Det, Tok::Trn, Tok::DotP, Tok::Abs, Tok::Inv, Tok::Sq, Tok::Cube,
                           Tok::Mul, Tok::Add, Tok::Sub, Tok::Div, Tok::Open, Tok::Close, Tok::Neg, Tok::D2, Tok::D7};
  for (int i = 0; i < lists; ++i) {
    const bool vec = g.P(50);
    Mat vars[4];
    for (Mat& m : vars) {
      if (g.P(10)) continue;  // no size yet
      const int rows = vec ? 1 : 1 + g.R(3), cols = vec ? 2 + g.R(2) : 1 + g.R(3);
      m = Mat::zero(rows, cols);
      for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
          m.at(r, c) = g.P(70) ? Num::ofInt(g.R(21) - 10) : g.P(50) ? Num::ofDouble(g.R(2001) / 7.0 - 140) : Num::ofDouble(9e49);
    }
    std::vector<Tok> t;
    const int len = 1 + g.R(g.P(80) ? 12 : 99);
    for (int k = 0; k < len; ++k)
      t.push_back(g.P(75) ? mt[g.R(sizeof(mt) / sizeof(mt[0]))] : static_cast<Tok>(g.R(static_cast<int>(Tok::Count))));
    Vars v;
    v.a = Num::ofInt(g.R(1000) - 500);
    const MvResult r = evaluateMatVec(t, v, vars, vec, static_cast<AngleUnit>(g.R(3)), half);
    if (r.error != CalcError::None) {
      if (r.errorPos < 0 || r.errorPos > static_cast<int>(t.size())) g_rep.fail("evaluateMatVec: error position out of range");
      continue;
    }
    if (r.kind == MvKind::Scalar) {
      if (!std::isfinite(r.scalar.v) || !(std::fabs(r.scalar.v) < 1e100)) g_rep.fail("evaluateMatVec: scalar out of range: " + exprText(t));
      continue;
    }
    if ((r.kind == MvKind::Vector) != vec) g_rep.fail("evaluateMatVec: a result of the other mode's kind");
    MatVecApp app;
    app.reset(vec);
    if (!app.setVar(3, r.m)) g_rep.fail("evaluateMatVec: a result the mode can't hold: " + exprText(t));
    checkMatVec(app, "matvecSoup: ");
    for (int rr = 0; rr < r.m.rows; ++rr)
      for (int c = 0; c < r.m.cols; ++c)
        if (badText(mvCellText(r.m.at(rr, c), 7))) g_rep.fail("mvCellText bad");
  }
}

// BASE-N: random token lists (digits, A-F, and or xnor xor Not( Neg( d h b o, brackets, + − × ÷
// and the rest) through evaluateBaseN() in each base; a result fits the base and shows soundly.
void baseSoup(Rng& g, int lists) {
  static const Tok bn[] = {Tok::D0, Tok::D1, Tok::D7, Tok::D9, Tok::HexA, Tok::HexF, Tok::And, Tok::Or, Tok::Xnor,
                           Tok::Xor, Tok::Not, Tok::BNeg, Tok::PreD, Tok::PreH, Tok::PreB, Tok::PreO, Tok::Open,
                           Tok::Close, Tok::Add, Tok::Sub, Tok::Mul, Tok::Div, Tok::Ans, Tok::VarA};
  for (int i = 0; i < lists; ++i) {
    std::vector<Tok> t;
    const int len = 1 + g.R(99);
    for (int k = 0; k < len; ++k)
      t.push_back(g.P(70) ? bn[g.R(sizeof(bn) / sizeof(bn[0]))] : static_cast<Tok>(g.R(static_cast<int>(Tok::Count))));
    Vars v;
    v.a = Num::ofDouble(g.P(50) ? g.R(70000) - 35000 + 0.5 : 3e9);
    v.ans = Num::ofInt(g.P(50) ? -2147483647LL - 1 : 2147483647LL);
    const Base b = static_cast<Base>(g.R(4));
    const BaseResult r = evaluateBaseN(t, v, b);
    if (r.error == CalcError::None) {
      if (!baseFits(r.value, b)) g_rep.fail("evaluateBaseN: a result outside the base without an error: " + exprText(t));
      if (!baseTextOk(baseText(r.value, b), b)) g_rep.fail("baseText bad: " + baseText(r.value, b));
    } else if (r.errorPos < 0 || r.errorPos > static_cast<int>(t.size())) {
      g_rep.fail("evaluateBaseN: error position out of range");
    }
  }
}

}  // namespace

int run(const Options& o) {
  g_rep = Report();
  g_rep.verbose = o.verbose;
  uint64_t steps = 0, typedSends = 0;
  for (size_t s = 0; s < o.seeds.size(); ++s) {
    Rng g(o.seeds[s]);
    for (int i = 0; i < o.deviceSessions; ++i) {
      deviceSession(g, o.deviceSteps, typedSends);
      steps += static_cast<uint64_t>(o.deviceSteps);
    }
  }
  int apps = 0, state = 0, pngs = 0;
  {
    Rng g(o.seeds.empty() ? 1 : o.seeds[0] ^ 0x9E3779B97F4A7C15ull);
    persistMutants(g, o.persistIterations, apps, state);
    pngRoundTrips(g, o.pngImages, pngs);
    moduleKeys(g, o.moduleSteps);
    modeKeys(g, o.modeSteps);
    engineSoup(g, o.engineLists);
    complexSoup(g, o.engineLists);
    baseSoup(g, o.engineLists);
    matvecSoup(g, o.engineLists);
  }
  std::printf("fuzz: device %llu steps (%llu typed sends), persist %d mutants (%d + %d accepted), %d PNGs, "
              "modules %d keys, modes %d keys, engine %d lists (+ %d CMPLX, + %d BASE-N, + %d MATRIX / VECTOR): %d failed\n",
              static_cast<unsigned long long>(steps), static_cast<unsigned long long>(typedSends),
              o.persistIterations * 40, apps, state, pngs, o.moduleSteps, o.modeSteps, o.engineLists, o.engineLists,
              o.engineLists, o.engineLists,
              g_rep.count);
  if (!g_rep.fails.empty())
    for (const auto& f : g_rep.fails) std::printf("  %6d x %s\n", f.second, f.first.c_str());
  return g_rep.count;
}

}  // namespace fuzz

// The bounded run for every test build: ~200k device steps, 2k persistence mutants,
// ~70 PNGs, 50k module keys, 20k token lists.
int runFuzzTests() {
  fuzz::Options o;
  return fuzz::run(o);
}
