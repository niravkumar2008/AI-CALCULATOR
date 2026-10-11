// Hotspot name/password, the AI proxy's address and this calculator's device token,
// kept in the chip's flash (NVS). Set them over USB (Serial Monitor, type "help") or from
// a phone (setup_portal.h); both write the same NVS keys.
//
// The calculator never holds a Claude API key. It talks to our proxy (server/proxy),
// which holds the key, checks the device token, the subscription and the monthly
// fair-use cap, and forwards the photo to Claude. A stolen token can only use that
// one calculator's allowance and is revoked on the server.
#pragma once
#include <string>

struct Settings {
  std::string ssid, password;
  std::string proxyUrl;     // e.g. https://calc-proxy.example.com (no trailing slash)
  std::string deviceToken;  // issued by the proxy's admin tool, one per calculator
  bool aiReady() const { return !proxyUrl.empty() && !deviceToken.empty(); }
  std::string tokenHint() const;  // "dt_ab...9f" for screens and status
};

enum class SerialCmd {
  None, Changed, Snap, Keys, ExamOff, Scan, Preview, PreviewOff, Pair, Account, SelfTest,
  Setup, SetupOff,  // phone setup network on / off
  Update            // check the proxy for new firmware and install it
};

Settings settingsLoad();
void settingsSave(const Settings& s);

// Checks shared by the serial commands and the phone setup page. "" = fine, else
// the sentence to show. settingsCheckProxy also strips a trailing '/'.
std::string settingsCheckWifi(const std::string& ssid, const std::string& password);
std::string settingsCheckProxy(std::string& url);
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
