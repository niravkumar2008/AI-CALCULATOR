#include "app.h"

#include "text.h"

namespace calc {

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
  if (!online_) return showMessage("NO CONNECTION", "Turn on your phone's hotspot.", true);
  activeId_ = pendingId_ = nextId_++;
  captured_ = false;
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
      else if (k == Key::Eq && (screen_ == Screen::Result || retryable_)) startRequest();
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
  if (!accepts(id) || confidence < kUnclearThreshold || answer.empty()) return;
  if (streaming_ && result_.answer == answer) return;
  const int keepTop = streaming_ ? top_ : 0;
  result_ = SolveResult();
  result_.answer = answer;
  showResult(false);
  lines_.push_back({"Steps coming...", false});
  streaming_ = true;
  scrollBy(keepTop);
}

void App::onReply(int id, const std::string& json) {
  if (!accepts(id)) return;
  activeId_ = 0;
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
  if (result_.confidence >= kUnclearThreshold) {
    showResult(false);
    return scrollBy(keepTop);
  }

  std::vector<std::string> items;
  for (const auto& u : result_.unclear) items.push_back("- " + u);
  if (items.empty()) items.push_back("Parts were hard to read.");
  setLines("! PHOTO UNCLEAR", items, "=retake \xE2\x86\x93" "answer");  // "=retake ↓answer"
  screen_ = Screen::Warning;
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
  setLines(unclear ? "ANSWER (unclear)" : "ANSWER", {result_.answer}, "");
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
      fb.drawText(0, kFirst + 1, "Aim the camera at a");
      fb.drawText(0, kFirst + 2, "problem and press =");
      fb.drawText(0, kFirst + 3, !hasKey_ ? "No API key" : (online_ ? "Hotspot OK" : "No connection"));
      fb.drawText(0, kFirst + 5, "AC:calculator MODE:menu");
      return;
    case Screen::Busy: {
      int dots = static_cast<int>(((nowMs_ - busySinceMs_) / 400) % 4);
      fb.drawText(0, kFirst, (captured_ ? "Solving" : "Reading") + std::string(dots, '.'));
      fb.drawText(0, kFirst + 1, captured_ ? "Claude is working" : "Hold still");
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
