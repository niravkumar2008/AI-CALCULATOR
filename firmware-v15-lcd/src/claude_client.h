// Sends one photo to the calculator's AI server over HTTPS and streams the reply back:
// the proxy (server/proxy) or the Cloudflare Worker (Settings::api; core/worker_api.h).
// The server holds the Claude API key, checks this calculator's device token, subscription
// and monthly cap, and returns Claude's own streamed reply, so the calculator parses
// exactly what it would get from the Claude API.
// Runs on its own task; results come back through callbacks.
#pragma once
#include <cstdint>
#include <functional>
#include <string>

#include "app.h"
#include "settings.h"
#include "worker_api.h"

struct SolveCallbacks {
  std::function<bool()> cancelled;  // true once the user pressed AC
  std::function<void(double confidence, const std::string& answer)> partial;
  std::function<void(const std::string& replyJson)> reply;
  std::function<void(calc::Failure f, const std::string& detail)> fail;
  // Worker only, optional: called (before fail) when the failure needs the loop to act:
  // forget the token and register again, the calculator was removed, or a link code arrived.
  std::function<void(const calc::WorkerFailure& wf)> worker;
};

// detail: optional close-up of the writing (see camera.h); "" for none.
// effort: App::effortParam(). tutor: ask for hint-sized steps (the proxy may route it).
void claudeSolve(const Settings& s, const std::string& jpeg, const std::string& detail, const char* effort,
                 bool tutor, const SolveCallbacks& cb);

// A small JSON request to the server in use (pairing, account status, registration).
// method "GET" or "POST". `auth` false sends no token (the Worker's /v1/device/register).
// Returns the HTTP status (0 = no connection) and the body.
int proxyRequest(const Settings& s, const char* method, const std::string& path, const std::string& body,
                 std::string& reply, bool auth = true);

// A large GET from the proxy (the firmware image), handed to `sink` as it arrives
// (return false to stop). Returns the HTTP status (0 = no connection, `error` says
// why); contentLength is the Content-Length header, 0 if none.
int proxyDownload(const Settings& s, const std::string& path, const std::function<bool(const char*, size_t)>& sink,
                  size_t& contentLength, std::string& error);

// millis() of the last byte sent or received on a proxy connection (any task). A solve
// that is still streaming (or receiving the proxy's keep-alive pings) keeps moving it.
uint32_t claudeLastActivityMs();

// Sent to the proxy as x-firmware and reported by status/selftest. The proxy's
// /v1/firmware compares it with the image it holds (OTA, see ota.h). A macro as well, so
// ota.cpp can build the version mark it looks for inside a downloaded image.
#define CALC_FIRMWARE_VERSION "v15lcd-2026.10.11"
constexpr const char* kFirmwareVersion = CALC_FIRMWARE_VERSION;
