// The simulated v15 board (see board.h): firmware-v15-lcd/src/main.cpp's loop, service by
// service, around the real core and the real LCD drawing code. Comments name the main.cpp
// function each part stands in for.
#include "board.h"

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "Arduino.h"  // millis(): the simulator's clock (host_hw.cpp)
#include "battery.h"
#include "calc_engine.h"
#include "claude_api.h"  // peekAnswer(): the board's early-answer parser
#include "lcd.h"
#include "sim_host.h"
#include "ui.h"
#include "vf_lcd.h"

using namespace calc;

namespace {

// ---- timing (main.cpp's constants; the AI and Wi-Fi ones are shortened for testing)
constexpr uint32_t kOffGraceMs = 4000;          // off: awake this long after the last key, then "deep sleep"
constexpr uint32_t kAppsSaveDelayMs = 2000;     // history / notes to flash 2 s after the last change
constexpr uint32_t kScreenSaveDelayMs = 2000;   // brightness / sleep timer likewise
constexpr uint32_t kLinkWifiWaitMs = 20000;     // Link calculator: time to join for the code
constexpr uint32_t kSerialWifiHoldMs = 30000;
constexpr uint32_t kSetupMs = 15u * 60 * 1000;  // setup by phone: the network's time limit
// Fake parts:
constexpr uint32_t kWifiJoinMs = 1500;          // joining the hotspot
constexpr uint32_t kCamStartMs = 600;           // the OV5640 powering up for the viewfinder
constexpr uint32_t kFrameMs = 70;               // ~14 frames a second, like the board
constexpr uint32_t kAfMs = 700;                 // autofocus settling
constexpr uint32_t kHoldStillMs = 1200;         // the photo: "HOLD STILL" until the frames are steady
constexpr uint32_t kNoWifiGiveUpMs = 5000;      // the board waits 12 s (photo) / 20 s (typed)
constexpr uint32_t kPartialMs = 700;            // the answer shows early, the steps follow
constexpr uint32_t kReplyMs = 1500;             // the whole reply
constexpr uint32_t kFailMs = 1200;              // server error / busy
constexpr uint32_t kTimeoutMs = 4000;           // the board's client gives up after 120 s
constexpr const char* kLinkUrl = "https://proofline.app";

// ---- canned AI replies, in the format the proxy returns (core/solve_result.cpp reads it)
// For the sample photo (tools/sim_v15/sample_photo.png: "4) Solve for x: 3x + 7 = 22").
const char* const kSampleReply =
    R"J({"readable":true,"confidence":0.97,"unclear":[],"read_as":"Solve for x: 3x + 7 = 22",)J"
    R"J("expression":"(22-7)/3","choice":"","answer":"x = 5","check":"(22-7)/3",)J"
    R"J("steps":["Subtract 7 from both sides: 3x = 15.","Divide both sides by 3: x = 5.",)J"
    R"J("Check: 3(5) + 7 = 15 + 7 = 22."]})J";
// Other photos: a few demo problems in turn (samples/*.json).
const char* const kDemoReplies[] = {
    R"J({"readable":true,"read_as":"Ball dropped from h = 5.0 m. Find its speed just before it hits the ground.",)J"
    R"J("confidence":0.95,"unclear":[],"expression":"sqrt(2*9.81*5.0)","answer":"v ≈ 9.90 m/s",)J"
    R"J("steps":["Energy is conserved: mgh = ½mv², so v = √(2gh).","v = √(2 × 9.81 m/s² × 5.0 m)","v = √98.1 ≈ 9.90 m/s"]})J",
    R"J({"readable":true,"read_as":"Evaluate 3/4 + 2^5 × sin 30°.","confidence":0.97,"unclear":[],)J"
    R"J("expression":"3/4+2^5*sin(30)","answer":"16.75 (= 67/4)","check":"3/4+2^5*sin(30)",)J"
    R"J("steps":["2⁵ = 32 and sin 30° = 0.5","32 × 0.5 = 16","3/4 + 16 = 16.75"]})J",
    R"J({"readable":true,"confidence":0.93,"unclear":[],"expression":"sqrt(2*9.8*5)","choice":"C",)J"
    R"J("answer":"(C) 9.90 m/s",)J"
    R"J("read_as":"A ball drops 5.0 m from rest. Speed at impact? A) 4.9 B) 7.0 C) 9.9 D) 49 m/s",)J"
    R"J("steps":["v² = 2gh","v = √(2 × 9.8 × 5.0)","v = √98 ≈ 9.90 m/s","Matches option C"]})J",
    R"J({"readable":true,"read_as":"How many moles are in 36.0 g of H₂O?","confidence":0.97,"unclear":[],)J"
    R"J("answer":"n ≈ 2.00 mol","steps":["Molar mass of H₂O = 2(1.008) + 16.00 = 18.02 g/mol.",)J"
    R"J("n = m / M = 36.0 g ÷ 18.02 g/mol","n ≈ 2.00 mol"]})J",
    R"J({"readable":true,"read_as":"Which organelle makes most of a cell's ATP?","confidence":0.99,"unclear":[],)J"
    R"J("answer":"The mitochondrion","steps":["ATP is mostly made by cellular respiration.",)J"
    R"J("Its final stages (Krebs cycle and electron transport chain) happen in the mitochondria."]})J",
};
const char* const kUnclearPhoto =
    R"J({"readable":true,"read_as":"Solve 2x + 7 = 15 for x.","confidence":0.62,)J"
    R"J("unclear":["line 1: '7' could be '1'","right edge slightly cut off"],"answer":"x = 4",)J"
    R"J("steps":["Subtract 7 from both sides: 2x = 8.","Divide both sides by 2: x = 4.","If the 7 is really a 1: x = 7."]})J";
