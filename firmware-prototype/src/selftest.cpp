#include "selftest.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_chip_info.h>

#include <string>
#include <vector>

#include "camera.h"
#include "claude_client.h"
#include "framebuffer.h"
#include "keys.h"
#include "log.h"
#include "ota.h"
#include "pins.h"
#include "power.h"
#include "screen.h"
#include "settings.h"

using calc::DKey;
using calc::Framebuffer;

namespace {
constexpr int kKeyCount = int(DKey::Count);  // 49 matrix keys + ON = 50
constexpr uint32_t kKeyTimeoutMs = 90000;
constexpr uint32_t kResultShownMs = 60000;    // the PASS / FAIL screen stays this long
constexpr uint32_t kPatternShownMs = 1500;    // a moment to look at the checkerboard
constexpr uint32_t kKeyScreenEveryMs = 1500;  // e-paper: key count redrawn at most this often
constexpr int kDisplayMinMs = 300, kDisplayMaxMs = 9000;  // a real full refresh takes 1.5-3 s
constexpr int kBatteryMinMv = 3000, kBatteryMaxMv = 4300; // a connected cell
constexpr int kWifiMinRssi = -80;

std::string jsonStr(const std::string& s) {
  std::string o = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\';
    if (static_cast<unsigned char>(c) < 0x20) continue;
    o += c;
  }
  return o + "\"";
}

struct Json {
  std::string s = "{";
  void add(const char* k, const std::string& raw) {
    if (s.size() > 1) s += ",";
    s += "\"" + std::string(k) + "\":" + raw;
  }
  void str(const char* k, const std::string& v) { add(k, jsonStr(v)); }
  void num(const char* k, long v) { add(k, std::to_string(v)); }
  void boolean(const char* k, bool v) { add(k, v ? "true" : "false"); }
  std::string done() const { return s + "}"; }
};

Framebuffer g_fb;
void show(const std::vector<std::string>& lines) {
  g_fb.clear();
  for (size_t i = 0; i < lines.size() && i < size_t(Framebuffer::kRows); ++i)
    g_fb.drawText(0, int(i), lines[i], i == 0);
  screenShow(g_fb, false);
}

bool g_seen[kKeyCount];
int g_seenCount = 0;
int g_acPresses = 0;
void onTestKey(DKey k) {
  if (k >= DKey::Count) return;
  if (!g_seen[int(k)]) {
    g_seen[int(k)] = true;
    ++g_seenCount;
    LOGF("selftest", "key %-6s ok (%d/%d)", calc::dkeyName(k), g_seenCount, kKeyCount);
  }
  g_acPresses = k == DKey::AC ? g_acPresses + 1 : 0;
}
}  // namespace

bool selfTestComboHeld() { return keysHeld(DKey::Shift) && keysHeld(DKey::Alpha); }

