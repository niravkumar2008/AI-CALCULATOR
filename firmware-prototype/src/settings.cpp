#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_mac.h>

#include "log.h"

namespace {
Preferences g_prefs;
std::string g_line;
enum class Ask { Nothing, Ssid, Password, Proxy, Token } g_ask = Ask::Nothing;
std::string g_newSsid;

constexpr size_t kSsidMax = 32, kPassMin = 8, kPassMax = 63;
constexpr size_t kUrlMin = 10, kUrlMax = 200, kTokenMin = 20, kTokenMax = 100;
constexpr size_t kLineMax = 256;  // longest valid input is ~200 characters

std::string trim(const std::string& v) {
  size_t b = v.find_first_not_of(" \t"), e = v.find_last_not_of(" \t");
  return b == std::string::npos ? "" : v.substr(b, e - b + 1);
}

void help() {
  Serial.println(
      "\nCommands:\n"
      "  wifi      set the hotspot name and password\n"
      "  proxy     set the AI proxy address (https://...)\n"
      "  token     set this calculator's device token (from the proxy's admin tool)\n"
      "  pair      get a code to link this calculator to an account\n"
      "  account   linked account, subscription and solves left this month\n"
      "  status    camera, Wi-Fi, battery and setup status\n"
      "  scan      list the Wi-Fi networks the calculator can see\n"
      "  preview   live camera view + settings on your phone (preview off to stop)\n"
      "  snap      take a test photo and report its size\n"
      "  selftest  factory self-test (keys, screen, camera, Wi-Fi, battery) -> JSON\n"
      "  setup     phone setup: a Wi-Fi network + page to enter all of the above (setup off to stop)\n"
      "  update    check the AI server for new firmware and install it (cable in)\n"
      "  forget    erase the hotspot, proxy and token from this calculator\n"
      "  keys ..   press calculator keys, e.g.  keys 1/3=   (full list in README)\n"
      "  exam off  end exam mode (this USB cable is the teacher's unlock)\n");
}

// Handles one finished line of input.
SerialCmd handle(Settings& s, const std::string& raw, std::string (*statusText)(), std::string& arg) {
  std::string line = g_ask == Ask::Password ? raw : trim(raw);  // passwords may have spaces
  switch (g_ask) {
    case Ask::Ssid:
      if (line.empty() || line.size() > kSsidMax) {
        Serial.printf("Hotspot name must be 1-%u characters. Type wifi to try again.\n", unsigned(kSsidMax));
        g_ask = Ask::Nothing;
        return SerialCmd::None;
      }
      g_newSsid = line;
      g_ask = Ask::Password;
      Serial.println("Hotspot password (8-63 characters, or empty for an open hotspot):");
      return SerialCmd::None;
    case Ask::Password: {
      g_ask = Ask::Nothing;
      const std::string why = settingsCheckWifi(g_newSsid, line);
      if (!why.empty()) {
        Serial.printf("%s Type wifi to try again.\n", why.c_str());
        return SerialCmd::None;
      }
      s.ssid = g_newSsid;
      s.password = line;
      settingsSave(s);
      LOGF("settings", "saved: hotspot \"%s\" is used in AI SOLVE", s.ssid.c_str());
      return SerialCmd::Changed;
    }
    case Ask::Proxy: {
      g_ask = Ask::Nothing;
      const std::string why = settingsCheckProxy(line);
      if (!why.empty()) {
        Serial.printf("%s Type proxy to try again.\n", why.c_str());
        return SerialCmd::None;
      }
      s.proxyUrl = line;
      settingsSave(s);
      LOGF("settings", "proxy saved: %s", s.proxyUrl.c_str());
      return SerialCmd::Changed;
    }
    case Ask::Token: {
      g_ask = Ask::Nothing;
      const std::string why = settingsCheckToken(line);
      if (!why.empty()) {
        Serial.printf("%s Type token to try again.\n", why.c_str());
        return SerialCmd::None;
      }
      s.deviceToken = line;
      settingsSave(s);
      LOGF("settings", "token saved (%s)", s.tokenHint().c_str());
      return SerialCmd::Changed;
    }
    case Ask::Nothing: break;
  }
  if (line == "wifi") {
    g_ask = Ask::Ssid;
    Serial.println("Hotspot name (exactly as your phone shows it):");
  } else if (line == "proxy") {
    g_ask = Ask::Proxy;
    Serial.printf("Proxy address (now: %s):\n", s.proxyUrl.empty() ? "not set" : s.proxyUrl.c_str());
  } else if (line == "token") {
    g_ask = Ask::Token;
    Serial.println("Paste this calculator's device token and press Enter:");
  } else if (line == "key") {
    Serial.println("The calculator no longer takes a Claude API key: the key lives on the proxy server.\n"
                   "Type proxy and token instead (see firmware/FIRMWARE_STAGE13.md).");
  } else if (line == "status") {
    Serial.println(statusText().c_str());
  } else if (line == "pair") {
    return SerialCmd::Pair;
  } else if (line == "account") {
    return SerialCmd::Account;
  } else if (line == "selftest") {
    return SerialCmd::SelfTest;
  } else if (line == "setup") {
    return SerialCmd::Setup;
  } else if (line == "setup off") {
    return SerialCmd::SetupOff;
  } else if (line == "update") {
    return SerialCmd::Update;
  } else if (line == "preview") {
    return SerialCmd::Preview;
  } else if (line == "preview off") {
    return SerialCmd::PreviewOff;
  } else if (line == "scan") {
    return SerialCmd::Scan;
  } else if (line == "snap") {
    return SerialCmd::Snap;
  } else if (line.rfind("keys ", 0) == 0) {
    arg = line.substr(5);
    return SerialCmd::Keys;
  } else if (line == "exam off") {
    return SerialCmd::ExamOff;
  } else if (line == "forget") {
    s = Settings{};
    settingsSave(s);
    LOGF("settings", "hotspot, proxy and token erased");
    return SerialCmd::Changed;
  } else if (!line.empty()) {
    help();
  }
  return SerialCmd::None;
}
}  // namespace