const char* const kUnreadablePhoto =
    R"J({"readable":false,"read_as":"","confidence":0.1,"unclear":["The photo shows a blank page."],"answer":"","steps":[]})J";

std::string jsonEsc(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += c;
    else if (c == '\n') o += "\\n";
    else if (static_cast<unsigned char>(c) < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

std::string jsonList(const std::vector<std::string>& v) {
  std::string o = "[";
  for (size_t i = 0; i < v.size(); ++i) o += (i ? ",\"" : "\"") + jsonEsc(v[i]) + "\"";
  return o + "]";
}

std::string replyJson(bool readable, double conf, const std::vector<std::string>& unclear, const std::string& readAs,
                      const std::string& expression, const std::string& answer, const std::string& check,
                      const std::vector<std::string>& steps) {
  char c[16];
  std::snprintf(c, sizeof c, "%.2f", conf);
  return std::string("{\"readable\":") + (readable ? "true" : "false") + ",\"confidence\":" + c +
         ",\"unclear\":" + jsonList(unclear) + ",\"read_as\":\"" + jsonEsc(readAs) + "\",\"expression\":\"" +
         jsonEsc(expression) + "\",\"choice\":\"\",\"answer\":\"" + jsonEsc(answer) + "\",\"check\":\"" +
         jsonEsc(check) + "\",\"steps\":" + jsonList(steps) + "}";
}

void replaceAll(std::string& s, const std::string& from, const std::string& to) {
  for (size_t p = 0; (p = s.find(from, p)) != std::string::npos; p += to.size()) s.replace(p, from.size(), to);
}

double zeroRandom() { return 0.5; }

// The calculator's own engine on typed text: false if it can't read or work it out.
bool evalText(const std::string& text, double& out, std::string* shown = nullptr) {
  std::vector<Tok> toks;
  if (!parseExpression(text, toks) || toks.empty()) return false;
  Vars vars;
  const EvalResult r = evaluate(toks, vars, AngleUnit::Deg, zeroRandom);
  if (r.error != CalcError::None) return false;
  out = r.value.v;
  if (shown) {
    std::string frac;
    *shown = formatResult(r.value, false, NormMode::Norm1);
    if (r.value.exact && r.value.den != 1 && fractionText(r.value, frac)) *shown += " (= " + frac + ")";
  }
  return true;
}

std::string number(double v) { return formatDecimal(v, NormMode::Norm1); }

std::string lower(std::string s) {
  for (char& c : s)
    if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
  return s;
}

std::string trim(const std::string& s) {
  const size_t b = s.find_first_not_of(" "), e = s.find_last_not_of(" ");
  return b == std::string::npos ? "" : s.substr(b, e - b + 1);
}

// A worked answer for a typed problem: arithmetic, or a linear equation in x, worked out by
// the calculator's engine; anything else gets a polite demo answer.
std::string typedReply(const std::string& typed, ReplyMode mode) {
  const std::string shown = trim(typed);
  if (mode == ReplyMode::Unreadable)
    return replyJson(false, 0.2, {"That doesn't look like a problem to solve."}, shown, "", "", "", {});
  const bool unsure = mode == ReplyMode::Unclear;
  const double conf = unsure ? 0.6 : 0.96;
  const std::vector<std::string> unclear =
      unsure ? std::vector<std::string>{"The typed text could mean two different things."} : std::vector<std::string>{};
  std::string t = lower(shown);
  replaceAll(t, "\xC3\x97", "*");      // ×
  replaceAll(t, "\xC3\xB7", "/");      // ÷
  replaceAll(t, "\xE2\x88\x92", "-");  // −
  for (const char* w : {"solve for x", "solve", "for x", "find x", "find", "what is", "work out", "calculate", "?"})
    replaceAll(t, w, "");
  t = trim(t);
  if (t.size() > 1 && t.back() == ':') t.pop_back();
  // 1. arithmetic
  double v;
  std::string answer;
  if (t.find('=') == std::string::npos && evalText(t, v, &answer)) {
    return replyJson(true, conf, unclear, shown, t, answer, t,
                     {"Work it out in order: brackets, powers, then \xC3\x97 \xC3\xB7, then + \xE2\x88\x92.",
                      shown + " = " + answer});
  }
  // 2. a linear equation in x: f(x) = left - right is a straight line, f(x) = 0 where it crosses
  const size_t eq = t.find('=');
  if (eq != std::string::npos && t.find('=', eq + 1) == std::string::npos && t.find('x') != std::string::npos) {
    const std::string left = trim(t.substr(0, eq)), right = trim(t.substr(eq + 1));
    auto f = [&](double x, double& out) {
      std::string l = left, r = right;
      const std::string val = "(" + number(x) + ")";
      replaceAll(l, "x", val);
      replaceAll(r, "x", val);
      return evalText("(" + l + ")-(" + r + ")", out);
    };
    double f0, f1, f2;
    if (f(0, f0) && f(1, f1) && f(2, f2) && std::fabs(f2 - 2 * f1 + f0) < 1e-9 && std::fabs(f1 - f0) > 1e-12) {
      const double a = f1 - f0, b = -f0, x = b / a;
      std::string xs = number(x);
      return replyJson(true, conf, unclear, shown, "", "x = " + xs, xs,
                       {"Put the x terms on one side and the numbers on the other: " + number(a) + "x = " + number(b) + ".",
                        "Divide both sides by " + number(a) + ": x = " + xs + ".",
                        "Check: put x = " + xs + " back in, and both sides are equal."});
    }
  }
  // 3. anything else
  return replyJson(true, conf, unclear, shown, "", "(demo answer)", "",
                   {"This is the simulator's offline demo tutor.",
                    "It works out arithmetic (like 2+2 or 3\xC3\xB7" "4+2^5) and equations in x (like 3X+7=22).",
                    "On the real calculator the AI reads any problem and explains each step."});
}

int millivoltsFor(int pct) {
  for (int mv = 3000; mv <= 4250; mv += 5)
    if (lipoPercent(mv) >= pct) return mv;
  return 4200;
}

const char* effortName(Effort e) {
  switch (e) {
    case Effort::Careful: return "Careful";
    case Effort::Max: return "Max";
    default: return "Normal";
  }
}

std::string hms(uint32_t ms) {
  char b[32];
  std::snprintf(b, sizeof b, "%u:%02u:%02u", unsigned(ms / 3600000), unsigned(ms / 60000 % 60), unsigned(ms / 1000 % 60));
  return b;
}

uint64_t epochMs() {
  return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count());
}

}  // namespace