[[noreturn]] void selfTestRun(const char* trigger) {
  LOGF("selftest", "start (%s)", trigger);
  Json j;
  bool pass = true;
  j.str("firmware", kFirmwareVersion);
  j.str("firmware_slot", otaSlotName());
  j.str("device_id", deviceId());
  j.str("trigger", trigger);
  j.str("reset_reason", resetReasonText());

  // 1. Chip and memory
  esp_chip_info_t chip;
  esp_chip_info(&chip);
  j.num("chip_rev", chip.revision);
  j.num("flash_mb", long(ESP.getFlashChipSize() / (1024 * 1024)));
  j.num("psram_kb", long(ESP.getPsramSize() / 1024));
  const bool memOk = ESP.getPsramSize() >= 2 * 1024 * 1024 - 65536 && ESP.getFlashChipSize() >= 4 * 1024 * 1024;
  j.boolean("memory_ok", memOk);
  pass &= memOk;

  // 2. Keypad scanner on I2C
  j.boolean("keypad_scanner_ok", keysScannerOk());
  pass &= keysScannerOk();

  // 3. Battery and charging
  const int mv = batteryMillivolts();
  j.num("battery_mv", mv);
  j.num("battery_pct", calc::lipoPercent(mv));
  j.boolean("vbus", usbPresent());
  j.boolean("chg_stat_high", statPinHigh());
  const calc::Charge ch = chargeState();
  j.str("charge", ch == calc::Charge::Charging ? "charging" : ch == calc::Charge::Full ? "full" : "no_cable");
  // A connected cell reads 3.0-4.25 V. No cell (or a reversed one behind Q1) reads
  // ~0 V, or the charger's 4.2 V regulation with a cable in.
  const bool batOk = mv >= kBatteryMinMv && mv <= kBatteryMaxMv;
  j.boolean("battery_ok", batOk);
  pass &= batOk;

  // 4. E-paper pattern: the refresh time proves BUSY and the booster work.
  show({"SELF-TEST", "Screen pattern..."});
  Framebuffer pat;
  pat.drawText(0, 4, "SELF-TEST: check the");
  pat.drawText(0, 5, "squares and the border");
  pat.drawText(0, 6, deviceId());
  const uint32_t refreshMs = screenTestPattern(pat);
  j.num("display_refresh_ms", long(refreshMs));
  const bool epdOk = refreshMs > uint32_t(kDisplayMinMs) && refreshMs < uint32_t(kDisplayMaxMs);
  j.boolean("display_ok", epdOk);
  pass &= epdOk;
  delay(kPatternShownMs);

  // 5. Camera
  show({"SELF-TEST", "Camera..."});
  std::string sensor;
  size_t bytes = 0;
  uint32_t camMs = 0;
  bool af = false;
  const bool camOk = cameraSelfTest(sensor, bytes, camMs, af);
  j.str("camera", sensor);
  j.num("camera_jpeg_bytes", long(bytes));
  j.num("camera_ms", long(camMs));
  j.boolean("camera_autofocus", af);
  j.boolean("camera_ok", camOk);
  pass &= camOk;
  cameraSleep();

  // 6. Wi-Fi: scan (no password needed), strongest signal, battery dip while the radio works.
  show({"SELF-TEST", "Wi-Fi scan..."});
  WiFi.mode(WIFI_STA);
  powerLimitWifi();
  const int n = WiFi.scanNetworks();
  int best = -127;
  for (int i = 0; i < n; ++i) best = std::max(best, int(WiFi.RSSI(i)));
  const int mvRadio = batteryMillivolts();
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
  j.num("wifi_networks", n < 0 ? 0 : n);
  j.num("wifi_best_rssi", n > 0 ? best : 0);
  j.num("wifi_tx_cap_dbm_x4", kWifiMaxTxQuarterDbm);
  j.num("battery_mv_radio_on", mvRadio);
  const bool wifiOk = n > 0 && best > kWifiMinRssi;
  j.boolean("wifi_ok", wifiOk);
  pass &= wifiOk;

  // 7. Every key (needs a person): press all 50, or AC twice in a row to stop early.
  g_seenCount = 0;
  g_acPresses = 0;
  for (bool& s : g_seen) s = false;
  LOGF("selftest", "press every key once (ON too); AC twice in a row (or q here) to stop early");
  uint32_t t0 = millis(), shownAt = 0;
  int shownCount = -1;
  while (g_seenCount < kKeyCount && millis() - t0 < kKeyTimeoutMs && g_acPresses < 2) {
    keysPoll(onTestKey);
    if (Serial.available() && Serial.read() == 'q') break;  // "q" in the Serial Monitor skips
    if (g_seenCount != shownCount && millis() - shownAt > kKeyScreenEveryMs) {
      shownCount = g_seenCount;
      shownAt = millis();
      show({"SELF-TEST: KEYS", "Press every key once.", std::to_string(g_seenCount) + " of " + std::to_string(kKeyCount) + " done",
            "AC AC: stop early", "", std::to_string((kKeyTimeoutMs - (millis() - t0)) / 1000) + " s left"});
    }
    delay(5);
  }
  std::string missing;
  for (int k = 0; k < kKeyCount; ++k)
    if (!g_seen[k]) missing += std::string(missing.empty() ? "" : ",") + jsonStr(calc::dkeyName(DKey(k)));
  j.num("keys_ok", g_seenCount);
  j.num("keys_total", kKeyCount);
  j.add("keys_missing", "[" + missing + "]");
  const bool keysOk = g_seenCount == kKeyCount;
  j.boolean("keys_all_ok", keysOk);
  pass &= keysOk;

  j.boolean("pass", pass);
  Serial.printf("SELFTEST_JSON %s\n", j.done().c_str());
  show({pass ? "SELF-TEST PASS" : "SELF-TEST FAIL", std::string("Keys ") + std::to_string(g_seenCount) + "/50" + (keysOk ? " ok" : ""),
        std::string("Screen ") + (epdOk ? "ok" : "FAIL") + "  Camera " + (camOk ? "ok" : "FAIL"),
        std::string("Wi-Fi ") + (wifiOk ? std::to_string(best) + " dBm" : "FAIL") + "  Bat " + std::to_string(mv) + "mV",
        std::string("Charge ") + (ch == calc::Charge::Charging ? "charging" : ch == calc::Charge::Full ? "full" : "no cable"),
        "Any key: restart"});
  // Wait for a key (or a minute), then start the calculator normally.
  t0 = millis();
  bool any = false;
  while (!any && millis() - t0 < kResultShownMs) {
    keysPoll([](DKey) {});
    any = digitalRead(PIN_KEY_ON) == LOW;
    if (Serial.available()) any = true;
    for (int k = 0; k < kKeyCount && !any; ++k) any = keysHeld(DKey(k));
    delay(20);
  }
  Serial.flush();
  ESP.restart();
  while (true) {}
}
