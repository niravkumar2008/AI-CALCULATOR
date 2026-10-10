// Firmware for the custom board inside the Casio (stage 14): the full calculator
// (Device: COMP maths, MODE / SETUP menus, AI SOLVE, exam mode), the same code as
// the simulators and the tester, with the OV5640 autofocus camera, e-paper screen,
// the whole keypad (keys.cpp), Wi-Fi through the AI proxy, the 150 mAh battery and
// deep sleep while off (power.cpp). Starts in COMP like the Casio.
//
// The calculator works fully offline: Wi-Fi is only switched on in AI SOLVE (and for
// a few serial commands and firmware updates). It never holds a Claude API key: AI
// requests go to our proxy (server/proxy) with this calculator's device token.
//
// Setup without a PC: = on SETUP > 6 starts a phone setup network (setup_portal.h).
// Every key can also be pressed from the Serial Monitor: "keys 1/3=" (see README).
// "exam off" over USB is the teacher's unlock. "selftest" runs the factory test.
// Serial log lines start with "[module]"; "SELFTEST_JSON {...}" is the test result.
#include <Arduino.h>
#include <WiFi.h>

#include "camera.h"
#include "claude_client.h"
#include "device.h"
#include "json.h"
#include "keys.h"
#include "log.h"
#include "ota.h"
#include "pins.h"
#include "power.h"
#include "preview.h"
#include "screen.h"
#include "selftest.h"
#include "settings.h"
#include "setup_portal.h"
#include "viewfinder.h"

using namespace calc;

// Headroom for JSON, TLS and the calculator's recursive parser (default is 8 KB).
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

namespace {

// ---- timing, in one place
// While off, the chip stays awake this long after the last key before deep sleep
// (so SHIFT, 7, ON and SHIFT + ALPHA + ON can be typed after a wake).
constexpr uint32_t kOffGraceMs = 4000;
constexpr uint32_t kBootSerialDelayMs = 300;       // let a Serial Monitor catch the first lines
constexpr uint32_t kSelfTestComboScanMs = 150;     // held-key scan at start for SHIFT + ALPHA + ON
constexpr uint32_t kBatteryRefreshMs = 30000;      // status-bar battery and the AI lock-out
constexpr uint32_t kExamSaveEveryMs = 60000;       // exam-mode elapsed time to NVS
constexpr uint32_t kSolveWifiWaitMs = 12000;       // after the photo: a slow hotspot's last chance
constexpr uint32_t kSerialWifiWaitMs = 15000;      // pair / account / update: time to join
constexpr uint32_t kSerialWifiHoldMs = 30000;      // ...and how long Wi-Fi stays on for them
constexpr uint32_t kSendWifiWaitMs = 25000;        // preview Send: hotspot after leaving the AP
// Preview Send: Claude's answer. An idle limit, not a total one: a long xhigh/max solve keeps
// streaming (and the proxy sends ": ping" every 15 s while Claude thinks), so it may take minutes.
// The idle limit is above the client's own kIdleMs (120 s), which normally reports first.
constexpr uint32_t kSendSolveIdleMs = 150000;      // no byte to or from the proxy this long
constexpr uint32_t kSendSolveMaxMs = 300000;       // ...and a cap even while it still streams
constexpr uint32_t kViewfinderLogEveryMs = 10000;  // fps / battery line while the viewfinder runs
constexpr uint32_t kSetupAfterSaveMs = 3000;       // phone setup: the "Saved" page is sent, then the network closes
constexpr uint32_t kOtaConfirmAfterMs = 15000;     // a freshly installed image confirms itself after this
constexpr uint32_t kOtaAutoDelayMs = 20000;        // off on a cable this long before checking for updates
constexpr uint32_t kOtaRecheckMs = 24u * 60 * 60 * 1000;  // ...and again once a day if it stays on the cable
constexpr uint32_t kOtaRestartDelayMs = 1500;      // "Update done" on the glass before the restart
constexpr uint32_t kEventPostWaitMs = 1000;        // solve task -> loop queue

Device g_dev;
Framebuffer g_fb;
bool g_wasExam = false;
uint32_t g_examSavedAt = 0;
Settings g_settings;
SemaphoreHandle_t g_settingsLock;  // the solve task copies the settings
uint32_t g_lastKeyAt = 0;

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
  if (xQueueSend(g_events, &e, pdMS_TO_TICKS(kEventPostWaitMs)) != pdTRUE) delete e;
}

Settings settingsCopy() {
  xSemaphoreTake(g_settingsLock, portMAX_DELAY);
  Settings s = g_settings;
  xSemaphoreGive(g_settingsLock);
  return s;
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
    // Wi-Fi was switched on when AI SOLVE opened and joins while the photo is taken;
    // give a slow hotspot a few more seconds before giving up.
    for (uint32_t w = millis(); WiFi.status() != WL_CONNECTED && millis() - w < kSolveWifiWaitMs && g_wantedId == id;)
      delay(50);
    LOGF("solve", "[%d] photo %u bytes%s in %lu ms", id, (unsigned)jpeg.size(),
         detail.empty() ? "" : (" + close-up " + std::to_string(detail.size())).c_str(), millis() - t0);
    post(new Event{Event::Captured, id, ""});
    SolveCallbacks cb;
    cb.cancelled = [id] { return g_wantedId != id; };
    cb.partial = [id, t0](double conf, const std::string& a) {
      LOGF("solve", "[%d] answer after %lu ms", id, millis() - t0);
      post(new Event{Event::Partial, id, a, conf});
    };
    cb.reply = [id, t0](const std::string& json) {
      LOGF("solve", "[%d] done after %lu ms", id, millis() - t0);
      post(new Event{Event::Reply, id, json});
    };
    cb.fail = [id](Failure f, const std::string& d) {
      LOGF("solve", "[%d] failed: %d %s", id, (int)f, d.c_str());
      post(new Event{Event::Fail, id, d, 0, f});
    };
    // Effort and tutor are read from this task: safe because they only change on
    // the Ready screen, never while a request is in flight.
    claudeSolve(settingsCopy(), jpeg, detail, g_dev.ai().effortParam(), g_dev.ai().tutor(), cb);
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
  if (previewResume()) LOGF("preview", "back: the page shows the answer");
}

