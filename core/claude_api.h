// Talking to the Claude Messages API, independent of how bytes are sent
// (WinHTTP in the simulator, the ESP32 HTTP client on the device).
#pragma once
#include <string>

#include "app.h"

namespace calc {

constexpr const char* kModel = "claude-sonnet-5-5";
constexpr const char* kApiHost = "api.anthropic.com";
constexpr const char* kApiPath = "/v1/messages";
constexpr const char* kApiVersion = "2023-06-01";

// The full request body for one photo (JPEG bytes, not yet base64-encoded).
// Streams the reply, asks for the fixed JSON reply format, and lets Claude
// think first when a problem needs it. `detail`, if not empty, is a
// full-resolution close-up of the writing in the same photo (see focus.h),
// sent as a second image so small characters get more pixels.
// `effort`: "high", "xhigh" or "max" (App::effortParam()).
std::string buildSolveRequest(const std::string& jpeg, const std::string& detail = "", const char* effort = "high");

std::string base64Encode(const std::string& bytes);

// The instructions and JSON schema sent with every photo (the web simulator
// sends the same ones through the artifact's own Claude access).
const char* solveInstructions();
const char* solveSchema();

// Reads the API's server-sent-event stream. Feed it raw chunks in any split.
class StreamReader {
 public:
  void feed(const std::string& chunk);
  const std::string& text() const { return text_; }  // reply text so far
  bool finished() const { return finished_; }         // message_stop seen
  const std::string& stopReason() const { return stopReason_; }
  const std::string& errorType() const { return errorType_; }  // from an error event
  const std::string& errorMessage() const { return errorMessage_; }

 private:
  void handleEvent(const std::string& data);
  std::string buf_, text_, stopReason_, errorType_, errorMessage_;
  bool finished_ = false;
};

// From a reply that is still arriving, returns true once "confidence" and the
// whole "answer" string have arrived, so the answer can be shown early.
bool peekAnswer(const std::string& partialJson, double& confidence, std::string& answer);

// Maps a finished request to a failure, or returns false if it succeeded.
// httpStatus is the HTTP code; body is the non-streamed error body (if any).
bool classifyFailure(int httpStatus, const std::string& body, const StreamReader& stream,
                     Failure& f, std::string& detail);

}  // namespace calc
