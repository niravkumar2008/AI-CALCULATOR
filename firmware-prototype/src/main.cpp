// Firmware for the final prototype (the custom board inside the Casio): the
// full calculator (Device: COMP maths, MODE / SETUP menus, AI SOLVE, exam
// mode), the same code as the simulators and the tester, with the OV5640
// autofocus camera, e-paper screen, the whole keypad (keys.cpp), Wi-Fi, the
// battery, and sleep while off. Starts in COMP like the Casio.
// Every key can also be pressed from the Serial Monitor: "keys 1/3=" (see
// README). "exam off" over USB is the teacher's unlock.
#include <Arduino.h>
#include <WiFi.h>

#include "device.h"
#include "camera.h"
#include "keys.h"
#include "claude_client.h"
#include "pins.h"
#include "preview.h"
#include "screen.h"
#include "settings.h"

using namespace calc;

// Headroom for JSON, TLS and the calculator's recursive parser (default is 8 KB).
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

namespace {

Device g_dev;
Framebuffer g_fb;
bool g_wasExam = false;
uint32_t g_examSavedAt = 0;
Settings g_settings;
bool g_cameraOk = false;

// ---- work handed from the loop to the solve task, and results back ----
struct Event {
  enum Kind { Captured, Partial, Reply, Fail } kind;
  int id;
  std::string text;
  double confidence = 0;
  Failure failure = Failure::ApiError;
};
QueueHandle_t g_requests;          // int ids, loop -> task
QueueHandle_t g_events;            // Event*, task -> loop
volatile int g_wantedId = 0;       // 0 = the user cancelled (AC / OFF)

// A photo taken before the request (Send on the preview page, which takes it
// while the camera is still aimed, then steps off to reach the hotspot).
// Written by the loop before the request is queued; read once by the task.
std::string g_prePhoto, g_preDetail;

void post(Event* e) {
  if (xQueueSend(g_events, &e, pdMS_TO_TICKS(1000)) != pdTRUE) delete e;
}

void solveTask(void*) {
  int id;
  while (true) {
    if (xQueueReceive(g_requests, &id, portMAX_DELAY) != pdTRUE) continue;
    if (g_wantedId != id) continue;
    uint32_t t0 = millis();
    std::string jpeg, detail, error;
    if (!g_prePhoto.empty()) {  // taken already by the preview page's Send
      jpeg.swap(g_prePhoto);
      detail.swap(g_preDetail);
    } else if (!cameraCapture(jpeg, detail, error, [id] { post(new Event{Event::Captured, id, ""}); })) {
      post(new Event{Event::Fail, id, error, 0, Failure::Camera});
      continue;
    }
    Serial.printf("[%d] photo %u bytes%s in %lu ms\n", id, (unsigned)jpeg.size(),
                  detail.empty() ? "" : (" + close-up " + std::to_string(detail.size())).c_str(), millis() - t0);
    post(new Event{Event::Captured, id, ""});
    SolveCallbacks cb;
    cb.cancelled = [id] { return g_wantedId != id; };
    cb.partial = [id, t0](double conf, const std::string& a) {
      Serial.printf("[%d] answer after %lu ms\n", id, millis() - t0);
      post(new Event{Event::Partial, id, a, conf});
    };
    cb.reply = [id, t0](const std::string& json) {
      Serial.printf("[%d] done after %lu ms\n", id, millis() - t0);
      post(new Event{Event::Reply, id, json});
    };
    cb.fail = [id](Failure f, const std::string& d) {
      Serial.printf("[%d] failed: %d %s\n", id, (int)f, d.c_str());
      post(new Event{Event::Fail, id, d, 0, f});
    };
    std::string key = g_settings.apiKey;  // copy: settings may change mid-request
    // Read from this task: safe because ▲ ▼ only change it on the Ready
    // screen, never while a request is in flight.
    claudeSolve(key, jpeg, detail, g_dev.ai().effortParam(), cb);
    std::string().swap(jpeg);
    std::string().swap(detail);
  }
}

// ---- Send from the preview page ----
enum class SendStep { Idle, WaitWifi, Solving } g_send = SendStep::Idle;
uint32_t g_sendAt = 0;
int g_sendId = 0;

void sendFinished(const std::string& reply, const std::string& error) {
  previewSetResult(reply, error);
  g_send = SendStep::Idle;
  g_sendId = 0;
  if (previewResume()) Serial.println("Preview is back: the page shows the answer.");
}

const char* failureText(Failure f) {
  switch (f) {
    case Failure::NoConnection: return "No connection: check the phone's hotspot.";
    case Failure::NoApiKey: return "No API key: type key in the Serial Monitor.";
    case Failure::Timeout: return "Claude took too long to reply.";
    case Failure::ApiBusy: return "Claude is busy: try again in a moment.";
    case Failure::ApiError: return "Claude refused the request.";
    case Failure::BadReply: return "Claude's reply couldn't be read.";
    case Failure::Camera: return "Couldn't take the photo.";
  }
  return "The scan failed.";
}

void handleEvent(Event* e) {
  switch (e->kind) {
    case Event::Captured: g_dev.onCaptured(e->id); break;
    case Event::Partial: g_dev.onPartialAnswer(e->id, e->confidence, e->text); break;
    case Event::Reply: g_dev.onReply(e->id, e->text); break;
    case Event::Fail: g_dev.onFailure(e->id, e->failure, e->text); break;
  }
  if (g_send == SendStep::Solving && e->id == g_sendId) {
    if (e->kind == Event::Reply) sendFinished(e->text, "");
    else if (e->kind == Event::Fail) sendFinished("", failureText(e->failure));
  }
  delete e;
}

// ---- keys ----
void press(DKey k) {
  Serial.printf("key %s\n", dkeyName(k));  // lets you test every button without a multimeter
  g_dev.tick(millis());
  g_dev.onKey(k);
  g_wantedId = g_dev.activeRequest();  // 0 after AC / MODE / OFF: the solve task stops
  if (int id = g_dev.takeRequest()) {
    g_wantedId = id;
    xQueueSend(g_requests, &id, 0);
  }
}

void typeKeys(const std::string& text) {
  for (char c : text) {
    if (c == ' ') continue;
    DKey ks[2];
    int n = 0;
    if (!keysForChar(c, ks, n)) {
      Serial.printf("No key for '%c' (type help, or see the README key list).\n", c);
      return;
    }
    for (int i = 0; i < n; ++i) press(ks[i]);
  }
}

// ---- Wi-Fi ----
// Why the last join attempt failed, in words (0 = none yet).
volatile uint8_t g_wifiReason = 0;
volatile bool g_wifiReasonNew = false;

const char* wifiReasonText(uint8_t r) {
  switch (r) {
    case WIFI_REASON_NO_AP_FOUND:
      return "can't see that hotspot. Check the name with scan; keep the phone's Personal Hotspot screen open";
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_802_1X_AUTH_FAILED:
      return "wrong password (type wifi to enter it again)";
    case WIFI_REASON_ASSOC_FAIL:
    case WIFI_REASON_AUTH_EXPIRE:
    case WIFI_REASON_BEACON_TIMEOUT:
      return "hotspot didn't answer; move the phone closer";
    default:
      return nullptr;
  }
}

void onWifiEvent(arduino_event_id_t event, arduino_event_info_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    const uint8_t r = info.wifi_sta_disconnected.reason;
    if (r != g_wifiReason) g_wifiReasonNew = true;
    g_wifiReason = r;
  } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    g_wifiReason = 0;
    Serial.printf("Wi-Fi connected: %s\n", WiFi.localIP().toString().c_str());
  }
}