const char* failureText(Failure f) {
  switch (f) {
    case Failure::NoConnection: return "No connection: check the phone's hotspot.";
    case Failure::NoApiKey: return "Not set up: use = on SETUP 6, or proxy and token in the Serial Monitor.";
    case Failure::Timeout: return "Claude took too long to reply.";
    case Failure::ApiBusy: return "Claude is busy: try again in a moment.";
    case Failure::ApiError: return "Claude refused the request.";
    case Failure::BadReply: return "Claude's reply couldn't be read.";
    case Failure::Camera: return "Couldn't take the photo.";
    case Failure::Account: return "The AI account refused it (link / subscription / monthly cap).";
    case Failure::LowBattery: return "Battery too low for AI: charge it.";
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
    else if (e->kind == Event::Fail) sendFinished("", e->text.empty() ? failureText(e->failure) : e->text);
  }
  delete e;
}

// ---- battery: the AI lock-out (review S2) ----
// Camera + Wi-Fi together on a nearly flat cell can brown the board out, so below
// kAiMinMillivolts (on battery) the viewfinder, the radio and = are all held off,
// and the AI SOLVE screen says so. Hysteresis: back on above kAiResumeMillivolts.
bool g_aiBatteryOk = true;
uint32_t g_batteryAt = 0;

void serviceBattery(bool force = false) {
  if (!force && g_batteryAt && millis() - g_batteryAt < kBatteryRefreshMs) return;
  g_batteryAt = millis() | 1;
  const int mv = batteryMillivolts();
  const Charge c = chargeState();
  const bool was = g_aiBatteryOk;
  if (usbPresent()) g_aiBatteryOk = true;
  else if (mv < kAiMinMillivolts) g_aiBatteryOk = false;
  else if (mv >= kAiResumeMillivolts) g_aiBatteryOk = true;
  if (was != g_aiBatteryOk)
    LOGF("power", "battery %d mV: AI, camera and Wi-Fi %s", mv, g_aiBatteryOk ? "allowed again" : "off until charged");
  g_dev.setLowBattery(!g_aiBatteryOk);
  g_dev.setBattery(lipoPercent(mv), c == Charge::Charging);
}

// Queues a new AI request, unless the battery can't take the Wi-Fi bursts.
void queueRequest(int id) {
  if (!g_aiBatteryOk || !batteryOkForAi()) {
    g_dev.onFailure(id, Failure::LowBattery);
    return;
  }
  g_wantedId = id;
  xQueueSend(g_requests, &id, 0);
}

// ---- camera viewfinder (AI SOLVE home screen) ----
Viewfinder g_vf;
Panel g_panel;
bool g_vfPaused = false;     // timed out, or no camera: the normal AI SOLVE screen shows
bool g_wasAiReady = false;   // for noticing "just arrived on the AI SOLVE home screen"
uint32_t g_vfLogAt = 0;
int g_vfLogFrames = 0, g_vfLogMv = 0;

// Frames come from their own task, so the next frame is taken and decoded while the
// e-paper is still busy with the last one (~2.5 fps instead of ~2), and the camera's
// 1 s power-up never blocks the keypad.
SemaphoreHandle_t g_grabLock;   // held by the task for a whole grab; taken to stop it
SemaphoreHandle_t g_frameLock;  // guards the hand-over below
bool g_grabOn = false;          // written only while holding g_grabLock
volatile bool g_grabFailed = false;
std::vector<uint8_t> g_frame;
int g_frameW = 0, g_frameH = 0;
bool g_frameFocused = false;
uint32_t g_frameSeq = 0, g_frameUsed = 0;

void grabTask(void*) {
  std::vector<uint8_t> buf;
  while (true) {
    xSemaphoreTake(g_grabLock, portMAX_DELAY);
    if (!g_grabOn) {
      xSemaphoreGive(g_grabLock);
      vTaskDelay(pdMS_TO_TICKS(30));
      continue;
    }
    int w = 0, h = 0;
    bool focused = false;
    const bool ok = cameraPreviewFrame(buf, w, h, focused);
    if (ok) {
      xSemaphoreTake(g_frameLock, portMAX_DELAY);
      g_frame.swap(buf);
      g_frameW = w;
      g_frameH = h;
      g_frameFocused = focused;
      ++g_frameSeq;
      xSemaphoreGive(g_frameLock);
    } else {
      g_grabOn = false;
      g_grabFailed = true;
    }
    xSemaphoreGive(g_grabLock);
    vTaskDelay(1);
  }
}

