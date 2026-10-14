#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_mac.h>

#include <mutex>

#include "log.h"

namespace {
// One Preferences object, used by the loop and by the phone-setup page's HTTP task: every
// begin() ... end() runs under this lock, so the two never interleave on it.
Preferences g_prefs;
std::mutex g_prefsLock;
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
      "  api       which AI server is used: api proxy | api worker [https://api...] (shows it without arguments)\n"
      "  register  (Worker) register this calculator again now: a new, unlinked token and link code\n"
      "  enroll hs_...  QA only: store this board's enrolment secret (from scripts/enroll-board.mjs)\n"
      "  forget    erase the hotspot, servers and tokens from this calculator (not the enrolment)\n"
      "  keys ..   press calculator keys, e.g.  keys 1/3=   (full list in README)\n"
      "  bright N  screen brightness 10-100 % (saved; SHIFT + up/down on the keypad does the same)\n"
      "  screen dim|off|sleep N   seconds without a key before the light dims / goes off /\n"
      "            the panel sleeps (0 = never); e.g. screen off 120\n"
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
    Serial.println("The calculator no longer takes an AI service key: the key lives on the proxy server.\n"
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
  } else if (line.rfind("bright ", 0) == 0) {
    arg = trim(line.substr(7));
    return SerialCmd::Bright;
  } else if (line.rfind("screen ", 0) == 0) {
    arg = trim(line.substr(7));
    return SerialCmd::Screen;
  } else if (line == "exam off") {
    return SerialCmd::ExamOff;
  } else if (line == "api") {
    Serial.printf("AI server: %s (%s), token %s%s\n", s.worker() ? "Worker" : "proxy",
                  s.serverUrl().empty() ? "address not set" : s.serverUrl().c_str(), s.tokenHint().c_str(),
                  s.worker() ? (factoryEnrolled() ? ", board enrolled" : ", board not enrolled") : "");
  } else if (line == "api proxy") {
    s.api = Api::Proxy;
    settingsSave(s);
    LOGF("settings", "AI server: the proxy (%s)", s.proxyUrl.empty() ? "address not set: type proxy" : s.proxyUrl.c_str());
    return SerialCmd::Changed;
  } else if (line == "api worker" || line.rfind("api worker ", 0) == 0) {
    std::string url = line.size() > 10 ? trim(line.substr(10)) : "";
    if (url.empty()) {
      if (s.workerUrl.empty()) {
        Serial.println("Give the Worker's address: api worker https://api.example.com");
        return SerialCmd::None;
      }
    } else {
      const std::string why = settingsCheckWorker(url);
      if (!why.empty()) {
        Serial.printf("%s\n", why.c_str());
        return SerialCmd::None;
      }
      if (url != s.workerUrl) {  // a token belongs to one server
        s.workerUrl = url;
        s.workerToken.clear();
        s.workerDeviceId.clear();
      }
    }
    s.api = Api::Worker;
    settingsSave(s);
    LOGF("settings", "AI server: the Worker at %s (%s)", s.workerUrl.c_str(),
         s.workerToken.empty() ? "registers on the next Wi-Fi connection" : "registered");
    return SerialCmd::Changed;
  } else if (line == "register") {
    return SerialCmd::Register;
  } else if (line == "enroll" || line.rfind("enroll ", 0) == 0) {
    const std::string secret = line.size() > 7 ? trim(line.substr(7)) : "";
    if (secret.size() != 46 || secret.rfind("hs_", 0) != 0 ||
        secret.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", 3) != std::string::npos) {
      Serial.println("That isn't an enrolment secret (hs_ + 43 characters from scripts/enroll-board.mjs).");
      return SerialCmd::None;
    }
    {
      std::lock_guard<std::mutex> lock(g_prefsLock);
      g_prefs.begin("factory", false);
      g_prefs.putString("hw_secret", secret.c_str());
      g_prefs.end();
    }
    // A token from before the enrolment belongs to an unenrolled device: register again.
    s.workerToken.clear();
    s.workerDeviceId.clear();
    settingsSave(s);
    LOGF("settings", "board enrolled (secret saved; it is never shown). %s", deviceId().c_str());
    return SerialCmd::Changed;
  } else if (line == "forget") {
    s = Settings{};
    settingsSave(s);
    LOGF("settings", "hotspot, servers and tokens erased (the board's enrolment stays)");
    return SerialCmd::Changed;
  } else if (!line.empty()) {
    help();
  }
  return SerialCmd::None;
}
}  // namespace