// Reports a new failure reason once, not on every retry.
void reportWifi() {
  if (!g_wifiReasonNew) return;
  g_wifiReasonNew = false;
  const uint8_t r = g_wifiReason;
  const char* t = wifiReasonText(r);
  if (t) Serial.printf("Wi-Fi: %s (code %u)\n", t, r);
  else Serial.printf("Wi-Fi: not connected (code %u)\n", r);
}

void scanWifi() {
  Serial.println("Scanning (a few seconds)...");
  const int n = WiFi.scanNetworks();
  if (n <= 0) {
    Serial.println("No networks found. The chip only sees 2.4 GHz: on iPhone turn on Maximize Compatibility.");
    return;
  }
  bool found = false;
  for (int i = 0; i < n; ++i) {
    const String name = WiFi.SSID(i);
    const bool mine = !g_settings.ssid.empty() && name == g_settings.ssid.c_str();
    found |= mine;
    Serial.printf("  %s\"%s\"  signal %d dBm%s\n", mine ? "* " : "  ", name.c_str(), WiFi.RSSI(i),
                  mine ? "  <- your saved hotspot" : "");
  }
  if (!g_settings.ssid.empty() && !found)
    Serial.printf("Your saved hotspot \"%s\" is not in the list. Names must match exactly (capitals, apostrophes).\n",
                  g_settings.ssid.c_str());
  WiFi.scanDelete();
}