Board::Board() { vf_.buildImage = false; }
Board::~Board() = default;

void Board::logf(const char* fmt, ...) {
  char msg[512];
  va_list ap;
  va_start(ap, fmt);
  std::vsnprintf(msg, sizeof msg, fmt, ap);
  va_end(ap);
  log_.push_back(hms(millis()) + "  " + msg);
  while (log_.size() > 300) log_.pop_front();
  ++logRev_;
}

// ---------------------------------------------------------------- files (flash / RTC memory)

void Board::writeFile(const char* name, const std::string& data) {
  if (dataDir.empty()) return;
  std::error_code ec;
  std::filesystem::create_directories(dataDir, ec);
  const auto tmp = dataDir / (std::string(name) + ".tmp");
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f) return;
    f.write(data.data(), std::streamsize(data.size()));
    if (!f) return;
  }
  std::filesystem::rename(tmp, dataDir / name, ec);  // a new copy, then the swap (as appsWrite())
}

bool Board::readFile(const char* name, std::string& data) const {
  if (dataDir.empty()) return false;
  std::ifstream f(dataDir / name, std::ios::binary);
  if (!f) return false;
  std::stringstream ss;
  ss << f.rdbuf();
  data = ss.str();
  return true;
}

void Board::saveNow() {
  if (!dev_) return;
  writeFile("apps.bin", dev_->saveApps());
  appsDirty_ = false;
  const std::string st = dev_->saveState();
  writeFile("state.bin", std::to_string(epochMs()) + "\n" + st);
  stateSaved_ = st;
  writeFile("settings.txt", std::to_string(lcd::brightness()) + " " + std::to_string(dev_->sleepMinutes()) + " " +
                                (linked ? "1" : "0") + " " + linkCode + "\n");
  screenDirty_ = false;
}

void Board::loadFiles() {
  std::string s;
  // Settings: brightness, sleep timer, link state (NVS "lcd" and "link" on the board).
  if (readFile("settings.txt", s)) {
    std::istringstream in(s);
    int bright = 60, sleep = 1, link = 0;
    std::string code;
    if (in >> bright >> sleep >> link) {
      lcd::setBrightness(bright, true);
      lcd::IdlePolicy p = lcd::idlePolicy();
      p.offSeconds = sleep * 60;
      p.dimSeconds = p.offSeconds / 2 < 20 ? p.offSeconds / 2 : 20;
      p.sleepSeconds = p.offSeconds + 60;
      lcd::setIdlePolicy(p, true);
      linked = link != 0;
      if (in >> code) linkCode = code;
    }
  }
  // State kept while "asleep" (RTC memory): memories, Ans, replay, SETUP, exam mode.
  if (readFile("state.bin", s)) {
    const size_t nl = s.find('\n');
    if (nl != std::string::npos) {
      const uint64_t then = std::strtoull(s.substr(0, nl).c_str(), nullptr, 10);
      const uint64_t now = epochMs();
      const uint64_t gap = now > then ? now - then : 0;
      const uint32_t asleep = gap > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(gap);
      if (dev_->restoreState(s.substr(nl + 1), asleep)) {
        logf("[boot] calculator state restored (asleep %s)", hms(asleep).c_str());
        if (dev_->examExpiredOnRestore()) logf("[exam] exam mode ran out (12 h) while the calculator was off");
      } else {
        logf("[boot] saved state unreadable: ignored");
      }
    }
  }
  // History, notes and the last app (LittleFS /apps.bin on the board).
  if (readFile("apps.bin", s)) {
    if (dev_->restoreApps(s)) logf("[apps] history / notes restored (%u bytes)", unsigned(s.size()));
    else logf("[apps] saved history / notes unreadable (%u bytes): ignored", unsigned(s.size()));
  }
}

// ---------------------------------------------------------------- setup()

