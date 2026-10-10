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

enum class Key { On, Off, AC, Eq, Up, Down, Tutor };

enum class Screen { Off, Ready, Busy, Warning, Result, Message };

enum class Failure { NoConnection, NoApiKey, Timeout, ApiBusy, ApiError, BadReply, Camera,
                     Account,      // the proxy refused: not linked / subscription / monthly cap (detail = its message)
                     LowBattery }; // the platform won't start Wi-Fi on a nearly empty cell

// Whether the calculator's own engine agreed with Claude's final number (SolveResult::check).
enum class Verify : uint8_t { None, Verified, Mismatch };

// Re-computes `check` (calculator notation) with the calculator engine and compares it with the
// first number in `answer` (after a multiple-choice label or the last '='). None when either
// side has no number. `computed` gets the engine's value as the calculator would show it.
Verify verifyAnswer(const std::string& answer, const std::string& check, std::string& computed);

// How hard Claude thinks: chosen with ▲ ▼ on the AI SOLVE screen. More is
// slower and costs more, and reads tricky math (exponents, limits) better.
enum class Effort : uint8_t { Normal, Careful, Max };

class App {
 public:
  static constexpr double kUnclearThreshold = 0.8;  // warn below this confidence

  void onKey(Key k);
  void setOnline(bool online) { online_ = online; }
  void setHasApiKey(bool has) { hasKey_ = has; }
  // The platform refuses Wi-Fi and the camera on a nearly empty battery (review S2):
  // the AI SOLVE screen says so and = shows BATTERY LOW instead of starting a scan.
  void setLowBattery(bool low) { lowBattery_ = low; }
  bool lowBattery() const { return lowBattery_; }
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
  // While a photo is being taken: when the camera will have its frames (so
  // the user can stop holding still). The busy screen counts down to it.
  // 0 = unknown (plain "Hold still").
  void setHoldUntil(uint32_t ms) { holdUntilMs_ = ms; }

  // Draws into content rows 1-6 (row 0 is left for the status bar); does
  // not clear the framebuffer. Scroll arrows are reported through icons.
  void render(Framebuffer& fb) const;

  // The scanned expression in calculator notation, once per reply ("" if
  // none), so the device can load it into the calculator for review.
  std::string takeExpression();
  // How sure Claude was of its reading of the last reply (0..1).
  double lastConfidence() const { return result_.confidence; }
  const SolveResult& result() const { return result_; }  // the last reply (e.g. its multiple-choice label)
  int activeRequest() const { return activeId_; }  // 0 when nothing is in flight

  Effort effort() const { return effort_; }
  void setEffort(Effort e) { effort_ = e; }
  // Tutor mode: a reply shows the question and one step at a time (= for the next hint);
  // the answer comes last. Toggled with Key::Tutor on the AI SOLVE home screen.
  bool tutor() const { return tutor_; }
  void setTutor(bool on) { tutor_ = on; }
  Verify verified() const { return verify_; }
  const char* effortParam() const;  // the API's name for it: "high", "xhigh", "max"

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
  void showTutor();
  void showMessage(const std::string& title, const std::string& body, bool retryable);
  void scrollBy(int d);
  bool accepts(int id) const;

  Screen screen_ = Screen::Ready;
  Effort effort_ = Effort::Normal;
  bool online_ = true;
  bool hasKey_ = true;
  bool lowBattery_ = false;
  bool captured_ = false;
  bool retryable_ = false;
  bool streaming_ = false;  // showing an early answer while steps arrive
  bool tutor_ = false;
  int revealed_ = 0;        // tutor mode: steps shown so far
  bool tutorShown_ = false; // the result screen is the tutor's
  Verify verify_ = Verify::None;
  std::string computed_;    // the engine's value for result_.check
  std::string noKeyHelp_ = "Run setup: hold AC while turning on.";
  int nextId_ = 1;
  int activeId_ = 0;
  int pendingId_ = 0;
  uint32_t nowMs_ = 0;
  uint32_t busySinceMs_ = 0;
  uint32_t holdUntilMs_ = 0;
  SolveResult result_;
  std::string expression_;
  std::vector<Line> lines_;
  int top_ = 0;
};

}  // namespace calc
