#include "app.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

#include "calc_engine.h"
#include "text.h"

namespace calc {

namespace {
bool digitAt(const std::string& s, size_t k) { return k < s.size() && isdigit(static_cast<unsigned char>(s[k])); }

// First number in s: optional sign (- or U+2212), digits with thousands commas, a decimal part,
// an exponent written E-5, e-5, *10^-5 or ×10^-5, or a /denominator. False if none.
bool firstNumber(const std::string& s, double& out) {
  static const std::string kMinus = "\xE2\x88\x92", kTimes10 = "\xC3\x97" "10^";
  for (size_t i = 0; i < s.size(); ++i) {
    size_t k = i;
    bool neg = false;
    if (s[k] == '-') {
      neg = true;
      ++k;
    } else if (s.compare(k, kMinus.size(), kMinus) == 0) {
      neg = true;
      k += kMinus.size();
    }
    if (!(digitAt(s, k) || (k < s.size() && s[k] == '.' && digitAt(s, k + 1)))) continue;
    if (i > 0 && (isalpha(static_cast<unsigned char>(s[i - 1])) || s[i - 1] == '_')) continue;  // x2, H2O
    std::string num;
    while (k < s.size()) {
      if (digitAt(s, k) || s[k] == '.') {
        num += s[k++];
      } else if (s[k] == ',' && digitAt(s, k + 1) && digitAt(s, k + 2) && digitAt(s, k + 3) && !digitAt(s, k + 4)) {
        ++k;  // 1,200
      } else {
        break;
      }
    }
    double v = std::atof(num.c_str());
    size_t e = k;
    while (e < s.size() && s[e] == ' ') ++e;
    size_t d = std::string::npos;
    if (e < s.size() && (s[e] == 'E' || s[e] == 'e') && (digitAt(s, e + 1) || (e + 1 < s.size() && (s[e + 1] == '-' || s[e + 1] == '+'))))
      d = e + 1;
    else if (s.compare(e, kTimes10.size(), kTimes10) == 0)
      d = e + kTimes10.size();
    else if (e + 4 <= s.size() && s.compare(e, 4, "*10^") == 0)
      d = e + 4;
    if (d != std::string::npos) {
      int sign = 1;
      if (d < s.size() && (s[d] == '-' || s[d] == '+')) {
        sign = s[d] == '-' ? -1 : 1;
        ++d;
      } else if (s.compare(d, kMinus.size(), kMinus) == 0) {
        sign = -1;
        d += kMinus.size();
      }
      std::string ex;
      while (digitAt(s, d)) ex += s[d++];
      if (!ex.empty()) v *= std::pow(10.0, sign * std::atof(ex.c_str()));
    } else if (k < s.size() && s[k] == '/' && digitAt(s, k + 1)) {
      std::string den;
      size_t q = k + 1;
      while (digitAt(s, q) || (q < s.size() && s[q] == '.')) den += s[q++];
      const double dv = std::atof(den.c_str());
      if (dv != 0) v /= dv;
    }
    out = neg ? -v : v;
    return true;
  }
  return false;
}

double noRandom() { return 0.5; }
}  // namespace

Verify verifyAnswer(const std::string& answer, const std::string& check, std::string& computed) {
  computed.clear();
  if (check.empty()) return Verify::None;
  std::vector<Tok> toks;
  if (!parseExpression(check, toks)) return Verify::None;
  Vars vars;
  const EvalResult r = evaluate(toks, vars, AngleUnit::Deg, noRandom);
  if (r.error != CalcError::None || !std::isfinite(r.value.v)) return Verify::None;
  computed = formatResult(r.value, false, NormMode::Norm1);
  // The number Claude gave: after "(C)" and after the last '=' ("x = 4.2 m").
  std::string a = answer;
  if (a.size() > 2 && a[0] == '(') {
    const size_t close = a.find(')');
    if (close != std::string::npos && close <= 4) a = a.substr(close + 1);
  }
  const size_t eq = a.rfind('=');
  if (eq != std::string::npos) a = a.substr(eq + 1);
  double given;
  if (!firstNumber(a, given)) return Verify::None;
  const double want = r.value.v;
  // Claude rounds to the data's significant figures: 1 % covers 3-figure rounding.
  const double tol = std::max(1e-9, 0.01 * std::fabs(want));
  return std::fabs(given - want) <= tol ? Verify::Verified : Verify::Mismatch;
}

namespace {
constexpr int kCols = Framebuffer::kCols;
constexpr int kFirst = 1;                      // row 0 is the device's status bar
constexpr int kRows = Framebuffer::kRows - 1;  // 6 content rows
}  // namespace

int App::takeRequest() {
  int id = pendingId_;
  pendingId_ = 0;
  return id;
}

void App::startRequest() {
  if (!hasKey_) return showMessage("NO API KEY", noKeyHelp_, false);
  if (lowBattery_) return showMessage("BATTERY LOW", "Plug in the cable to use AI. Maths still works.", false);
  if (!online_) return showMessage("NO CONNECTION", "Turn on your phone's hotspot.", true);
  activeId_ = pendingId_ = nextId_++;
  captured_ = false;
  holdUntilMs_ = 0;
  streaming_ = false;
  busySinceMs_ = nowMs_;
  screen_ = Screen::Busy;
}

void App::onKey(Key k) {
  if (k == Key::Off) {
    screen_ = Screen::Off;
    activeId_ = pendingId_ = 0;
    streaming_ = false;
    return;
  }
  if (k == Key::AC && streaming_) {  // stop the rest of the reply
    activeId_ = 0;
    streaming_ = false;
  }
  switch (screen_) {
    case Screen::Off:
      if (k == Key::On) screen_ = Screen::Ready;
      break;
    case Screen::Ready:
      if (k == Key::Eq) startRequest();
      else if (k == Key::Tutor) tutor_ = !tutor_;
      else if (k == Key::Up && effort_ != Effort::Max) effort_ = Effort(int(effort_) + 1);
      else if (k == Key::Down && effort_ != Effort::Normal) effort_ = Effort(int(effort_) - 1);
      break;
    case Screen::Busy:
      if (k == Key::AC) {
        activeId_ = pendingId_ = 0;
        screen_ = Screen::Ready;
      }
      break;
    case Screen::Warning:
      if (k == Key::Down) showResult(true);
      else if (k == Key::Eq) startRequest();
      else if (k == Key::AC) screen_ = Screen::Ready;
      break;
    case Screen::Result:
    case Screen::Message:
      if (k == Key::Up) scrollBy(-1);
      else if (k == Key::Down) scrollBy(1);
      else if (k == Key::AC) screen_ = Screen::Ready;
      else if (k == Key::Eq && screen_ == Screen::Result && tutor_ && tutorShown_ &&
               revealed_ <= static_cast<int>(result_.steps.size())) {
        ++revealed_;  // next hint (the answer after the last step)
        showTutor();
      } else if (k == Key::Eq && (screen_ == Screen::Result || retryable_)) startRequest();
      break;
  }
}

void App::onCaptured(int id) {
  if (id == activeId_ && screen_ == Screen::Busy) captured_ = true;
}

bool App::accepts(int id) const {
  return id != 0 && id == activeId_ && (screen_ == Screen::Busy || streaming_);
}

void App::onPartialAnswer(int id, double confidence, const std::string& answer) {
  if (!accepts(id) || answer.empty()) return;
  if (tutor_) return;  // tutor mode: the answer comes last, after the steps
  if (streaming_ && result_.answer == answer) return;
  const int keepTop = streaming_ ? top_ : 0;
  result_ = SolveResult();
  result_.answer = answer;
  result_.confidence = confidence;
  showResult(confidence < kUnclearThreshold);
  lines_.push_back({"Steps coming...", false});
  streaming_ = true;
  scrollBy(keepTop);
}

void App::onReply(int id, const std::string& json) {
  if (!accepts(id)) return;
  activeId_ = 0;
  verify_ = Verify::None;
  tutorShown_ = false;
  const int keepTop = streaming_ ? top_ : 0;
  streaming_ = false;
  std::string err;
  if (!parseSolveResult(json, result_, err))
    return showMessage("REPLY ERROR", "Couldn't understand Claude's reply.", true);
  expression_ = result_.readable ? result_.expression : "";
  if (!result_.readable) {
    std::string body = "No problem found in the photo.";
    if (!result_.unclear.empty()) body += " " + result_.unclear.front();
    return showMessage("NOTHING TO SOLVE", body, true);
  }
  verify_ = verifyAnswer(result_.answer, result_.check, computed_);
  if (tutor_) {
    revealed_ = 0;
    return showTutor();
  }
  // Unsure readings still show the answer, marked as a best guess, with what
  // to check: a guess the user can verify beats no answer (= retakes).
  showResult(result_.confidence < kUnclearThreshold);
  scrollBy(keepTop);
}

void App::onFailure(int id, Failure f, const std::string& detail) {
  if (!accepts(id)) return;
  activeId_ = 0;
  streaming_ = false;
  auto body = [&](const char* fallback) { return detail.empty() ? std::string(fallback) : detail; };
  switch (f) {
    case Failure::NoConnection: return showMessage("NO CONNECTION", body("Check your phone's hotspot."), true);
    case Failure::NoApiKey: return showMessage("NO API KEY", body(noKeyHelp_.c_str()), false);
    case Failure::Timeout: return showMessage("TIMED OUT", "Claude took too long to reply.", true);
    case Failure::ApiBusy: return showMessage("CLAUDE BUSY", "Try again in a moment.", true);
    case Failure::ApiError: return showMessage("CLAUDE ERROR", body("The request was refused."), true);
    case Failure::BadReply: return showMessage("REPLY ERROR", "Couldn't understand Claude's reply.", true);
    case Failure::Camera: return showMessage("CAMERA ERROR", body("Couldn't take the photo."), true);
    case Failure::Account: return showMessage("AI ACCOUNT", body("Link this calculator to use AI."), true);
    case Failure::LowBattery:
      return showMessage("BATTERY LOW", body("Charge the calculator to use AI. Maths still works."), false);
  }
}

void App::setLines(const std::string& title, const std::vector<std::string>& paragraphs,
                   const std::string& hint) {
  lines_ = {{title, true}};
  for (const auto& p : paragraphs)
    for (auto& l : wrapText(p, kCols)) lines_.push_back({l, false});
  if (!hint.empty()) lines_.push_back({hint, false});
  top_ = 0;
}

void App::showResult(bool unclear) {
  const int pct = static_cast<int>(result_.confidence * 100 + 0.5);
  std::string head = unclear ? "BEST GUESS (" + std::to_string(pct) + "% sure)" : "ANSWER";
  std::string answer = result_.answer;
  if (!result_.choice.empty()) {  // multiple choice: the letter first, on its own
    head = (unclear ? "GUESS: CHOICE " : "CHOICE ") + result_.choice + (unclear ? " (" + std::to_string(pct) + "%)" : "");
    const std::string label = "(" + result_.choice + ")";
    if (answer.compare(0, label.size(), label) != 0) answer = label + " " + answer;
  }
  if (verify_ == Verify::Verified) head += " \xE2\x9C\x93";  // ✓ the calculator agrees
  setLines(head, {answer}, "");
  if (verify_ == Verify::Verified) lines_.push_back({"\xE2\x9C\x93 calculator agrees", false});
  if (verify_ == Verify::Mismatch) {
    lines_.push_back({"\xE2\x9A\xA0 CALCULATOR GETS", true});  // ⚠
    for (auto& l : wrapText(computed_ + " = " + result_.check, kCols)) lines_.push_back({l, false});
  }
  if (unclear) {
    lines_.push_back({"CHECK", true});
    for (const auto& u : result_.unclear)
      for (auto& l : wrapText("- " + u, kCols)) lines_.push_back({l, false});
    if (result_.unclear.empty()) lines_.push_back({"Parts were hard to read.", false});
    lines_.push_back({"=: retake the photo", false});
  }
  if (!result_.readAs.empty()) {
    lines_.push_back({"QUESTION", true});
    for (auto& l : wrapText(result_.readAs, kCols)) lines_.push_back({l, false});
  }
  const int n = static_cast<int>(result_.steps.size());
  for (int i = 0; i < n; ++i) {
    lines_.push_back({"STEP " + std::to_string(i + 1) + "/" + std::to_string(n), true});
    for (auto& l : wrapText(result_.steps[i], kCols)) lines_.push_back({l, false});
  }
  if (!result_.expression.empty()) {
    lines_.push_back({"ON THE CALCULATOR", true});
    for (auto& l : wrapText(result_.expression, kCols)) lines_.push_back({l, false});
    lines_.push_back({"AC: check it and edit it", false});
  }
  screen_ = Screen::Result;
}

void App::showTutor() {
  const int n = static_cast<int>(result_.steps.size());
  if (revealed_ > n + 1) revealed_ = n + 1;
  tutorShown_ = true;
  setLines("TUTOR: QUESTION", {result_.readAs.empty() ? std::string("(see your photo)") : result_.readAs}, "");
  for (int i = 0; i < revealed_ && i < n; ++i) {
    lines_.push_back({"HINT " + std::to_string(i + 1) + "/" + std::to_string(n), true});
    for (auto& l : wrapText(result_.steps[i], kCols)) lines_.push_back({l, false});
  }
  if (revealed_ > n) {
    lines_.push_back({std::string("ANSWER") + (verify_ == Verify::Verified ? " \xE2\x9C\x93" : ""), true});
    std::string answer = result_.answer;
    if (!result_.choice.empty() && answer.rfind("(" + result_.choice + ")", 0) != 0)
      answer = "(" + result_.choice + ") " + answer;
    for (auto& l : wrapText(answer, kCols)) lines_.push_back({l, false});
    if (verify_ == Verify::Mismatch) {
      lines_.push_back({"\xE2\x9A\xA0 CALCULATOR GETS", true});
      for (auto& l : wrapText(computed_, kCols)) lines_.push_back({l, false});
    }
    lines_.push_back({"=:new photo  AC:back", false});
  } else {
    lines_.push_back({revealed_ == n ? "=: show the answer" : (revealed_ == 0 ? "=: first hint" : "=: next hint"), false});
  }
  screen_ = Screen::Result;
  top_ = 0;
  scrollBy(1000);  // keep the newest hint in view
}

void App::showMessage(const std::string& title, const std::string& body, bool retryable) {
  retryable_ = retryable;
  setLines(title, {body}, retryable ? "=retry  AC:back" : "AC:back");
  screen_ = Screen::Message;
}

void App::scrollBy(int d) {
  int maxTop = static_cast<int>(lines_.size()) - kRows;
  top_ += d;
  if (top_ > maxTop) top_ = maxTop;
  if (top_ < 0) top_ = 0;
}

const char* App::effortParam() const {
  switch (effort_) {
    case Effort::Careful: return "xhigh";
    case Effort::Max: return "max";
    default: return "high";
  }
}

std::string App::takeExpression() {
  std::string e;
  e.swap(expression_);
  return e;
}

void App::render(Framebuffer& fb) const {
  switch (screen_) {
    case Screen::Off:
      return;
    case Screen::Ready:
      fb.drawText(0, kFirst, "AI SOLVE", true);
      if (lowBattery_) {  // the platform keeps the camera and Wi-Fi off until it is charged
        fb.drawText(0, kFirst + 1, "Battery low: plug in the");
        fb.drawText(0, kFirst + 2, "cable to use AI solve.");
      } else if (!hasKey_) {
        fb.drawText(0, kFirst + 1, "Not set up yet: SETUP 6");
        fb.drawText(0, kFirst + 2, "shows how (or USB help).");
      } else {
        fb.drawText(0, kFirst + 1, "Point the camera at the");
        fb.drawText(0, kFirst + 2, "problem and press =");
      }
      fb.drawText(0, kFirst + 3, std::string(lowBattery_ ? "Low battery" : !hasKey_ ? "Not set up"
                                                                           : (online_ ? "Hotspot OK" : "No connection")) +
                                     (tutor_ ? " 1:Tutor on" : " 1:Tutor off"));
      fb.drawText(0, kFirst + 4, std::string("Effort: ") +
                                     (effort_ == Effort::Max ? "Max" : effort_ == Effort::Careful ? "Careful" : "Normal") +
                                     " \xE2\x96\xB2\xE2\x96\xBC");  // ▲▼
      fb.drawText(0, kFirst + 5, "AC:calculator MODE:menu");
      return;
    case Screen::Busy: {
      int dots = static_cast<int>(((nowMs_ - busySinceMs_) / 400) % 4);
      if (!captured_ && holdUntilMs_ > nowMs_) {  // countdown in whole seconds: one refresh a second
        const uint32_t left = (holdUntilMs_ - nowMs_ + 999) / 1000;
        fb.drawText(0, kFirst, "HOLD STILL", true);
        fb.drawTextPx(0, (kFirst + 1) * 8 + 1, std::to_string(left) + " s", 2);
        fb.drawText(0, kFirst + 4, "Taking the photo");
      } else {
        fb.drawText(0, kFirst, (captured_ ? "Solving" : "Reading") + std::string(dots, '.'));
        fb.drawText(0, kFirst + 1, captured_ ? "Claude is working" : "Hold still");
      }
      fb.drawText(0, kFirst + 5, "AC:cancel");
      return;
    }
    default:
      for (int r = 0; r < kRows && top_ + r < static_cast<int>(lines_.size()); ++r)
        fb.drawText(0, kFirst + r, lines_[top_ + r].text, lines_[top_ + r].header);
      fb.setIcon(Icon::Up, top_ > 0);
      fb.setIcon(Icon::Down, top_ + kRows < static_cast<int>(lines_.size()));
  }
}

}  // namespace calc