void Board::boot() {
  stopViewfinder();
  req_.reset();
  holdUntil_ = 0;
  dev_ = std::make_unique<Device>();
  Device& d = *dev_;
  const uint32_t now = millis();
  lcd::begin();
  ui::begin();
  ui::invalidate();
  d.setNoKeyHelp("Set up: = on SETUP 6 (phone), or USB: proxy, token.");
  d.setRandomSource([] { return esp_random() / 4294967296.0; });
  d.setSplash(true);  // the Proofline splash at start and after ON, as on the board
  d.startBoot();      // nothing to load: the bar just fills
  d.finishBoot();
  d.setTypedSolveSupported(true);
  d.setBlinkingCursor(true);
  d.setHasApiKey(true);  // the simulator's fake tutor is always "set up"
  d.setNetInfo("Phone hotspot (simulated)", "token ...a1b2", "=: set up from a phone");
  d.tick(now);
  loadFiles();
  d.setLinkInfo("", kLinkUrl, linked);
  d.setVersion("v15lcd-sim");
  // screenSettingsFromLcd()
  d.setBrightness(lcd::brightness());
  const int off = lcd::idlePolicy().offSeconds;
  d.setSleepMinutes(off <= 0 ? 30 : (off + 59) / 60);
  brightSynced_ = d.brightness();
  sleepSynced_ = d.sleepMinutes();
  appsRevSeen_ = d.appsRevision();
  appsSelSeen_ = d.appsSelected();
  appsDirty_ = false;
  wasExam_ = d.examActive();
  wasOff_ = d.isOff();
  asleep_ = false;
  wifiOn_ = wifiConnected_ = false;
  setupActive_ = false;
  vfPaused_ = wasAiReady_ = false;
  stateSaved_ = d.saveState();
  stateCheckedAt_ = now;
  serviceBattery();
  lastKeyAt_ = now;
  lightWasOff_ = false;
  logf("[boot] Proofline v15 simulator: real core + real LCD drawing code; fake AI, camera, Wi-Fi, battery");
}

void Board::coldBoot() {
  saveNow();
  logf("[power] restart (cold boot)");
  boot();
}

void Board::factoryReset() {
  std::error_code ec;
  for (const char* f : {"apps.bin", "state.bin", "settings.txt"}) std::filesystem::remove(dataDir / f, ec);
  linked = false;
  linkCode = "K7Q2MX";
  lcd::setBrightness(60, true);
  lcd::setIdlePolicy(lcd::IdlePolicy(), true);
  logf("[power] factory reset: saved history, notes, memories and settings deleted");
  boot();
  saveNow();
}

// ---------------------------------------------------------------- keys (main.cpp press())

void Board::press(DKey k) {
  Device& d = *dev_;
  const uint32_t now = millis();
  lastKeyAt_ = now;
  asleep_ = false;
  if (setupActive_) {  // the keypad only leaves setup mode (AC or ON)
    if (k == DKey::AC || k == DKey::On) {
      setupActive_ = false;
      ui::invalidate();
      logf("[setup] ended with %s", dkeyName(k));
    }
    if (k != DKey::On) return;
  }
  // A key while the light is off only wakes the screen. ON always counts.
  if (lightWasOff_ && k != DKey::On) {
    lightWasOff_ = false;
    logf("[keys] %s (screen wake)", dkeyName(k));
    return;
  }
  lightWasOff_ = false;
  if (vf_.running()) vf_.touch(now);
  if (k == DKey::Right && vf_.running() && onAiReady()) {  // > in the viewfinder: focus again
    focusFrom_ = refocusAt_ = now;
    logf("[cam] refocus");
    return;
  }
  if (k == DKey::Right && vfPaused_ && onAiReady()) {  // > on the AI SOLVE screen: viewfinder again
    vfPaused_ = false;
    return;
  }
  // SHIFT + up / down: screen brightness.
  if ((k == DKey::Up || k == DKey::Down) && shiftShown_ &&
      (d.view() == View::Calc || d.view() == View::Stat || d.view() == View::Table || d.view() == View::Eqn ||
       d.view() == View::MatVec ||
       d.view() == View::CalcRun || d.view() == View::SolveRun)) {
    lcd::setBrightness(lcd::brightness() + (k == DKey::Up ? 10 : -10), true);
    d.setBrightness(lcd::brightness());
    brightSynced_ = d.brightness();
    screenDirty_ = true;
    screenChangedAt_ = now;
    logf("[lcd] brightness %d %%", lcd::brightness());
  }
  // = on SETUP > 6:Wi-Fi & key: set up from a phone.
  if (k == DKey::Eq && d.view() == View::NetInfo && !d.isOff() && d.radiosAllowed()) {
    setupActive_ = true;
    setupUntil_ = now + kSetupMs;
    wifiOn_ = wifiConnected_ = false;
    logf("[setup] started (= on SETUP 6). Simulated: no phone can join; AC or ON ends it");
    return;
  }
  if (!quietKeys_) logf("[keys] %s", dkeyName(k));
  // Type it: remember the text, for the fake tutor's "read_as" (the board sends a PNG of it).
  if (d.view() == View::Keyboard && !d.keyboardForNote()) lastTyped_ = d.keyboardText();
  d.tick(now);
  d.onKey(k);
  if (int id = d.takeRequest()) queueRequest(id);
}

bool Board::typeChar(char c) {
  Device& d = *dev_;
  // The Type it / notes keyboard: letters (x and y in lower case are the maths x, y cells),
  // space, = and , are cells; digits and + - * / ( ) ^ . are keys, as on the calculator.
  if (!d.isOff() && d.view() == View::Keyboard &&
      ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ' || c == '=' || c == ',')) {
    std::string label = c == ' '                 ? "SPACE"
                        : c == 'x' || c == 'y'   ? std::string(1, c)
                        : c >= 'a' && c <= 'z'   ? std::string(1, char(c - 'a' + 'A'))
                                                 : std::string(1, c);
    kbPressCell(label);
    return true;
  }
  DKey ks[2];
  int n = 0;
  if (!keysForChar(c, ks, n)) return false;
  for (int i = 0; i < n; ++i) press(ks[i]);
  return true;
}