void settingsSave(const Settings& s) {
  g_prefs.begin("calc", false);
  g_prefs.putString("ssid", s.ssid.c_str());
  g_prefs.putString("pass", s.password.c_str());
  g_prefs.putString("proxy", s.proxyUrl.c_str());
  g_prefs.putString("token", s.deviceToken.c_str());
  g_prefs.end();
}

std::string settingsCheckWifi(const std::string& ssid, const std::string& password) {
  if (ssid.empty() || ssid.size() > kSsidMax) return "Hotspot name must be 1-32 characters.";
  if (!password.empty() && (password.size() < kPassMin || password.size() > kPassMax))
    return "Wi-Fi passwords are 8-63 characters (or empty for an open hotspot).";
  return "";
}

std::string settingsCheckProxy(std::string& url) {
  while (!url.empty() && url.back() == '/') url.pop_back();
  const bool ok = url.rfind("https://", 0) == 0 && url.size() > kUrlMin && url.size() < kUrlMax &&
                  url.find_first_of(" \t\"") == std::string::npos;
  return ok ? "" : "The AI server address must start with https:// and have no spaces.";
}

std::string settingsCheckToken(const std::string& token) {
  if (token.rfind("sk-ant-", 0) == 0)
    return "That is a Claude API key. Never put it in the calculator: it belongs on the proxy server only.";
  if (token.size() < kTokenMin || token.size() > kTokenMax || token.find_first_of(" \t\"") != std::string::npos)
    return "That doesn't look like a device token (dt_ + 40 characters).";
  return "";
}

std::string Settings::tokenHint() const {
  if (deviceToken.empty()) return "not set";
  if (deviceToken.size() < 10) return "set";
  return deviceToken.substr(0, 5) + "..." + deviceToken.substr(deviceToken.size() - 2);
}

Settings settingsLoad() {
  Settings s;
  g_prefs.begin("calc", false);
  // Older firmware kept a Claude API key here. This firmware never uses one:
  // erase it so the calculator holds no key at all.
  if (g_prefs.isKey("key")) {
    g_prefs.remove("key");
    LOGF("settings", "erased an old Claude API key from this calculator (the key now lives on the proxy)");
  }
  s.ssid = g_prefs.getString("ssid", "").c_str();
  s.password = g_prefs.getString("pass", "").c_str();
  s.proxyUrl = g_prefs.getString("proxy", "").c_str();
  s.deviceToken = g_prefs.getString("token", "").c_str();
  g_prefs.end();
  return s;
}

SerialCmd settingsPoll(Settings& s, std::string (*statusText)(), std::string& arg) {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == 8 || c == 127) {  // backspace
      if (!g_line.empty()) g_line.pop_back();
      continue;
    }
    if (c != '\n') {
      if (g_line.size() < kLineMax) g_line += c;
      continue;
    }
    std::string line;
    line.swap(g_line);
    SerialCmd r = handle(s, line, statusText, arg);
    if (r != SerialCmd::None) return r;
  }
  return SerialCmd::None;
}

void examSave(bool active, uint32_t elapsedMs) {
  g_prefs.begin("calc", false);
  g_prefs.putBool("exam", active);
  g_prefs.putUInt("examMs", active ? elapsedMs : 0);
  g_prefs.end();
}

bool examLoad(uint32_t& elapsedMs) {
  g_prefs.begin("calc", true);
  bool on = g_prefs.getBool("exam", false);
  elapsedMs = g_prefs.getUInt("examMs", 0);
  g_prefs.end();
  return on;
}

std::string deviceId() {
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[24];
  snprintf(buf, sizeof buf, "calc-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return buf;
}
