// Hotspot name/password, the AI proxy's address and this calculator's device token,
// kept in the chip's flash (NVS). Set them over USB (Serial Monitor, type "help") or from
// a phone (setup_portal.h); both write the same NVS keys.
//
// The calculator never holds a Claude API key. It talks to our proxy (server/proxy),
// which holds the key, checks the device token, the subscription and the monthly
// fair-use cap, and forwards the photo to Claude. A stolen token can only use that
// one calculator's allowance and is revoked on the server.
//
// Two servers (NVS "api"): "proxy" (server/proxy, the default: proxy address + a token
// typed in) or "worker" (the Cloudflare Worker API, docs/web/openapi.yaml: only its address
// is set; the calculator registers itself and keeps the token it is given). Serial:
// "api worker https://api.example.com", "api proxy", "api". The proxy's address and token are
// kept while the Worker is used, so switching back needs nothing typed again.
#pragma once
#include <cstdint>
#include <string>

enum class Api : uint8_t { Proxy, Worker };

struct Settings {
  std::string ssid, password;
  std::string proxyUrl;     // e.g. https://calc-proxy.example.com (no trailing slash)
  std::string deviceToken;  // issued by the proxy's admin tool, one per calculator
  Api api = Api::Proxy;
  std::string workerUrl;       // e.g. https://api.proofline.app (no trailing slash)
  std::string workerToken;     // dt_..., from POST /v1/device/register ("" = register on the next connection)
  std::string workerDeviceId;  // dev_..., from the same reply (status / logs only)
  bool worker() const { return api == Api::Worker; }
  // The server in use and the token for it.
  const std::string& serverUrl() const { return worker() ? workerUrl : proxyUrl; }
  const std::string& serverToken() const { return worker() ? workerToken : deviceToken; }
  // AI can be tried: the Worker needs only its address (the token comes by itself).
  bool aiReady() const { return worker() ? !workerUrl.empty() : !proxyUrl.empty() && !deviceToken.empty(); }
  std::string tokenHint() const;  // "dt_ab...9f" (of the server in use) for screens and status
};

enum class SerialCmd {
  None, Changed, Snap, Keys, ExamOff, Scan, Preview, PreviewOff, Pair, Account, SelfTest,
  Setup, SetupOff,  // phone setup network on / off
  Update,           // check the proxy for new firmware and install it
  Bright,           // v15: "bright N" (10-100 %), arg = N
  Screen,           // v15: "screen dim|off|sleep N" (seconds, 0 = never), arg = "dim N"
  Register          // Worker: register this calculator again now (a new, unlinked token)
};

// The board's enrolment secret (security review S1): written once at QA by the "enroll hs_..."
// serial command into NVS namespace "factory", which Forget / Unlink never erase. Sent only
// in POST /v1/device/register; never printed. "" when the board was not enrolled.
std::string factorySecret();
bool factoryEnrolled();

Settings settingsLoad();
void settingsSave(const Settings& s);

// Checks shared by the serial commands and the phone setup page. "" = fine, else
// the sentence to show. settingsCheckProxy also strips a trailing '/'.
std::string settingsCheckWifi(const std::string& ssid, const std::string& password);
std::string settingsCheckProxy(std::string& url);
// The Worker's address: https, or plain http only in a CALC_ALLOW_HTTP=1 bench build
// (a local wrangler dev on the same network). Strips a trailing '/'.
std::string settingsCheckWorker(std::string& url);
std::string settingsCheckToken(const std::string& token);
// Reads serial input without blocking; handles the setup commands.
// statusText() is printed by "status". For Keys, `arg` holds the characters
// to press (see keysForChar in core/device.h).
SerialCmd settingsPoll(Settings& s, std::string (*statusText)(), std::string& arg);

// Exam mode survives a restart (pulling the battery doesn't end it).
void examSave(bool active, uint32_t elapsedMs);
bool examLoad(uint32_t& elapsedMs);

// A stable id for this calculator (from its Wi-Fi MAC), e.g. "calc-1a2b3c4d5e6f".
std::string deviceId();