// As the screenshot harness's kbPress(): arrows to the cell, then =.
void Board::kbPressCell(const std::string& label) {
  Device& d = *dev_;
  int target = -1;
  for (int i = 0; i < Device::keyboardCellCount(); ++i)
    if (label == Device::keyboardCellAt(i).label) target = i;
  if (target < 0) return;
  const int row = Device::keyboardCellAt(target).row;
  quietKeys_ = true;  // one log line, not every arrow
  for (int g = 0; g < 4 && Device::keyboardCellAt(d.keyboardCell()).row != row; ++g) press(DKey::Down);
  for (int g = 0; g < 40 && d.keyboardCell() != target; ++g) press(DKey::Right);
  if (d.keyboardCell() == target) press(DKey::Eq);
  quietKeys_ = false;
  logf("[keys] \"%s\" on the keyboard (arrows to the cell, then =)", label.c_str());
}

void Board::cursorToSend() {
  Device& d = *dev_;
  if (d.isOff() || d.view() != View::Keyboard) return;
  quietKeys_ = true;
  for (int g = 0; g < 4 && Device::keyboardCellAt(d.keyboardCell()).row != 2; ++g) press(DKey::Down);
  for (int g = 0; g < 12 && Device::keyboardCellAt(d.keyboardCell()).kind != KeyCell::Send; ++g) press(DKey::Right);
  quietKeys_ = false;
  logf("[keys] cursor to %s", d.keyboardForNote() ? "SAVE" : "SEND");
}

void Board::startExam() {
  Device& d = *dev_;
  if (d.examActive()) {
    logf("[exam] already on");
    return;
  }
  // One loop pass after each key, as on the board (keys are far apart there): the screen
  // code sees the calculator off before it comes back on in exam mode.
  auto key = [&](DKey k) {
    lightWasOff_ = false;
    press(k);
    step();
  };
  if (!d.isOff()) {
    key(DKey::Shift);
    key(DKey::AC);
  }
  key(DKey::Shift);
  key(DKey::D7);
  key(DKey::On);
}

void Board::teacherUnlock() {
  if (dev_->examActive()) {
    dev_->usbUnlock();
    logf("[exam] teacher's USB unlock (\"exam off\")");
  } else {
    logf("[exam] exam mode is not on");
  }
}

void Board::powerButton() {
  lightWasOff_ = false;
  if (dev_->isOff()) {
    press(DKey::On);
  } else {
    press(DKey::Shift);
    step();
    lightWasOff_ = false;
    press(DKey::AC);
  }
}

void Board::advance(uint32_t ms) {
  simhost::advance(ms);
  logf("[time] +%s (simulated)", hms(ms).c_str());
}

void Board::setPhoto(const std::vector<uint8_t>& rgb, bool isDefault) {
  if (rgb.size() != size_t(vflcd::kFrameW) * vflcd::kFrameH * 3) return;
  photo_ = rgb;
  photoDefault_ = isDefault;
}

void Board::batteryChanged() {
  serviceBattery();
  ui::invalidate();
}

// The proxy's link state (main.cpp linkState()), as the panel switch says.
void Board::linkChanged() {
  Device& d = *dev_;
  const bool was = d.linked();
  if (was && !linked) {
    d.clearHistory();
    logf("[link] this calculator was unlinked: saved answers cleared");
  }
  if (linked) d.setLinkInfo("", kLinkUrl, true);
  else d.setLinkInfo(d.linkCode().empty() ? "" : linkCode, kLinkUrl, false);
  if (was != linked) logf("[link] %s", linked ? "linked to an account" : "not linked");
  saveNow();
}

// ---------------------------------------------------------------- the fake AI (solveTask)

void Board::queueRequest(int id) {
  Device& d = *dev_;
  std::string png;
  const bool typed = d.takeTypedImage(png);
  if (!aiBatteryOk_) {
    d.onFailure(id, Failure::LowBattery);
    logf("[solve] [%d] refused: battery too low", id);
    return;
  }
  if (!typed) stopViewfinder();
  req_ = std::make_unique<Request>();
  req_->id = id;
  req_->typed = typed;
  req_->text = typed ? lastTyped_ : "";
  req_->t0 = req_->stageAt = millis();
  req_->stage = typed ? 1 : 0;
  holdUntil_ = typed ? 0 : req_->t0 + kHoldStillMs;
  if (typed) logf("[solve] [%d] typed problem \"%s\" (PNG %u bytes)", id, req_->text.c_str(), unsigned(png.size()));
  else logf("[solve] [%d] taking the photo (fake camera)", id);
}

void Board::chooseReply(Request& r) {
  std::string json;
  if (r.typed) {
    json = typedReply(r.text, reply);
  } else if (reply == ReplyMode::Unclear) {
    json = kUnclearPhoto;
  } else if (reply == ReplyMode::Unreadable) {
    json = kUnreadablePhoto;
  } else if (photoDefault_) {
    json = kSampleReply;
  } else {
    const int n = int(sizeof kDemoReplies / sizeof kDemoReplies[0]);
    json = kDemoReplies[demoNext_++ % n];
  }
  r.json = json;
  r.confidence = 0;
  r.answer.clear();
  // The early answer, found by the same parser the board's stream reader uses.
  double conf = 0;
  std::string ans;
  if (peekAnswer(json, conf, ans)) {
    r.confidence = conf;
    r.answer = ans;
  }
}

