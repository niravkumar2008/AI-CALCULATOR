#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>

namespace {
Preferences g_prefs;
std::string g_line;
enum class Ask { Nothing, Ssid, Password, Key } g_ask = Ask::Nothing;
std::string g_newSsid;

void save(const Settings& s) {
  g_prefs.begin("calc", false);
  g_prefs.putString("ssid", s.ssid.c_str());
  g_prefs.putString("pass", s.password.c_str());
  g_prefs.putString("key", s.apiKey.c_str());
  g_prefs.end();
}

std::string trim(const std::string& v) {
  size_t b = v.find_first_not_of(" \t"), e = v.find_last_not_of(" \t");
  return b == std::string::npos ? "" : v.substr(b, e - b + 1);
}

void help() {
  Serial.println(
      "\nCommands:\n"
      "  wifi    set the hotspot name and password\n"
      "  key     set your Claude API key (not shown on screen)\n"
      "  status  camera, Wi-Fi and key status\n"
      "  scan    list the Wi-Fi networks the calculator can see\n"
      "  preview live camera view + settings on your phone (preview off to stop)\n"
      "  snap    take a test photo and report its size\n"
      "  forget  erase the hotspot and key from this calculator\n"
      "  keys .. press calculator keys, e.g.  keys 1/3=   (full list in README)\n"
      "  exam off  end exam mode (this USB cable is the teacher's unlock)\n");
}

// Handles one finished line of input.
SerialCmd handle(Settings& s, const std::string& raw, std::string (*statusText)(), std::string& arg) {
  std::string line = g_ask == Ask::Password ? raw : trim(raw);  // passwords may have spaces
  switch (g_ask) {
    case Ask::Ssid:
      if (line.empty() || line.size() > 32) {
        Serial.println("Hotspot name must be 1-32 characters. Type wifi to try again.");
        g_ask = Ask::Nothing;
        return SerialCmd::None;
      }
      g_newSsid = line;
      g_ask = Ask::Password;
      Serial.println("Hotspot password (8-63 characters, or empty for an open hotspot):");
      return SerialCmd::None;
    case Ask::Password:
      g_ask = Ask::Nothing;
      if (!line.empty() && (line.size() < 8 || line.size() > 63)) {
        Serial.println("Wi-Fi passwords are 8-63 characters. Type wifi to try again.");
        return SerialCmd::None;
      }
      s.ssid = g_newSsid;
      s.password = line;
      save(s);
      Serial.printf("Saved. Joining \"%s\"...\n", s.ssid.c_str());
      return SerialCmd::Changed;
    case Ask::Key:
      g_ask = Ask::Nothing;
      if (line.rfind("sk-ant-", 0) != 0 || line.size() < 30 || line.find_first_of(" \t") != std::string::npos) {
        Serial.println("That doesn't look like a Claude API key (they start with sk-ant-). Type key to try again.");
        return SerialCmd::None;
      }
      s.apiKey = line;
      save(s);
      Serial.printf("Key saved (ends ...%s).\n", s.apiKey.substr(s.apiKey.size() - 4).c_str());
      return SerialCmd::Changed;
    case Ask::Nothing: break;
  }
  if (line == "wifi") {
    g_ask = Ask::Ssid;
    Serial.println("Hotspot name (exactly as your phone shows it):");
  } else if (line == "key") {
    g_ask = Ask::Key;
    Serial.println("Paste your Claude API key and press Enter:");
  } else if (line == "status") {
    Serial.println(statusText().c_str());
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
    save(s);
    Serial.println("Hotspot and key erased.");
    return SerialCmd::Changed;
  } else if (!line.empty()) {
    help();
  }
  return SerialCmd::None;
}
}  // namespace

Settings settingsLoad() {
  Settings s;
  g_prefs.begin("calc", true);
  s.ssid = g_prefs.getString("ssid", "").c_str();
  s.password = g_prefs.getString("pass", "").c_str();
  s.apiKey = g_prefs.getString("key", "").c_str();
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
      if (g_line.size() < 256) g_line += c;  // longest valid input is ~110 chars
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