void setGrabbing(bool on) {
  xSemaphoreTake(g_grabLock, portMAX_DELAY);  // waits for a grab in progress to finish
  g_grabOn = on;
  if (on) g_grabFailed = false;
  xSemaphoreGive(g_grabLock);
}

bool onAiReady() {
  return !g_dev.isOff() && g_dev.mode() == Mode::Ai && g_dev.view() == View::Ai && g_dev.ai().screen() == Screen::Ready;
}

// ---- screens drawn by this file (not by Device): setup mode, updates ----
void showLines(const std::vector<std::string>& lines) {
  g_fb.clear();
  for (size_t i = 0; i < lines.size() && i < size_t(Framebuffer::kRows); ++i)
    g_fb.drawText(0, int(i), lines[i], i == 0);
  screenShow(g_fb, false);
}

// ---- phone setup mode (review S3) ----
SetupInfo g_setupInfo;
bool g_setupSaved = false;  // the page saved: close the network once the "Saved" page is out

void setupBegin(const char* how);  // below (needs wifiOff)

void setupEnd() {
  if (!setupActive()) return;
  setupStop();
  g_setupSaved = false;
}

void drawSetupScreen() {
  // Whole minutes: the e-paper is redrawn only when the text changes.
  const uint32_t left = setupSecondsLeft();
  char timer[24];
  snprintf(timer, sizeof timer, "%lu min left", (unsigned long)((left + 59) / 60));
  if (g_setupSaved) {
    showLines({"SETUP BY PHONE", "Saved. Closing the", "setup network...", "", "Next: MODE 4, then =", "", "AC:back"});
    return;
  }
  showLines({"SETUP BY PHONE",
             "1.Join Wi-Fi " + g_setupInfo.apName,
             "  password " + g_setupInfo.apPass,
             "2.Open the page it shows",
             "  or go to 192.168.4.1",
             "AC:back",
             timer});
}

// ---- keys ----
void press(DKey k) {
  g_lastKeyAt = millis();
  if (setupActive()) {  // the keypad only leaves setup mode (AC or ON)
    if (k == DKey::AC || k == DKey::On) {
      LOGF("setup", "ended with %s", dkeyName(k));
      setupEnd();
    }
    if (k != DKey::On) return;
  }
  if (g_vf.running()) g_vf.touch(millis());  // any key keeps the viewfinder on
  if (k == DKey::Right && g_vfPaused && onAiReady()) {  // ▶ on the AI SOLVE screen: viewfinder again
    g_vfPaused = false;
    return;
  }
  // = on SETUP > 6:Wi-Fi & key: set up from a phone (no PC needed).
  if (k == DKey::Eq && g_dev.view() == View::NetInfo && !g_dev.isOff() && g_dev.radiosAllowed()) {
    setupBegin("= on SETUP 6");
    return;
  }
  // SHIFT + ALPHA held while ON is pressed: the factory self-test.
  if (k == DKey::On && selfTestComboHeld()) selfTestRun("SHIFT+ALPHA+ON");
  LOGF("keys", "%s", dkeyName(k));  // lets you test every button without a multimeter
  g_dev.tick(millis());
  g_dev.onKey(k);
  g_wantedId = g_dev.activeRequest();  // 0 after AC / MODE / OFF: the solve task stops
  if (int id = g_dev.takeRequest()) queueRequest(id);
}

void typeKeys(const std::string& text) {
  for (char c : text) {
    if (c == ' ') continue;
    DKey ks[2];
    int n = 0;
    if (!keysForChar(c, ks, n)) {
      LOGF("keys", "no key for '%c' (type help, or see the README key list)", c);
      return;
    }
    for (int i = 0; i < n; ++i) press(ks[i]);
  }
}

// ---- Wi-Fi: only while it's needed ----
// Why the last join attempt failed, in words (0 = none yet).
volatile uint8_t g_wifiReason = 0;
volatile bool g_wifiReasonNew = false;
bool g_wifiOn = false;
uint32_t g_wifiNeededUntil = 0;  // serial commands (pair, account, update) keep it on briefly

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
    LOGF("wifi", "connected: %s", WiFi.localIP().toString().c_str());
  }
}

// Reports a new failure reason once, not on every retry.
void reportWifi() {
  if (!g_wifiReasonNew || !g_wifiOn) return;
  g_wifiReasonNew = false;
  const uint8_t r = g_wifiReason;
  const char* t = wifiReasonText(r);
  if (t) LOGF("wifi", "%s (code %u)", t, r);
  else LOGF("wifi", "not connected (code %u)", r);
}

void scanWifi() {
  LOGF("wifi", "scanning (a few seconds)...");
  if (!g_wifiOn) {
    WiFi.mode(WIFI_STA);
    powerLimitWifi();
  }
  const int n = WiFi.scanNetworks();
  if (n <= 0) {
    LOGF("wifi", "no networks found. The chip only sees 2.4 GHz: on iPhone turn on Maximize Compatibility");
  } else {
    bool found = false;
    for (int i = 0; i < n; ++i) {
      const String name = WiFi.SSID(i);
      const bool mine = !g_settings.ssid.empty() && name == g_settings.ssid.c_str();
      found |= mine;
      Serial.printf("  %s\"%s\"  signal %d dBm%s\n", mine ? "* " : "  ", name.c_str(), WiFi.RSSI(i),
                    mine ? "  <- your saved hotspot" : "");
    }
    if (!g_settings.ssid.empty() && !found)
      LOGF("wifi", "your saved hotspot \"%s\" is not in the list. Names must match exactly (capitals, apostrophes)",
           g_settings.ssid.c_str());
  }
  WiFi.scanDelete();
  if (!g_wifiOn) WiFi.mode(WIFI_OFF);
}