void Board::serviceRequests(uint32_t now) {
  if (!req_) return;
  Device& d = *dev_;
  Request& r = *req_;
  if (d.activeRequest() != r.id) {  // AC / MODE / OFF: the solve task stops
    logf("[solve] [%d] cancelled", r.id);
    req_.reset();
    holdUntil_ = 0;
    return;
  }
  switch (r.stage) {
    case 0:  // the photo
      if (int32_t(now - (r.t0 + kHoldStillMs)) < 0) return;
      holdUntil_ = 0;
      d.onCaptured(r.id);
      logf("[solve] [%d] photo taken", r.id);
      r.stage = 1;
      r.stageAt = now;
      return;
    case 1:  // Wi-Fi
      if (wifiConnected_) {
        d.onCaptured(r.id);
        r.stage = 2;
        r.stageAt = now;
        chooseReply(r);
        logf("[solve] [%d] sent to the AI (simulated reply: %s)", r.id,
             reply == ReplyMode::Normal ? "normal" : reply == ReplyMode::Unclear ? "unclear"
             : reply == ReplyMode::Unreadable ? "unreadable" : reply == ReplyMode::Timeout ? "timeout"
             : reply == ReplyMode::ServerError ? "server error" : "busy");
      } else if (now - r.stageAt > kNoWifiGiveUpMs) {
        d.onFailure(r.id, Failure::NoConnection, "");
        logf("[solve] [%d] failed: no connection (is the hotspot reachable?)", r.id);
        lastKeyAt_ = now;
        req_.reset();
      }
      return;
    default: {  // the AI
      const uint32_t t = now - r.stageAt;
      Failure f;
      uint32_t at = 0;
      switch (reply) {
        case ReplyMode::Timeout: f = Failure::Timeout; at = kTimeoutMs; break;
        case ReplyMode::ServerError: f = Failure::ApiError; at = kFailMs; break;
        case ReplyMode::Busy: f = Failure::ApiBusy; at = kFailMs; break;
        default: f = Failure::ApiError; break;
      }
      if (at) {
        if (t < at) return;
        d.onFailure(r.id, f, "");
        logf("[solve] [%d] failed: %s (simulated)", r.id,
             f == Failure::Timeout ? "timeout" : f == Failure::ApiBusy ? "AI busy" : "server error");
        lastKeyAt_ = now;
        req_.reset();
        return;
      }
      if (!r.partialSent && t >= kPartialMs) {
        r.partialSent = true;
        if (!r.answer.empty()) d.onPartialAnswer(r.id, r.confidence, r.answer);
      }
      if (t < kReplyMs) return;
      d.onReply(r.id, r.json);
      logf("[solve] [%d] answer: %s", r.id, r.answer.empty() ? "(none)" : r.answer.c_str());
      lastKeyAt_ = now;
      req_.reset();
      return;
    }
  }
}

// ---------------------------------------------------------------- viewfinder (main.cpp serviceViewfinder())

bool Board::onAiReady() const {
  const Device& d = *dev_;
  return !d.isOff() && d.mode() == Mode::Ai && d.view() == View::Ai && d.ai().screen() == Screen::Ready;
}

void Board::stopViewfinder() {
  if (!vf_.running()) return;
  vf_.stop();
  ui::invalidate();  // the calculator screen is redrawn over the picture
}

bool Board::serviceViewfinder(uint32_t now) {
  const Device& d = *dev_;
  const bool ready = onAiReady();
  if (ready && !wasAiReady_) vfPaused_ = false;  // every visit starts with the viewfinder
  wasAiReady_ = ready;
  const bool camBusy = req_ && req_->stage == 0;
  const bool want = ready && d.radiosAllowed() && aiBatteryOk_ && !vfPaused_ && !setupActive_ && !camBusy &&
                    !photo_.empty();
  if (!want) {
    stopViewfinder();
    return false;
  }
  if (!vf_.running()) {
    vf_.start(now);
    vflcd::showStarting("starting...");
    camReadyAt_ = nextFrameAt_ = focusFrom_ = now + kCamStartMs;
    fpsAt_ = now;
    fpsFrames_ = 0;
    fps_ = 0;
    logf("[cam] viewfinder on: = takes the photo, > refocuses, AC leaves, 30 s idle turns it off");
    return true;
  }
  if (vf_.timedOut(now)) {
    stopViewfinder();
    vfPaused_ = true;
    logf("[cam] viewfinder off (30 s without a key) to save the battery. > shows it again");
    return false;
  }
  if (int32_t(now - nextFrameAt_) < 0) return true;
  nextFrameAt_ = now + kFrameMs;
  // One camera frame: the picture (shifted a little for a shaky hand), RGB565 in the
  // panel's byte order (high byte first), as the OV5640 DMA leaves it.
  static std::vector<uint8_t> buf;
  buf.resize(size_t(vflcd::kFrameW) * vflcd::kFrameH * 2);
  int dx = 0, dy = 0;
  if (shaky) {
    const int phase = int(now / 90) % 6;
    dx = (phase * 5) % 13 - 6;
    dy = (phase * 3) % 7 - 3;
  }
  for (int y = 0; y < vflcd::kFrameH; ++y)
    for (int x = 0; x < vflcd::kFrameW; ++x) {
      int sx = x - dx, sy = y - dy;
      sx = sx < 0 ? 0 : sx >= vflcd::kFrameW ? vflcd::kFrameW - 1 : sx;
      sy = sy < 0 ? 0 : sy >= vflcd::kFrameH ? vflcd::kFrameH - 1 : sy;
      const uint8_t* p = &photo_[(size_t(sy) * vflcd::kFrameW + sx) * 3];
      const uint16_t v = lcd::rgb(p[0], p[1], p[2]);
      buf[(size_t(y) * vflcd::kFrameW + x) * 2] = uint8_t(v >> 8);
      buf[(size_t(y) * vflcd::kFrameW + x) * 2 + 1] = uint8_t(v & 0xFF);
    }
  uint8_t grey[80 * 60];
  vflcd::greyThumb(buf.data(), grey);
  const bool focused = int32_t(now - (focusFrom_ + kAfMs)) >= 0;
  vf_.feed(grey, 80, 60, focused, now);
  vflcd::Overlay o;
  o.info = vf_.info();
  o.focus = focused && now - refocusAt_ > 400 ? vflcd::FocusState::Focused : vflcd::FocusState::Focusing;
  o.secondsLeft = vf_.secondsLeft(now);
  o.mode = std::string(effortName(d.ai().effort())) + (d.ai().tutor() ? " +Tutor" : "");
  o.fps = fps_;
  o.battery = batteryPct;
  o.charging = charging();
  o.wifiConnected = wifiConnected_;
  vflcd::showFrame(buf.data(), o);
  ++fpsFrames_;
  if (now - fpsAt_ >= 1000) {
    fps_ = fpsFrames_ * 1000.0 / (now - fpsAt_);
    fpsAt_ = now;
    fpsFrames_ = 0;
  }
  return true;
}