void connectWifi() {
  if (previewRunning()) previewStop();  // it shares the radio (a Send's pause is handled by its own step)
  WiFi.disconnect(true);
  if (g_settings.ssid.empty() || !g_dev.radiosAllowed()) {
    WiFi.mode(WIFI_OFF);
    return;
  }
  WiFi.mode(WIFI_STA);
  // 15 dBm instead of the maximum 19.5: plenty for a phone in the same room,
  // and smaller current spikes on a laptop's USB port.
  WiFi.setTxPower(WIFI_POWER_15dBm);
  WiFi.setAutoReconnect(true);
  g_wifiReason = 0;
  g_wifiReasonNew = false;
  WiFi.begin(g_settings.ssid.c_str(), g_settings.password.c_str());
}

}  // namespace

void previewEnded() { connectWifi(); }

namespace {

// ---- battery and charging (the prototype runs on its LiPo) ----
// The battery reads through a 1 M / 1 M divider (half its voltage).
// kBatteryCal trims it if the % disagrees with a multimeter (ADCs vary ~3%).
constexpr float kBatteryCal = 1.00f;

bool usbPlugged() { return digitalRead(PIN_VBUS) == HIGH; }
// MCP73831 STAT is low while charging (pulled down here when USB is out).
bool charging() { return usbPlugged() && digitalRead(PIN_CHG_STAT) == LOW; }

int batteryPercent() {
  const int mv = int(analogReadMilliVolts(PIN_BATTERY) * 2 * kBatteryCal);
  // ponytail: straight line from 3.3 V (empty) to 4.2 V (full); a LiPo
  // discharge-curve table if the % drops unevenly.
  return constrain((mv - 3300) / 9, 0, 100);
}

std::string statusText() {
  std::string s = "Camera: " + std::string(cameraName());
  s += "\nWi-Fi: " + (g_settings.ssid.empty() ? std::string("not set (type wifi)")
                                              : "\"" + g_settings.ssid + "\" " +
                                                    (WiFi.status() == WL_CONNECTED
                                                         ? "connected, " + std::string(WiFi.localIP().toString().c_str())
                                                         : std::string("not connected") +
                                                               (wifiReasonText(g_wifiReason)
                                                                    ? std::string(": ") + wifiReasonText(g_wifiReason)
                                                                    : std::string())));
  s += "\nKey: " + (g_settings.apiKey.empty() ? std::string("not set (type key)")
                                              : "saved (ends ..." + g_settings.apiKey.substr(g_settings.apiKey.size() - 4) + ")");
  s += "\nBattery: " + std::to_string(batteryPercent()) + "%" +
       (charging() ? " (charging)" : usbPlugged() ? " (USB, full)" : "");
  s += "\nFree PSRAM: " + std::to_string(ESP.getFreePsram() / 1024) + " KB";
  return s;
}

// Steps the Send through: photo -> hotspot -> Claude -> back to the page.
void serviceSend() {
  switch (g_send) {
    case SendStep::Idle: {
      if (!previewTakeSend()) return;
      if (g_dev.examActive()) return previewSetResult("", "Exam mode is on.");
      if (g_settings.apiKey.empty()) return previewSetResult("", "No API key: type key in the Serial Monitor.");
      if (g_settings.ssid.empty()) return previewSetResult("", "No hotspot set: type wifi in the Serial Monitor.");
      if (g_dev.isOff()) press(DKey::On);
      std::string err;
      g_preDetail.clear();
      if (!cameraCapture(g_prePhoto, g_preDetail, err)) {
        g_prePhoto.clear();
        return previewSetResult("", err);
      }
      Serial.println("Send: photo taken; leaving the preview network to reach Claude...");
      previewSuspend();
      connectWifi();
      g_send = SendStep::WaitWifi;
      g_sendAt = millis();
      return;
    }
    case SendStep::WaitWifi:
      if (WiFi.status() == WL_CONNECTED) {
        g_dev.tick(millis());
        g_dev.setOnline(true);
        if (g_dev.startScan()) {
          g_sendId = g_dev.takeRequest();
          g_wantedId = g_sendId;
          xQueueSend(g_requests, &g_sendId, 0);
          g_send = SendStep::Solving;
          g_sendAt = millis();
        } else {
          g_prePhoto.clear();
          sendFinished("", "The calculator couldn't start a scan.");
        }
      } else if (millis() - g_sendAt > 25000) {
        g_prePhoto.clear();
        sendFinished("", "Couldn't reach the hotspot: is it on, with the phone nearby?");
      }
      return;
    case SendStep::Solving:
      // The reply or failure arrives through handleEvent. A user who pressed
      // AC on the calculator cancelled it: bring the page back anyway.
      if (g_dev.activeRequest() != g_sendId || millis() - g_sendAt > 120000)
        sendFinished("", "The scan was cancelled or took too long.");
      return;
  }
}

void applySettings() {
  g_dev.setHasApiKey(!g_settings.apiKey.empty());
  g_dev.setNetInfo(g_settings.ssid,
                   g_settings.apiKey.empty() ? "not set"
                                             : "saved (..." + g_settings.apiKey.substr(g_settings.apiKey.size() - 4) + ")",
                   "Change: USB, wifi / key");
  connectWifi();
}

// Exam mode: radios off while it's on, and remembered across restarts.
void serviceExam() {
  const bool exam = g_dev.examActive();
  if (exam != g_wasExam) {
    g_wasExam = exam;
    examSave(exam, g_dev.examElapsedMs());
    g_examSavedAt = millis();
    Serial.println(exam ? "Exam mode ON: Wi-Fi and camera off." : "Exam mode OFF.");
    if (exam) {
      previewStop();
      g_send = SendStep::Idle;
    }
    connectWifi();
  } else if (exam && millis() - g_examSavedAt > 60000) {
    examSave(true, g_dev.examElapsedMs());
    g_examSavedAt = millis();
  }
}

// ---- battery refresh and sleep ----
uint32_t g_batteryAt = 0;

void serviceBattery() {
  if (g_batteryAt && millis() - g_batteryAt < 30000) return;
  g_batteryAt = millis() | 1;
  g_dev.setBattery(batteryPercent(), charging());
}

// Off (SHIFT AC, or 10 minutes idle): Wi-Fi and camera off, then light sleep
// until a key is pressed or USB is plugged in. The e-paper keeps its picture
// with no power. With USB plugged in it stays awake (radios still off), so
// the Serial Monitor keeps working for setup and "exam off".
// ponytail: light sleep keeps RAM, so memory and settings survive for free
// (~0.3 mA: about 3 weeks on 150 mAh). Deep sleep (months) would need the
// calculator's state saved to RTC memory first.
bool g_wasOff = false;

void servicePower() {
  const bool off = g_dev.isOff() && g_dev.activeRequest() == 0 && !previewActive() && g_send == SendStep::Idle;
  if (!off) {
    if (g_wasOff) {  // just switched on: back onto the hotspot
      g_wasOff = false;
      connectWifi();
    }
    return;
  }
  if (!g_wasOff) {
    g_wasOff = true;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    cameraSleep();
  }
  if (!usbPlugged()) {
    keysSleepUntilPress();
    g_batteryAt = 0;  // fresh reading after a sleep
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nAI Calculator prototype. Type help for setup commands.");
  // Why the chip restarted last time: tells a crash from a power dip.
  switch (esp_reset_reason()) {
    case ESP_RST_BROWNOUT:
      Serial.println("Last restart: POWER DIP (brownout). The USB port or cable couldn't supply enough "
                     "current; try another port or a shorter cable.");
      break;
    case ESP_RST_PANIC: Serial.println("Last restart: software crash (see the lines above it)."); break;
    case ESP_RST_TASK_WDT:
    case ESP_RST_INT_WDT:
    case ESP_RST_WDT: Serial.println("Last restart: watchdog (something froze)."); break;
    default: break;
  }
  keysBegin();
  pinMode(PIN_VBUS, INPUT);
  pinMode(PIN_CHG_STAT, INPUT_PULLDOWN);

  screenBegin();
  g_cameraOk = cameraBegin();
  Serial.printf("Camera: %s\n", cameraName());

  WiFi.onEvent(onWifiEvent);
  g_settings = settingsLoad();
  g_dev.setNoKeyHelp("Connect USB, open the Serial Monitor and type key.");
  g_dev.setRandomSource([] { return esp_random() / 4294967296.0; });
  g_dev.tick(millis());
  uint32_t examMs;
  if (examLoad(examMs)) g_dev.restoreExam(examMs);
  g_wasExam = g_dev.examActive();
  g_examSavedAt = millis();
  applySettings();

  g_requests = xQueueCreate(4, sizeof(int));
  g_events = xQueueCreate(16, sizeof(Event*));
  // 16 KB stack: TLS handshakes need the room. Core 0, away from the loop.
  xTaskCreatePinnedToCore(solveTask, "solve", 16384, nullptr, 1, nullptr, 0);

  Serial.println(statusText().c_str());
}

void loop() {
  std::string arg;
  switch (settingsPoll(g_settings, statusText, arg)) {
    case SerialCmd::Changed: applySettings(); break;
    case SerialCmd::Keys: typeKeys(arg); break;
    case SerialCmd::ExamOff:
      if (g_dev.examActive()) g_dev.usbUnlock();
      else Serial.println("Exam mode is not on.");
      break;
    case SerialCmd::Snap: {
      if (g_dev.activeRequest() != 0) {
        Serial.println("Busy with a photo; try again in a moment.");
        break;
      }
      std::string jpeg, detail, err;
      uint32_t t0 = millis();
      if (cameraCapture(jpeg, detail, err))
        Serial.printf("Test photo: %u bytes%s in %lu ms\n", (unsigned)jpeg.size(),
                      detail.empty() ? "" : (" + close-up " + std::to_string(detail.size())).c_str(), millis() - t0);
      else
        Serial.println(err.c_str());
      break;
    }
    case SerialCmd::Scan:
      if (!g_dev.radiosAllowed()) Serial.println("Wi-Fi is off in exam mode.");
      else scanWifi();
      break;
    case SerialCmd::Preview: {
      if (!g_dev.radiosAllowed()) {
        Serial.println("Preview is off in exam mode.");
        break;
      }
      std::string msg;
      previewStart(msg);
      Serial.println(msg.c_str());
      break;
    }
    case SerialCmd::PreviewOff:
      if (previewRunning()) previewStop();
      else Serial.println("Preview is not on.");
      break;
    case SerialCmd::None: break;
  }
  reportWifi();
  previewService();
  serviceSend();

  keysPoll(press);
  Event* e;
  while (xQueueReceive(g_events, &e, 0) == pdTRUE) handleEvent(e);

  serviceExam();
  g_dev.setOnline(WiFi.status() == WL_CONNECTED);
  g_dev.setHoldUntil(cameraHoldUntil());  // "HOLD STILL 3 s" countdown while the camera waits
  g_dev.tick(millis());
  g_dev.render(g_fb);
  screenShow(g_fb, g_dev.dotMatrix());  // no-op unless the picture changed
  serviceBattery();
  servicePower();
  delay(10);
}
