// The device's whole behaviour: take a picture on '=', show Claude's answer
// and the steps to it. Platform code (simulator or ESP32 firmware) feeds keys
// and network results in, and draws whatever render() produces.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "framebuffer.h"
#include "solve_result.h"

namespace calc {

enum class Key { On, Off, AC, Eq, Up, Down };

enum class Screen { Off, Ready, Busy, Warning, Result, Message };

enum class Failure { NoConnection, NoApiKey, Timeout, ApiBusy, ApiError, BadReply, Camera };

class App {
 public:
  static constexpr double kUnclearThreshold = 0.8;  // warn below this confidence

  void onKey(Key k);
  void setOnline(bool online) { online_ = online; }
  void setHasApiKey(bool has) { hasKey_ = has; }
  // What the "no API key" message tells the user to do.
  void setNoKeyHelp(const std::string& help) { noKeyHelp_ = help; }

  // After '=', takeRequest() returns a new request id once (0 otherwise).
  // The platform captures and sends the photo, calls onCaptured() when the
  // photo is taken, then onReply() or onFailure() with the same id. Results
  // for an id that is no longer active (e.g. cancelled with AC) are ignored.
  int takeRequest();
  void onCaptured(int id);
  // While the reply streams in: show the answer early (steps follow).
  void onPartialAnswer(int id, double confidence, const std::string& answer);
  void onReply(int id, const std::string& replyJson);
  void onFailure(int id, Failure f, const std::string& detail = "");
  void tick(uint32_t nowMs) { nowMs_ = nowMs; }

  // Draws into content rows 1-6 (row 0 is left for the status bar); does
  // not clear the framebuffer. Scroll arrows are reported through icons.
  void render(Framebuffer& fb) const;

  // The scanned expression in calculator notation, once per reply ("" if
  // none), so the device can load it into the calculator for review.
  std::string takeExpression();
  // How sure Claude was of its reading of the last reply (0..1).
  double lastConfidence() const { return result_.confidence; }
  int activeRequest() const { return activeId_; }  // 0 when nothing is in flight

  Screen screen() const { return screen_; }
  int scrollTop() const { return top_; }
  int lineCount() const { return static_cast<int>(lines_.size()); }

 private:
  struct Line {
    std::string text;
    bool header = false;  // drawn underlined
  };

  void startRequest();
  void setLines(const std::string& title, const std::vector<std::string>& paragraphs,
                const std::string& hint);
  void showResult(bool unclear);
  void showMessage(const std::string& title, const std::string& body, bool retryable);
  void scrollBy(int d);
  bool accepts(int id) const;

  Screen screen_ = Screen::Ready;
  bool online_ = true;
  bool hasKey_ = true;
  bool captured_ = false;
  bool retryable_ = false;
  bool streaming_ = false;  // showing an early answer while steps arrive
  std::string noKeyHelp_ = "Run setup: hold AC while turning on.";
  int nextId_ = 1;
  int activeId_ = 0;
  int pendingId_ = 0;
  uint32_t nowMs_ = 0;
  uint32_t busySinceMs_ = 0;
  SolveResult result_;
  std::string expression_;
  std::vector<Line> lines_;
  int top_ = 0;
};

}  // namespace calc