// ---------------------------------------------------------------- the other services

ui::Status Board::uiStatus() const {
  ui::Status st;
  st.battery = batteryPct;
  st.charging = charging();
  st.wifiOn = wifiOn_;
  st.wifiConnected = wifiConnected_;
  st.lowBattery = !aiBatteryOk_;
  return st;
}

// Wi-Fi on only while the AI can use it (main.cpp serviceWifi()). The hotspot is assumed
// set up; "joining" takes kWifiJoinMs and only succeeds while the hotspot is reachable.
void Board::serviceWifi(uint32_t now) {
  const Device& d = *dev_;
  // Type it's keyboard too: it joins while the problem is typed, so the first SEND goes.
  const bool typing = d.view() == View::Keyboard && !d.keyboardForNote();
  const bool aiScreen = !d.isOff() && (d.mode() == Mode::Ai || typing);
  const bool want = d.radiosAllowed() && aiBatteryOk_ && !setupActive_ &&
                    (aiScreen || d.activeRequest() != 0 || int32_t(wifiNeededUntil_ - now) > 0);
  if (want && !wifiOn_) {
    wifiOn_ = true;
    wifiJoinAt_ = now + kWifiJoinMs;
    logf("[wifi] on: joining the hotspot");
  } else if (!want && wifiOn_) {
    wifiOn_ = false;
    logf("[wifi] off");
  }
  const bool c = wifiOn_ && hotspot && int32_t(now - wifiJoinAt_) >= 0;
  if (c != wifiConnected_) {
    wifiConnected_ = c;
    if (wifiOn_) logf("[wifi] %s", c ? "connected" : "lost the hotspot");
  }
}

// Battery: the AI lock-out with hysteresis (main.cpp serviceBattery()).
void Board::serviceBattery() {
  const int mv = millivoltsFor(batteryPct);
  const bool was = aiBatteryOk_;
  if (usb) aiBatteryOk_ = true;
  else if (mv < kAiMinMillivolts) aiBatteryOk_ = false;
  else if (mv >= kAiResumeMillivolts) aiBatteryOk_ = true;
  if (was != aiBatteryOk_)
    logf("[power] battery %d mV: AI, camera and Wi-Fi %s", mv, aiBatteryOk_ ? "allowed again" : "off until charged");
  dev_->setLowBattery(!aiBatteryOk_);
  dev_->setBattery(batteryPct, charging());
}

void Board::serviceExam() {
  const bool exam = dev_->examActive();
  if (exam == wasExam_) return;
  wasExam_ = exam;
  logf("[exam] %s", exam ? "ON: Wi-Fi and camera off" : "OFF");
  if (exam) setupActive_ = false;
}

void Board::serviceApps(uint32_t now) {
  const Device& d = *dev_;
  if (d.appsRevision() != appsRevSeen_ || d.appsSelected() != appsSelSeen_) {
    appsRevSeen_ = d.appsRevision();
    appsSelSeen_ = d.appsSelected();
    appsChangedAt_ = now;
    appsDirty_ = true;
  }
  if (appsDirty_ && now - appsChangedAt_ >= kAppsSaveDelayMs) {
    appsDirty_ = false;
    const std::string blob = d.saveApps();
    writeFile("apps.bin", blob);
    logf("[apps] history / notes saved (%u bytes)", unsigned(blob.size()));
  }
}

void Board::serviceScreenSettings(uint32_t now) {
  const Device& d = *dev_;
  if (d.brightness() != brightSynced_) {
    brightSynced_ = d.brightness();
    lcd::setBrightness(brightSynced_, false);
    screenDirty_ = true;
    screenChangedAt_ = now;
  }
  if (d.sleepMinutes() != sleepSynced_) {
    sleepSynced_ = d.sleepMinutes();
    lcd::IdlePolicy p = lcd::idlePolicy();
    p.offSeconds = sleepSynced_ * 60;
    p.dimSeconds = p.offSeconds / 2 < 20 ? p.offSeconds / 2 : 20;
    p.sleepSeconds = p.offSeconds + 60;
    lcd::setIdlePolicy(p, false);
    screenDirty_ = true;
    screenChangedAt_ = now;
  }
  if (screenDirty_ && now - screenChangedAt_ >= kScreenSaveDelayMs) {
    screenDirty_ = false;
    saveNow();
    logf("[lcd] brightness %d %%, light off after %d min (saved)", lcd::brightness(), sleepSynced_);
  }
}

