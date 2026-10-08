// Sends one photo to the calculator's AI proxy (server/proxy) over HTTPS and streams
// the reply back. The proxy holds the Claude API key, checks this calculator's device
// token, subscription and monthly cap, and returns Claude's own streamed reply, so the
// calculator parses exactly what it would get from the Claude API.
// Runs on its own task; results come back through callbacks.
#pragma once
#include <functional>
#include <string>

#include "app.h"
#include "settings.h"

struct SolveCallbacks {
  std::function<bool()> cancelled;  // true once the user pressed AC
  std::function<void(double confidence, const std::string& answer)> partial;
  std::function<void(const std::string& replyJson)> reply;
  std::function<void(calc::Failure f, const std::string& detail)> fail;
};

// detail: optional close-up of the writing (see camera.h); "" for none.
// effort: App::effortParam(). tutor: ask for hint-sized steps (the proxy may route it).
void claudeSolve(const Settings& s, const std::string& jpeg, const std::string& detail, const char* effort,
                 bool tutor, const SolveCallbacks& cb);

// A small JSON request to the proxy (pairing, account status). method "GET" or "POST".
// Returns the HTTP status (0 = no connection) and the body.
int proxyRequest(const Settings& s, const char* method, const std::string& path, const std::string& body,
                 std::string& reply);

// A large GET from the proxy (the firmware image), handed to `sink` as it arrives
// (return false to stop). Returns the HTTP status (0 = no connection, `error` says
// why); contentLength is the Content-Length header, 0 if none.
int proxyDownload(const Settings& s, const std::string& path, const std::function<bool(const char*, size_t)>& sink,
                  size_t& contentLength, std::string& error);

// Sent to the proxy as x-firmware and reported by status/selftest. The proxy's
// /v1/firmware compares it with the image it holds (OTA, see ota.h).
constexpr const char* kFirmwareVersion = "stage14-2026.10.06";
