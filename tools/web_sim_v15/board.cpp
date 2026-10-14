// The simulated v15 board for the website (see board.h): firmware-v15-lcd/src/main.cpp's
// loop, service by service, around the real core and the real LCD drawing code. Adapted
// from tools/sim_v15/board.cpp; comments name the main.cpp function each part stands in for.
#include "board.h"

#include <cmath>
#include <cstdio>

#include "Arduino.h"  // millis(): the page's clock (host_hw.cpp)
#include "battery.h"
#include "calc_engine.h"
#include "claude_api.h"  // peekAnswer(): the board's early-answer parser
#include "lcd.h"
#include "sim_host.h"
#include "ui.h"
#include "vf_lcd.h"

using namespace calc;

namespace {

constexpr uint32_t kAppsSaveDelayMs = 2000;  // history / notes kept 2 s after the last change
constexpr uint32_t kLinkWifiWaitMs = 20000;
constexpr uint32_t kSerialWifiHoldMs = 30000;
// Pretend parts:
constexpr uint32_t kWifiJoinMs = 1200;   // joining the hotspot
constexpr uint32_t kCamStartMs = 600;    // the camera powering up for the viewfinder
constexpr uint32_t kFrameMs = 70;        // ~14 frames a second, like the board
constexpr uint32_t kAfMs = 700;          // autofocus settling
constexpr uint32_t kHoldStillMs = 1200;  // the photo: "HOLD STILL" until the frames are steady
constexpr uint32_t kPartialMs = 700;     // the answer shows early, the steps follow
constexpr uint32_t kReplyMs = 1600;      // the whole reply
constexpr const char* kLinkUrl = "https://proofline.app";

// ---- demo replies, in the format the server returns (core/solve_result.cpp reads it).
// AI Solve by photo shows them in turn; the first matches the bundled sample photo
// ("Solve for x: 3x + 7 = 22").
const char* const kDemoReplies[] = {
    R"J({"readable":true,"confidence":0.97,"unclear":[],"read_as":"Solve for x: 3x + 7 = 22",)J"
    R"J("expression":"(22-7)/3","choice":"","answer":"x = 5","check":"(22-7)/3",)J"
    R"J("steps":["Subtract 7 from both sides: 3x = 15.","Divide both sides by 3: x = 5.",)J"
    R"J("Check: 3(5) + 7 = 15 + 7 = 22."]})J",
    R"J({"readable":true,"read_as":"A ball is dropped from h = 5.0 m. Find its speed just before it hits the ground.",)J"
    R"J("confidence":0.95,"unclear":[],"expression":"sqrt(2*9.81*5.0)","answer":"v ≈ 9.90 m/s",)J"
    R"J("steps":["Energy is conserved: mgh = ½mv², so v = √(2gh).","v = √(2 × 9.81 m/s² × 5.0 m)",)J"
    R"J("v = √98.1 ≈ 9.90 m/s"]})J",
    R"J({"readable":true,"read_as":"How many moles are in 36.0 g of H₂O?","confidence":0.97,"unclear":[],)J"
    R"J("answer":"n ≈ 2.00 mol","steps":["Molar mass of H₂O = 2(1.008) + 16.00 = 18.02 g/mol.",)J"
    R"J("n = m / M = 36.0 g ÷ 18.02 g/mol","n ≈ 2.00 mol"]})J",
};

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

std::string replyJson(const std::string& readAs, const std::string& expression, const std::string& answer,
                      const std::string& check, const std::vector<std::string>& steps) {
  return std::string("{\"readable\":true,\"confidence\":0.96,\"unclear\":[],\"read_as\":\"") + jsonEsc(readAs) +
         "\",\"expression\":\"" + jsonEsc(expression) + "\",\"choice\":\"\",\"answer\":\"" + jsonEsc(answer) +
         "\",\"check\":\"" + jsonEsc(check) + "\",\"steps\":" + jsonList(steps) + "}";
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
// the calculator's engine; anything else gets a friendly demo answer.
std::string typedReply(const std::string& typed) {
  const std::string shown = trim(typed);
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
    return replyJson(shown, t, answer, t,
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
      return replyJson(shown, "", "x = " + xs, xs,
                       {"Put the x terms on one side and the numbers on the other: " + number(a) + "x = " + number(b) + ".",
                        "Divide both sides by " + number(a) + ": x = " + xs + ".",
                        "Check: put x = " + xs + " back in, and both sides are equal."});
    }
  }
  // 3. anything else
  return replyJson(shown, "", "(demo answer)", "",
                   {"This is the website's offline demo.",
                    "Here it works out sums (like 2+2 or 3\xC3\xB7" "4+2^5) and equations in x (like 3X+7=22).",
                    "The real calculator's AI tutor reads any problem and explains each step."});
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

// The screen's idle policy for the website: no dimming, the light goes off after the
// Settings > Sleep timer (as the board), the panel "sleeps" a minute later.
void applySleepMinutes(int minutes) {
  lcd::IdlePolicy p = lcd::idlePolicy();
  p.offSeconds = minutes * 60;
  p.dimSeconds = 0;
  p.sleepSeconds = p.offSeconds + 60;
  lcd::setIdlePolicy(p, false);
}

}  // namespace

Board::Board() { vf_.buildImage = false; }
Board::~Board() = default;

// ---------------------------------------------------------------- setup()

void Board::boot(const std::string& apps) {
  stopViewfinder();
  req_.reset();
  holdUntil_ = 0;
  dev_ = std::make_unique<Device>();
  Device& d = *dev_;
  const uint32_t now = millis();
  lcd::begin();
  lcd::setBrightness(70, false);
  ui::begin();
  ui::invalidate();
  d.setNoKeyHelp("This demo needs no set-up.");
  d.setRandomSource([] { return esp_random() / 4294967296.0; });
  d.setSplash(true);  // the Proofline splash at start and after ON, as on the board
  d.startBoot();      // nothing to load: the bar just fills
  d.finishBoot();
  d.setTypedSolveSupported(true);
  d.setBlinkingCursor(true);
  d.setHasApiKey(true);  // the demo tutor is always "set up"
  d.setNetInfo("demo", "demo", "Demo: nothing to set up");
  d.tick(now);
  if (!apps.empty() && !d.restoreApps(apps)) {
    // damaged or from an older build: start empty
  }
  d.setLinkInfo("", kLinkUrl, false);
  d.setVersion("web demo");
  d.setBrightness(lcd::brightness());
  d.setSleepMinutes(10);
  applySleepMinutes(d.sleepMinutes());
  brightSynced_ = d.brightness();
  sleepSynced_ = d.sleepMinutes();
  appsRevSeen_ = d.appsRevision();
  appsSelSeen_ = d.appsSelected();
  appsDirty_ = false;
  wifiOn_ = wifiConnected_ = false;
  vfPaused_ = wasAiReady_ = false;
  serviceBattery();
  lastKeyAt_ = now;
  lightWasOff_ = false;
}

// ---------------------------------------------------------------- keys (main.cpp press())

void Board::press(DKey k) {
  Device& d = *dev_;
  const uint32_t now = millis();
  lastKeyAt_ = now;
  // A key while the light is off only wakes the screen. ON always counts.
  if (lightWasOff_ && k != DKey::On) {
    lightWasOff_ = false;
    return;
  }
  lightWasOff_ = false;
  if (vf_.running()) vf_.touch(now);
  if (k == DKey::Right && vf_.running() && onAiReady()) {  // > in the viewfinder: focus again
    focusFrom_ = refocusAt_ = now;
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
    lcd::setBrightness(lcd::brightness() + (k == DKey::Up ? 10 : -10), false);
    d.setBrightness(lcd::brightness());
    brightSynced_ = d.brightness();
  }
  // = on SETUP > 6:Wi-Fi & key starts setup by phone on the board: not in this demo
  // (the screen says "Demo: nothing to set up").
  if (k == DKey::Eq && d.view() == View::NetInfo && !d.isOff()) return;
  // Type it: remember the text, for the demo tutor's "read_as" (the board sends a PNG of it).
  if (d.view() == View::Keyboard && !d.keyboardForNote()) lastTyped_ = d.keyboardText();
  d.tick(now);
  d.onKey(k);
  if (int id = d.takeRequest()) queueRequest(id);
}

bool Board::typeChar(char c) {
  Device& d = *dev_;
  if (!d.isOff() && d.view() == View::Keyboard &&
      ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ' || c == '=' || c == ',')) {
    // Small x is the maths x (equations); every other letter, y included, is a capital
    // (the desktop simulator also maps small y to the maths y; "why" read badly here).
    std::string label = c == ' '                 ? "SPACE"
                        : c == 'x'               ? std::string(1, c)
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

// Arrows to the cell, then = (what a user does on the calculator).
void Board::kbPressCell(const std::string& label) {
  Device& d = *dev_;
  int target = -1;
  for (int i = 0; i < Device::keyboardCellCount(); ++i)
    if (label == Device::keyboardCellAt(i).label) target = i;
  if (target < 0) return;
  const int row = Device::keyboardCellAt(target).row;
  for (int g = 0; g < 4 && Device::keyboardCellAt(d.keyboardCell()).row != row; ++g) press(DKey::Down);
  for (int g = 0; g < 40 && d.keyboardCell() != target; ++g) press(DKey::Right);
  if (d.keyboardCell() == target) press(DKey::Eq);
}

void Board::cursorToSend() {
  Device& d = *dev_;
  if (d.isOff() || d.view() != View::Keyboard) return;
  for (int g = 0; g < 4 && Device::keyboardCellAt(d.keyboardCell()).row != 2; ++g) press(DKey::Down);
  for (int g = 0; g < 12 && Device::keyboardCellAt(d.keyboardCell()).kind != KeyCell::Send; ++g) press(DKey::Right);
}

void Board::setPhoto(const std::vector<uint8_t>& rgb) {
  if (rgb.size() != size_t(vflcd::kFrameW) * vflcd::kFrameH * 3) return;
  photo_ = rgb;
}

// ---------------------------------------------------------------- the demo AI (solveTask)

void Board::queueRequest(int id) {
  Device& d = *dev_;
  std::string png;
  const bool typed = d.takeTypedImage(png);
  if (!typed) stopViewfinder();
  req_ = std::make_unique<Request>();
  req_->id = id;
  req_->typed = typed;
  req_->text = typed ? lastTyped_ : "";
  req_->t0 = req_->stageAt = millis();
  req_->stage = typed ? 1 : 0;
  holdUntil_ = typed ? 0 : req_->t0 + kHoldStillMs;
}

void Board::chooseReply(Request& r) {
  const int n = int(sizeof kDemoReplies / sizeof kDemoReplies[0]);
  // The next demo in turn; it only counts as shown once the answer arrives (board step),
  // so a cancelled photo doesn't skip one.
  r.json = r.typed ? typedReply(r.text) : std::string(kDemoReplies[demoNext_ % n]);
  r.confidence = 0;
  r.answer.clear();
  // The early answer, found by the same parser the board's stream reader uses.
  double conf = 0;
  std::string ans;
  if (peekAnswer(r.json, conf, ans)) {
    r.confidence = conf;
    r.answer = ans;
  }
}

void Board::serviceRequests(uint32_t now) {
  if (!req_) return;
  Device& d = *dev_;
  Request& r = *req_;
  if (d.activeRequest() != r.id) {  // AC / MODE / OFF: the solve task stops
    req_.reset();
    holdUntil_ = 0;
    return;
  }
  switch (r.stage) {
    case 0:  // the photo
      if (int32_t(now - (r.t0 + kHoldStillMs)) < 0) return;
      holdUntil_ = 0;
      d.onCaptured(r.id);
      r.stage = 1;
      r.stageAt = now;
      return;
    case 1:  // Wi-Fi (always reachable here)
      if (wifiConnected_) {
        d.onCaptured(r.id);
        r.stage = 2;
        r.stageAt = now;
        chooseReply(r);
      }
      return;
    default: {  // the AI
      const uint32_t t = now - r.stageAt;
      if (!r.partialSent && t >= kPartialMs) {
        r.partialSent = true;
        if (!r.answer.empty()) d.onPartialAnswer(r.id, r.confidence, r.answer);
      }
      if (t < kReplyMs) return;
      d.onReply(r.id, r.json);
      if (!r.typed) ++demoNext_;
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
  const bool want = ready && d.radiosAllowed() && !vfPaused_ && !camBusy && !photo_.empty();
  if (!want) {
    stopViewfinder();
    return false;
  }
  if (!vf_.running()) {
    vf_.start(now);
    vflcd::showStarting("starting...");
    nextFrameAt_ = focusFrom_ = now + kCamStartMs;
    fpsAt_ = now;
    fpsFrames_ = 0;
    fps_ = 0;
    return true;
  }
  if (vf_.timedOut(now)) {
    stopViewfinder();
    vfPaused_ = true;
    return false;
  }
  if (int32_t(now - nextFrameAt_) < 0) return true;
  nextFrameAt_ = now + kFrameMs;
  // One camera frame: the picture, RGB565 in the panel's byte order (high byte first).
  static std::vector<uint8_t> buf;
  buf.resize(size_t(vflcd::kFrameW) * vflcd::kFrameH * 2);
  for (size_t i = 0, n = size_t(vflcd::kFrameW) * vflcd::kFrameH; i < n; ++i) {
    const uint8_t* p = &photo_[i * 3];
    const uint16_t v = lcd::rgb(p[0], p[1], p[2]);
    buf[i * 2] = uint8_t(v >> 8);
    buf[i * 2 + 1] = uint8_t(v & 0xFF);
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
  o.charging = false;
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
  st.charging = false;
  st.wifiOn = wifiOn_;
  st.wifiConnected = wifiConnected_;
  st.lowBattery = false;
  return st;
}

// Wi-Fi on only while the AI can use it (main.cpp serviceWifi()).
void Board::serviceWifi(uint32_t now) {
  const Device& d = *dev_;
  const bool typing = d.view() == View::Keyboard && !d.keyboardForNote();
  const bool aiScreen = !d.isOff() && (d.mode() == Mode::Ai || typing);
  const bool want = d.radiosAllowed() && (aiScreen || d.activeRequest() != 0 || int32_t(wifiNeededUntil_ - now) > 0);
  if (want && !wifiOn_) {
    wifiOn_ = true;
    wifiJoinAt_ = now + kWifiJoinMs;
  } else if (!want && wifiOn_) {
    wifiOn_ = false;
  }
  wifiConnected_ = wifiOn_ && int32_t(now - wifiJoinAt_) >= 0;
}

void Board::serviceBattery() {
  (void)millivoltsFor;
  dev_->setLowBattery(false);
  dev_->setBattery(batteryPct, false);
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
    if (onSaveApps) onSaveApps(d.saveApps());
  }
}

void Board::serviceScreenSettings(uint32_t) {
  const Device& d = *dev_;
  if (d.brightness() != brightSynced_) {
    brightSynced_ = d.brightness();
    lcd::setBrightness(brightSynced_, false);
  }
  if (d.sleepMinutes() != sleepSynced_) {
    sleepSynced_ = d.sleepMinutes();
    applySleepMinutes(sleepSynced_);
  }
}

// Settings > Link calculator: Wi-Fi on, then the server's answer (main.cpp serviceLink()).
// Here: always the demo code.
void Board::serviceLink(uint32_t now) {
  Device& d = *dev_;
  const bool onLink = !d.isOff() && d.view() == View::Link;
  if (onLink && !wasOnLink_ && d.radiosAllowed() && d.activeRequest() == 0) {
    linkWaiting_ = true;
    linkAt_ = now;
    wifiNeededUntil_ = now + kSerialWifiHoldMs;
  }
  wasOnLink_ = onLink;
  if (!linkWaiting_) return;
  if (!onLink) {
    linkWaiting_ = false;
    return;
  }
  if (wifiConnected_) {
    linkWaiting_ = false;
    d.setLinkInfo(linkCode, kLinkUrl, false);
    lastKeyAt_ = now;
  } else if (now - linkAt_ > kLinkWifiWaitMs) {
    linkWaiting_ = false;
  }
}

// ---------------------------------------------------------------- loop()

void Board::step() {
  Device& d = *dev_;
  const uint32_t now = millis();
  serviceRequests(now);
  serviceApps(now);
  serviceScreenSettings(now);
  serviceLink(now);
  serviceBattery();
  serviceWifi(now);
  d.setOnline(wifiConnected_ || wifiOn_);
  d.setHoldUntil(holdUntil_);
  d.tick(now);
  bool viewfinder = false;
  if (!(viewfinder = serviceViewfinder(now))) {
    d.render(fb_);
    shiftShown_ = (fb_.get(0, 1) || fb_.get(1, 1) || fb_.get(2, 1)) && !fb_.get(4, 3);
    ui::showDevice(d, fb_, uiStatus(), now);
  }
  const bool keepOn = viewfinder || d.activeRequest() != 0;
  if (d.isOff() && !keepOn) {
    lcd::setLevel(0);
    lightWasOff_ = false;
  } else {
    lightWasOff_ = lcd::serviceIdle(now - lastKeyAt_, keepOn);
  }
}