void wifiOff() {
  if (!g_wifiOn) return;
  g_wifiOn = false;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void wifiOn() {
  if (g_wifiOn) return;
  if (previewRunning()) previewStop();  // it shares the radio (a Send's pause is handled by its own step)
  if (setupActive()) setupEnd();
  g_wifiOn = true;
  WiFi.mode(WIFI_STA);
  powerLimitWifi();  // 11 dBm: plenty for a phone in the same room, small bursts for the cell
  WiFi.setAutoReconnect(true);
  g_wifiReason = 0;
  g_wifiReasonNew = false;
  WiFi.begin(g_settings.ssid.c_str(), g_settings.password.c_str());
}

// Wi-Fi is on only while the AI can use it: on, AI SOLVE (or a Send / serial command),
// not in exam mode, battery not nearly flat, hotspot set up. COMP maths never needs it.
void serviceWifi() {
  const bool aiScreen = !g_dev.isOff() && g_dev.mode() == Mode::Ai;
  const bool want = !g_settings.ssid.empty() && g_dev.radiosAllowed() && g_aiBatteryOk && !previewRunning() &&
                    !setupActive() &&
                    (aiScreen || g_dev.activeRequest() != 0 || g_send == SendStep::WaitWifi ||
                     g_send == SendStep::Solving || int32_t(g_wifiNeededUntil - millis()) > 0);
  if (want) wifiOn();
  else if (!previewActive() && !setupActive()) wifiOff();
}

// For serial commands and updates: Wi-Fi on now, waits for the hotspot.
bool wifiFor(uint32_t holdMs) {
  if (!g_dev.radiosAllowed()) {
    LOGF("wifi", "off in exam mode");
    return false;
  }
  if (g_settings.ssid.empty()) {
    LOGF("wifi", "no hotspot set: type wifi first (or = on SETUP 6)");
    return false;
  }
  g_wifiNeededUntil = millis() + holdMs;
  wifiOn();
  const uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < kSerialWifiWaitMs) delay(50);
  if (WiFi.status() != WL_CONNECTED) LOGF("wifi", "couldn't join the hotspot");
  return WiFi.status() == WL_CONNECTED;
}

void setupBegin(const char* how) {
  if (setupActive()) return;
  if (!g_dev.radiosAllowed()) {
    LOGF("setup", "not in exam mode");
    return;
  }
  wifiOff();
  if (previewRunning()) previewStop();
  g_send = SendStep::Idle;
  std::string err;
  if (!setupStart(g_settings, g_setupInfo, err)) {
    LOGF("setup", "%s", err.c_str());
    return;
  }
  g_setupSaved = false;
  LOGF("setup", "started (%s). On a phone: join %s (password %s), then open %s", how, g_setupInfo.apName.c_str(),
       g_setupInfo.apPass.c_str(), g_setupInfo.url.c_str());
}

}  // namespace

void previewEnded() { g_wifiOn = false; }  // serviceWifi() rejoins the hotspot if AI SOLVE needs it