void settingsSave(const Settings& s) {
  std::lock_guard<std::mutex> lock(g_prefsLock);
  g_prefs.begin("calc", false);
  g_prefs.putString("ssid", s.ssid.c_str());
  g_prefs.putString("pass", s.password.c_str());
  g_prefs.putString("proxy", s.proxyUrl.c_str());
  g_prefs.putString("token", s.deviceToken.c_str());
  g_prefs.putString("api", s.worker() ? "worker" : "proxy");
  g_prefs.putString("wurl", s.workerUrl.c_str());
  g_prefs.putString("wtoken", s.workerToken.c_str());
  g_prefs.putString("wdevid", s.workerDeviceId.c_str());
  g_prefs.end();
}

std::string factorySecret() {
  std::lock_guard<std::mutex> lock(g_prefsLock);
  // Read-write open: a read-only begin() of a namespace that was never written fails noisily.
  g_prefs.begin("factory", false);
  std::string secret = g_prefs.getString("hw_secret", "").c_str();
  g_prefs.end();
  return secret;
}

bool factoryEnrolled() { return !factorySecret().empty(); }

std::string settingsCheckWorker(std::string& url) {
  while (!url.empty() && url.back() == '/') url.pop_back();
  const bool clean = url.size() > kUrlMin && url.size() < kUrlMax && url.find_first_of(" \t\"\r\n") == std::string::npos;
#if CALC_ALLOW_HTTP
  const bool scheme = url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0;
  return clean && scheme ? "" : "The Worker's address must start with https:// (or http:// on a bench build) and have no spaces.";
#else
  return clean && url.rfind("https://", 0) == 0 ? "" : "The Worker's address must start with https:// and have no spaces.";
#endif
}

std::string settingsCheckWifi(const std::string& ssid, const std::string& password) {
  if (ssid.empty() || ssid.size() > kSsidMax) return "Hotspot name must be 1-32 characters.";
  if (!password.empty() && (password.size() < kPassMin || password.size() > kPassMax))
    return "Wi-Fi passwords are 8-63 characters (or empty for an open hotspot).";
  return "";
}

std::string settingsCheckProxy(std::string& url) {
  while (!url.empty() && url.back() == '/') url.pop_back();
  // No spaces, quotes or line breaks: a CR / LF would end the HTTP request line early.
  const bool ok = url.rfind("https://", 0) == 0 && url.size() > kUrlMin && url.size() < kUrlMax &&
                  url.find_first_of(" \t\"\r\n") == std::string::npos;
  return ok ? "" : "The AI server address must start with https:// and have no spaces.";
}

std::string settingsCheckToken(const std::string& token) {
  if (token.rfind("sk-ant-", 0) == 0)
    return "That looks like an AI service key. Never put it in the calculator: it belongs on the server only.";
  // CR / LF too: the token goes into an HTTP header.
  if (token.size() < kTokenMin || token.size() > kTokenMax || token.find_first_of(" \t\"\r\n") != std::string::npos)
    return "That doesn't look like a device token (dt_ + 40 characters).";
  return "";
}

std::string Settings::tokenHint() const {
  const std::string& t = serverToken();
  if (t.empty()) return worker() ? "not registered yet" : "not set";
  if (t.size() < 10) return "set";
  return t.substr(0, 5) + "..." + t.substr(t.size() - 2);
}

Settings settingsLoad() {
  Settings s;
  std::lock_guard<std::mutex> lock(g_prefsLock);
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
  s.api = g_prefs.getString("api", "proxy") == "worker" ? Api::Worker : Api::Proxy;
  s.workerUrl = g_prefs.getString("wurl", "").c_str();
  s.workerToken = g_prefs.getString("wtoken", "").c_str();
  s.workerDeviceId = g_prefs.getString("wdevid", "").c_str();
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
  std::lock_guard<std::mutex> lock(g_prefsLock);
  g_prefs.begin("calc", false);
  g_prefs.putBool("exam", active);
  g_prefs.putUInt("examMs", active ? elapsedMs : 0);
  g_prefs.end();
}

bool examLoad(uint32_t& elapsedMs) {
  std::lock_guard<std::mutex> lock(g_prefsLock);
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