// Settings > Link calculator: Wi-Fi on, then the proxy's answer (main.cpp serviceLink()).
void Board::serviceLink(uint32_t now) {
  Device& d = *dev_;
  const bool onLink = !d.isOff() && d.view() == View::Link;
  if (onLink && !wasOnLink_ && d.radiosAllowed() && aiBatteryOk_ && d.activeRequest() == 0 && !setupActive_) {
    linkWaiting_ = true;
    linkAt_ = now;
    wifiNeededUntil_ = now + kSerialWifiHoldMs;
    logf("[link] fetching the link state (Wi-Fi on)");
  }
  wasOnLink_ = onLink;
  if (!linkWaiting_) return;
  if (!onLink) {
    linkWaiting_ = false;
    return;
  }
  if (wifiConnected_) {
    linkWaiting_ = false;
    if (linked) {
      d.setLinkInfo("", kLinkUrl, true);
      logf("[link] Account: linked");
    } else {
      d.setLinkInfo(linkCode, kLinkUrl, false);
      logf("[link] Pairing code: %s (simulated)", linkCode.c_str());
    }
    lastKeyAt_ = now;
  } else if (now - linkAt_ > kLinkWifiWaitMs) {
    linkWaiting_ = false;
    logf("[link] couldn't join the hotspot for a link code");
  }
}

// Off: Wi-Fi and camera off, then "deep sleep" a few seconds after the last key (on
// battery). The board keeps the state in RTC memory: here, state.bin.
void Board::servicePower(uint32_t now) {
  const bool off = dev_->isOff() && dev_->activeRequest() == 0 && !setupActive_;
  if (!off) {
    wasOff_ = false;
    return;
  }
  if (!wasOff_) {
    wasOff_ = true;
    logf("[power] off");
  }
  if (usb || asleep_ || now - lastKeyAt_ < kOffGraceMs) return;
  asleep_ = true;
  saveNow();
  logf("[power] deep sleep (state saved; ON or any key wakes it)");
}

// The state the board keeps in RTC memory, written whenever it changes (every 2 s at most),
// so closing the window keeps memories, Ans, SETUP and exam mode.
void Board::serviceState(uint32_t now) {
  if (now - stateCheckedAt_ < 2000) return;
  stateCheckedAt_ = now;
  const std::string st = dev_->saveState();
  if (st == stateSaved_) return;
  stateSaved_ = st;
  writeFile("state.bin", std::to_string(epochMs()) + "\n" + st);
}

void Board::drawSetupScreen() {
  const uint32_t left = int32_t(setupUntil_ - millis()) > 0 ? (setupUntil_ - millis()) / 1000 : 0;
  char timer[24];
  std::snprintf(timer, sizeof timer, "%lu min left", (unsigned long)((left + 59) / 60));
  ui::showLines({"SETUP BY PHONE", "1.Join Wi-Fi AI-Calc-5C21", "  password k7wq2mxe", "2.Open the page it shows",
                 "  or go to 192.168.4.1", "AC:back", timer},
                uiStatus());
}

std::string Board::statusText() const {
  const Device& d = *dev_;
  std::string s = "Clock " + hms(millis() - 1000);
  s += "   Light " + (simhost::lightLevel() == 0 ? std::string("off") : std::to_string(simhost::lightLevel()) + "%");
  s += "\r\nWi-Fi: " + std::string(!wifiOn_ ? "off" : wifiConnected_ ? "connected" : "joining...");
  s += "   AI: " + std::string(aiBatteryOk_ ? "allowed" : "battery too low");
  s += "\r\nExam: " + (d.examActive() ? "ON " + hms(d.examElapsedMs()) + " of 12:00:00" : std::string("off"));
  s += d.isOff() ? (asleep_ ? "   Power: off (deep sleep)" : "   Power: off") : "   Power: on";
  return s;
}

// ---------------------------------------------------------------- loop()

void Board::step() {
  Device& d = *dev_;
  const uint32_t now = millis();
  if (setupActive_ && int32_t(now - setupUntil_) >= 0) {
    setupActive_ = false;
    ui::invalidate();
    logf("[setup] time limit: setup network closed");
  }
  serviceRequests(now);
  serviceExam();
  serviceApps(now);
  serviceScreenSettings(now);
  serviceLink(now);
  serviceBattery();
  serviceWifi(now);
  d.setOnline(wifiConnected_ || wifiOn_);  // "online" once Wi-Fi is on for AI SOLVE (as main.cpp)
  d.setHoldUntil(holdUntil_);
  d.tick(now);
  bool viewfinder = false;
  if (setupActive_) {
    drawSetupScreen();
  } else if (!(viewfinder = serviceViewfinder(now))) {
    d.render(fb_);
    shiftShown_ = (fb_.get(0, 1) || fb_.get(1, 1) || fb_.get(2, 1)) && !fb_.get(4, 3);
    ui::showDevice(d, fb_, uiStatus(), now);
  }
  const bool keepOn = viewfinder || d.activeRequest() != 0 || setupActive_;
  if (d.isOff() && !usb && !keepOn) {
    lcd::setLevel(0);
    lightWasOff_ = false;
  } else {
    lightWasOff_ = lcd::serviceIdle(now - lastKeyAt_, keepOn);
  }
  servicePower(now);
  serviceState(now);
}