namespace {

std::string chargeText() {
  switch (chargeState()) {
    case Charge::Charging: return " (charging)";
    case Charge::Full: return " (USB, full)";
    default: return "";
  }
}

std::string statusText() {
  std::string s = "Firmware: " + std::string(kFirmwareVersion) + " (slot " + otaSlotName() +
                  (otaPendingVerify() ? ", not yet confirmed" : "") + ")  id " + deviceId();
  s += "\nCamera: " + std::string(cameraName());
  s += "\nWi-Fi: " + (g_settings.ssid.empty() ? std::string("not set (type wifi, or = on SETUP 6)")
                                              : "\"" + g_settings.ssid + "\" " +
                                                    (WiFi.status() == WL_CONNECTED
                                                         ? "connected, " + std::string(WiFi.localIP().toString().c_str())
                                                         : std::string(g_wifiOn ? "joining" : "off (on in AI SOLVE)") +
                                                               (wifiReasonText(g_wifiReason)
                                                                    ? std::string(": ") + wifiReasonText(g_wifiReason)
                                                                    : std::string())));
  s += "\nAI proxy: " + (g_settings.proxyUrl.empty() ? std::string("not set (type proxy)") : g_settings.proxyUrl);
  s += "\nDevice token: " + g_settings.tokenHint() + (g_settings.deviceToken.empty() ? " (type token)" : "");
  const int mv = batteryMillivolts();
  s += "\nBattery: " + std::to_string(lipoPercent(mv)) + "% (" + std::to_string(mv) + " mV)" + chargeText() +
       (g_aiBatteryOk ? "" : "  AI, camera and Wi-Fi off until charged");
  s += "\nFree PSRAM: " + std::to_string(ESP.getFreePsram() / 1024) + " KB";
  if (setupActive()) s += "\nSetup mode: network " + g_setupInfo.apName + ", password " + g_setupInfo.apPass;
  return s;
}

const char* effortName() {
  switch (g_dev.ai().effort()) {
    case Effort::Careful: return "Careful";
    case Effort::Max: return "Max";
    default: return "Normal";
  }
}

void stopViewfinder(bool cameraOff) {
  if (!g_vf.running()) return;
  setGrabbing(false);  // after this no grab can power the camera up again
  g_vf.stop();
  screenEndPanel();
  if (cameraOff) cameraSleep();
  LOGF("cam", "viewfinder off after %d frames (%.1f fps)", g_vf.frames(), g_vf.fps(millis()));
}

// The viewfinder runs while the AI SOLVE home screen is up: frame -> dither ->
// partial refresh, about 2-3 frames a second (the e-paper's ~0.3 s partial refresh
// is the limit). = takes the full-resolution photo as before (the solve task);
// AC leaves AI SOLVE; 30 s without a key turns the camera off (▶ turns it on again).
// Not in exam mode and not on a nearly flat battery (the AI SOLVE screen explains).
// Returns true while it owns the screen.
bool serviceViewfinder() {
  const uint32_t now = millis();
  const bool ready = onAiReady();
  if (ready && !g_wasAiReady) g_vfPaused = false;  // every visit starts with the viewfinder
  g_wasAiReady = ready;
  const bool want = ready && g_dev.radiosAllowed() && g_aiBatteryOk && !g_vfPaused && !previewActive() &&
                    !setupActive() && g_send == SendStep::Idle;
  if (!want) {
    // Leaving for a scan (= pressed): keep the camera on, the solve task takes the photo.
    stopViewfinder(g_dev.activeRequest() == 0);
    return false;
  }
  if (!g_vf.running()) {
    g_vf.start(now);
    g_vf.renderStarting(g_panel);
    screenShowPanel(g_panel, true);
    g_vf.takeFullRefresh(g_panel);
    g_vfLogAt = now;
    g_vfLogFrames = 0;
    g_vfLogMv = batteryMillivolts();
    g_frameUsed = g_frameSeq;
    setGrabbing(true);
    LOGF("cam", "viewfinder on: = takes the photo, AC leaves, 30 s idle turns it off");
  }
  if (g_vf.timedOut(now)) {
    stopViewfinder(true);
    g_vfPaused = true;
    LOGF("cam", "viewfinder off (30 s without a key) to save the battery. Press the right arrow to see it again");
    return false;
  }
  if (g_grabFailed) {
    stopViewfinder(true);
    g_vfPaused = true;
    LOGF("cam", "viewfinder: no picture from the camera (%s)", cameraName());
    return false;
  }
  xSemaphoreTake(g_frameLock, portMAX_DELAY);
  const bool fresh = g_frameSeq != g_frameUsed && !g_frame.empty();
  if (fresh) {
    g_frameUsed = g_frameSeq;
    g_vf.feed(g_frame.data(), g_frameW, g_frameH, g_frameFocused, now);
  }
  xSemaphoreGive(g_frameLock);
  if (!fresh) return true;  // the picture on the glass is still the newest
  g_vf.render(g_panel, now, std::string(effortName()) + (g_dev.ai().tutor() ? " +Tutor" : ""));
  screenShowPanel(g_panel, g_vf.takeFullRefresh(g_panel));
  if (now - g_vfLogAt >= kViewfinderLogEveryMs) {  // battery cost: frames and battery drop every 10 s
    const int mv = batteryMillivolts();
    LOGF("cam", "viewfinder %.1f fps, sharpness %.0f, battery %d mV (%+d mV in %lu s)",
         (g_vf.frames() - g_vfLogFrames) * 1000.0 / (now - g_vfLogAt), g_vf.info().sharpness, mv, mv - g_vfLogMv,
         (unsigned long)((now - g_vfLogAt) / 1000));
    g_vfLogAt = now;
    g_vfLogFrames = g_vf.frames();
  }
  return true;
}

// Steps the Send through: photo -> hotspot -> Claude -> back to the page.
void serviceSend() {
  switch (g_send) {
    case SendStep::Idle: {
      if (!previewTakeSend()) return;
      if (g_dev.examActive()) return previewSetResult("", "Exam mode is on.");
      if (!g_settings.aiReady()) return previewSetResult("", "Not set up: type proxy and token in the Serial Monitor.");
      if (g_settings.ssid.empty()) return previewSetResult("", "No hotspot set: type wifi in the Serial Monitor.");
      if (!g_aiBatteryOk) return previewSetResult("", "Battery too low for AI: plug the cable in.");
      if (g_dev.isOff()) press(DKey::On);
      std::string err;
      g_preDetail.clear();
      if (!cameraCapture(g_prePhoto, g_preDetail, err)) {
        g_prePhoto.clear();
        return previewSetResult("", err);
      }
      LOGF("preview", "send: photo taken; leaving the preview network to reach Claude...");
      previewSuspend();
      g_wifiOn = false;
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
          g_send = SendStep::Solving;
          g_sendAt = millis();
          queueRequest(g_sendId);
        } else {
          g_prePhoto.clear();
          sendFinished("", "The calculator couldn't start a scan.");
        }
      } else if (millis() - g_sendAt > kSendWifiWaitMs) {
        g_prePhoto.clear();
        sendFinished("", "Couldn't reach the hotspot: is it on, with the phone nearby?");
      }
      return;
    case SendStep::Solving:
      // The reply or failure arrives through handleEvent. A user who pressed
      // AC on the calculator cancelled it: bring the page back anyway.
      {
        const uint32_t now = millis(), act = claudeLastActivityMs();
        // Bytes of this request only: activity from before the Send started doesn't count.
        const uint32_t quietSince = int32_t(act - g_sendAt) > 0 ? act : g_sendAt;
        if (g_dev.activeRequest() != g_sendId || now - quietSince > kSendSolveIdleMs ||
            now - g_sendAt > kSendSolveMaxMs)
          sendFinished("", "The scan was cancelled or took too long.");
      }
      return;
  }
}

void applySettings() {
  g_dev.setHasApiKey(g_settings.aiReady());
  g_dev.setNetInfo(g_settings.ssid, g_settings.aiReady() ? "token " + g_settings.tokenHint() : "not set up",
                   "=: set up from a phone");
  wifiOff();  // serviceWifi() joins again with the new details when AI SOLVE needs it
}

void publishSettings(const Settings& s) {
  xSemaphoreTake(g_settingsLock, portMAX_DELAY);
  g_settings = s;
  xSemaphoreGive(g_settingsLock);
  applySettings();
}

// Phone setup: DNS + time limit, and the settings the page saved.
void serviceSetup() {
  if (!setupActive()) return;
  setupService();
  Settings fromPhone;
  if (setupTakeSaved(fromPhone)) {
    g_setupSaved = true;
    publishSettings(fromPhone);
    wifiOff();  // applySettings turned the station side off; the setup network stays up for the "Saved" page
  }
  if (g_setupSaved && setupSavedAgoMs() > kSetupAfterSaveMs) {
    setupEnd();
    LOGF("setup", "settings from the phone are in use");
  }
}

// ---- pairing and account (serial commands) ----
void printProxyJson(const char* what, int status, const std::string& body) {
  Json j;
  std::string err;
  if (status == 0) {
    Serial.printf("%s: %s\n", what, body.c_str());
  } else if (Json::parse(body, j, err) && j["error"]["message"].isString()) {
    Serial.printf("%s: %s (HTTP %d)\n", what, j["error"]["message"].asString().c_str(), status);
  } else if (status == 200 && Json::parse(body, j, err)) {
    if (j["code"].isString()) {
      Serial.printf("Pairing code: %s\nOn your phone open %s and enter it (valid %d minutes).\n",
                    j["code"].asString().c_str(), j["link_url"].isString() ? j["link_url"].asString().c_str() : "the link page",
                    j["expires_in"].isNumber() ? int(j["expires_in"].asNumber() / 60) : 10);
    } else {
      Serial.printf("%s: %s\n", what, body.c_str());
    }
  } else {
    Serial.printf("%s: HTTP %d %s\n", what, status, body.substr(0, 200).c_str());
  }
}

void pairCommand() {
  if (!g_settings.aiReady()) return (void)LOGF("proxy", "set proxy and token first");
  if (!wifiFor(kSerialWifiHoldMs)) return;
  std::string reply;
  const int st = proxyRequest(g_settings, "POST", "/v1/pair/start", "{}", reply);
  printProxyJson("Pairing", st, reply);
}

void accountCommand() {
  if (!g_settings.aiReady()) return (void)LOGF("proxy", "set proxy and token first");
  if (!wifiFor(kSerialWifiHoldMs)) return;
  std::string reply;
  const int st = proxyRequest(g_settings, "GET", "/v1/device/status", "", reply);
  printProxyJson("Account", st, reply);
}

// ---- firmware updates (review S1) ----
// Wi-Fi must be connected. Checks the proxy; installs and restarts when it has a different
// image. `screen`: say what is happening on the e-paper (the automatic check is silent
// until an update really starts). Returns true if an update was installed (restarting).
bool runUpdate(bool screen) {
  OtaInfo info;
  std::string err;
  if (!otaCheck(g_settings, info, err)) {
    LOGF("ota", "check failed: %s", err.c_str());
    if (screen) showLines({"UPDATE", "Couldn't check:", err.substr(0, 25), err.size() > 25 ? err.substr(25, 25) : "", "", "AC:back"});
    return false;
  }
  if (!info.available) {
    LOGF("ota", "up to date (%s)", kFirmwareVersion);
    if (screen) showLines({"UPDATE", "Already up to date:", kFirmwareVersion, "", "", "AC:back"});
    return false;
  }
  LOGF("ota", "update %s available (%u KB)", info.version.c_str(), unsigned(info.size / 1024));
  showLines({"UPDATING", "Keep the cable in.", "Downloading " + info.version, "0%", "", "Don't turn it off."});
  const bool ok = otaInstall(g_settings, info, [&](int percent) {
    showLines({"UPDATING", "Keep the cable in.", "Downloading " + info.version, std::to_string(percent) + "%", "",
               "Don't turn it off."});
  }, err);
  if (!ok) {
    LOGF("ota", "not installed: %s", err.c_str());
    showLines({"UPDATE FAILED", err.substr(0, 25), err.size() > 25 ? err.substr(25, 25) : "", "The old firmware", "keeps running.", "AC:back"});
    return false;
  }
  showLines({"UPDATE DONE", "Restarting with", info.version, "", "", ""});
  delay(kOtaRestartDelayMs);
  Serial.flush();
  ESP.restart();
  return true;
}

void updateCommand() {
  if (!g_settings.aiReady()) return (void)LOGF("ota", "set proxy and token first");
  if (!batteryOkForOta(kOtaMinBatteryMv)) {
    LOGF("ota", "plug the cable in first (or charge above %d mV)", kOtaMinBatteryMv);
    return;
  }
  if (g_dev.activeRequest() != 0) return (void)LOGF("ota", "busy with a scan; try again in a moment");
  if (!wifiFor(kSerialWifiHoldMs)) return;
  runUpdate(true);
  g_wifiNeededUntil = 0;
}

// Automatic: when the calculator is switched off on a cable, it checks for an update
// (once, then daily while it stays plugged in) and installs it. Off = nothing to interrupt.
uint32_t g_otaCheckedAt = 0;
bool g_otaChecked = false;

void serviceOta() {
  if (g_dev.activeRequest() != 0 || previewActive() || setupActive() || g_send != SendStep::Idle) return;
  if (!g_dev.isOff() || !usbPresent() || !g_dev.radiosAllowed() || !g_settings.aiReady() || g_settings.ssid.empty()) return;
  if (millis() - g_lastKeyAt < kOtaAutoDelayMs) return;
  if (g_otaChecked && millis() - g_otaCheckedAt < kOtaRecheckMs) return;
  g_otaChecked = true;
  g_otaCheckedAt = millis() | 1;
  LOGF("ota", "off on a cable: checking the AI server for new firmware");
  if (!wifiFor(kSerialWifiHoldMs)) return;
  runUpdate(false);
  g_wifiNeededUntil = 0;
  wifiOff();
  g_dev.tick(millis());
  g_dev.render(g_fb);
  screenShow(g_fb, g_dev.dotMatrix());  // back to the off screen (blank, or "Charging")
}

// A just-installed image confirms itself once the keypad scanner answers and the loop
// has run for a while; until then a crash would make the bootloader go back.
void serviceOtaConfirm() {
  static bool done = false;
  if (done) return;
  if (!otaPendingVerify()) {
    done = true;
    return;
  }
  if (millis() < kOtaConfirmAfterMs) return;
  if (!keysScannerOk()) {
    static uint32_t warnedAt = 0;
    if (millis() - warnedAt > 60000) {
      warnedAt = millis();
      LOGF("ota", "new firmware NOT confirmed: the keypad scanner doesn't answer (a restart goes back to the old one)");
    }
    return;
  }
  otaConfirm();
  done = true;
}

// Exam mode: radios off while it's on, and remembered across restarts.
void serviceExam() {
  const bool exam = g_dev.examActive();
  if (exam != g_wasExam) {
    g_wasExam = exam;
    examSave(exam, g_dev.examElapsedMs());
    g_examSavedAt = millis();
    LOGF("exam", "%s", exam ? "ON: Wi-Fi and camera off" : "OFF");
    if (exam) {
      previewStop();
      setupEnd();
      g_send = SendStep::Idle;
      wifiOff();
      cameraSleep();
    }
  } else if (exam && millis() - g_examSavedAt > kExamSaveEveryMs) {
    examSave(true, g_dev.examElapsedMs());
    g_examSavedAt = millis();
  }
}

// Off (SHIFT AC, or 10 minutes idle): Wi-Fi and camera off, then deep sleep a few
// seconds after the last key. The e-paper keeps its picture with no power; memories,
// history and settings go to RTC memory and come back at the next key. With USB
// plugged in it stays awake (radios off), so the Serial Monitor keeps working and
// the screen says it is charging.
bool g_wasOff = false;

void servicePower() {
  const bool off = g_dev.isOff() && g_dev.activeRequest() == 0 && !previewActive() && !setupActive() &&
                   g_send == SendStep::Idle;
  if (!off) {
    g_wasOff = false;
    return;
  }
  if (!g_wasOff) {
    g_wasOff = true;
    wifiOff();
    cameraSleep();
  }
  if (usbPresent() || millis() - g_lastKeyAt < kOffGraceMs) return;
  if (g_dev.examActive()) examSave(true, g_dev.examElapsedMs());
  const bool keypadClear = keysPrepareSleep();
  LOGF("power", "deep sleep (ON or any key wakes it)");
  Serial.flush();
  powerDeepSleep(g_dev.saveState(kRtcStateBytes), keypadClear);
}

void bootMessage(Wake wake) {
  if (wake != Wake::ColdBoot) return;  // a key wake is routine: stay quiet
  Serial.printf("\nAI Calculator %s (slot %s). Type help for setup commands. Last restart: %s.\n", kFirmwareVersion,
                otaSlotName(), resetReasonText());
  if (esp_reset_reason() == ESP_RST_BROWNOUT)
    LOGF("power", "that was a POWER DIP (brownout): the battery is low or the USB port couldn't supply enough current");
  if (otaPendingVerify()) LOGF("ota", "first run of a new firmware: it confirms itself in %lu s", (unsigned long)(kOtaConfirmAfterMs / 1000));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  powerBegin();
  const Wake wake = powerWakeCause();
  const bool woke = powerWokeFromSleep();
  keysBegin(woke);
  if (wake == Wake::OnKey) keysWokeByOn();
  screenBegin(woke);
  if (!woke) delay(kBootSerialDelayMs);
  bootMessage(wake);

  g_settingsLock = xSemaphoreCreateMutex();
  WiFi.onEvent(onWifiEvent);
  g_settings = settingsLoad();
  g_dev.setNoKeyHelp("Set up: = on SETUP 6 (phone), or USB: proxy, token.");
  g_dev.setRandomSource([] { return esp_random() / 4294967296.0; });
  g_dev.tick(millis());
  std::string state;
  uint32_t asleepMs = 0;
  if (powerTakeState(state, asleepMs) && g_dev.restoreState(state, asleepMs)) {
    // back from deep sleep: the calculator is where it was
  } else {
    uint32_t examMs;
    if (examLoad(examMs)) g_dev.restoreExam(examMs);
  }
  g_wasExam = g_dev.examActive();
  g_examSavedAt = millis();
  g_wasOff = g_dev.isOff();
  applySettings();
  serviceBattery(true);
  g_lastKeyAt = millis();

  g_requests = xQueueCreate(4, sizeof(int));
  g_events = xQueueCreate(16, sizeof(Event*));
  // 16 KB stack: TLS handshakes need the room. Core 0, away from the loop.
  xTaskCreatePinnedToCore(solveTask, "solve", 16384, nullptr, 1, nullptr, 0);
  g_grabLock = xSemaphoreCreateMutex();
  g_frameLock = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(grabTask, "viewfinder", 8192, nullptr, 1, nullptr, 0);

  if (!woke) {
    // Factory self-test: SHIFT + ALPHA + ON held while the board starts.
    const uint32_t t0 = millis();
    while (millis() - t0 < kSelfTestComboScanMs) {
      keysPoll([](DKey) {});  // the scanner reports keys already held as presses
      delay(10);
    }
    if (selfTestComboHeld() && keysHeld(DKey::On)) selfTestRun("SHIFT+ALPHA+ON at start");
    Serial.println(statusText().c_str());
  }
}

void loop() {
  std::string arg;
  Settings edited = g_settings;  // the solve task may be copying g_settings right now
  const SerialCmd cmd = settingsPoll(edited, statusText, arg);
  switch (cmd) {
    case SerialCmd::Changed: publishSettings(edited); break;
    case SerialCmd::Keys: typeKeys(arg); break;
    case SerialCmd::ExamOff:
      if (g_dev.examActive()) g_dev.usbUnlock();
      else LOGF("exam", "exam mode is not on");
      break;
    case SerialCmd::Snap: {
      if (g_dev.activeRequest() != 0) {
        LOGF("cam", "busy with a photo; try again in a moment");
        break;
      }
      std::string jpeg, detail, err;
      uint32_t t0 = millis();
      if (cameraCapture(jpeg, detail, err))
        LOGF("cam", "test photo: %u bytes%s in %lu ms", (unsigned)jpeg.size(),
             detail.empty() ? "" : (" + close-up " + std::to_string(detail.size())).c_str(), millis() - t0);
      else
        LOGF("cam", "%s", err.c_str());
      break;
    }
    case SerialCmd::Scan:
      if (!g_dev.radiosAllowed()) LOGF("wifi", "off in exam mode");
      else scanWifi();
      break;
    case SerialCmd::Preview: {
      if (!g_dev.radiosAllowed()) {
        LOGF("preview", "off in exam mode");
        break;
      }
      wifiOff();
      setupEnd();
      std::string msg;
      previewStart(msg);
      Serial.println(msg.c_str());
      break;
    }
    case SerialCmd::PreviewOff:
      if (previewRunning()) previewStop();
      else LOGF("preview", "not on");
      break;
    case SerialCmd::Pair: pairCommand(); break;
    case SerialCmd::Account: accountCommand(); break;
    case SerialCmd::Setup:
      if (g_dev.isOff()) press(DKey::On);
      setupBegin("serial");
      break;
    case SerialCmd::SetupOff:
      if (setupActive()) setupEnd();
      else LOGF("setup", "not on");
      break;
    case SerialCmd::Update: updateCommand(); break;
    case SerialCmd::SelfTest:
      wifiOff();
      setupEnd();
      selfTestRun("serial");
      break;  // never reached (selfTestRun restarts); keeps the switch readable
    case SerialCmd::None: break;
  }
  if (cmd != SerialCmd::None) g_lastKeyAt = millis();
  reportWifi();
  previewService();
  serviceSend();
  serviceSetup();

  keysPoll(press);
  Event* e;
  while (xQueueReceive(g_events, &e, 0) == pdTRUE) handleEvent(e);

  serviceExam();
  if (usbPresent() && !g_aiBatteryOk) serviceBattery(true);  // a cable lifts the lock-out at once
  serviceBattery();
  serviceWifi();
  // "Online" once Wi-Fi is on for AI SOLVE: = may be pressed while it is still joining
  // the hotspot (the photo takes longer than the join; the solve task waits for it).
  g_dev.setOnline(WiFi.status() == WL_CONNECTED || (g_wifiOn && !g_settings.ssid.empty()));
  g_dev.setHoldUntil(cameraHoldUntil());  // "HOLD STILL 3 s" countdown while the camera waits
  g_dev.tick(millis());
  if (setupActive()) {
    drawSetupScreen();
  } else if (!serviceViewfinder()) {
    g_dev.render(g_fb);
    screenShow(g_fb, g_dev.dotMatrix());  // no-op unless the picture changed
  }
  serviceOtaConfirm();
  serviceOta();
  servicePower();
  delay(10);
}
