// Stage 4 firmware for the breadboard tester: the full calculator (Device:
// COMP maths, MODE / SETUP menus, AI SOLVE, exam mode), the same code as the
// simulators, with the real camera, e-paper screen, buttons and Wi-Fi.
//
// The tester has 5 buttons: = , AC, up, down, and BOOT = ON / OFF. It starts
// in AI SOLVE. Every other key can be pressed from the Serial Monitor:
// "keys 1/3=" (see README). "exam off" over USB is the teacher's unlock.
#include <Arduino.h>
#include <WiFi.h>

#include "device.h"
#include "camera.h"
#include "claude_client.h"
#include "pins.h"
#include "screen.h"
#include "settings.h"

using namespace calc;

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

void post(Event* e) {
  if (xQueueSend(g_events, &e, pdMS_TO_TICKS(1000)) != pdTRUE) delete e;
}

void solveTask(void*) {
  int id;
  while (true) {
    if (xQueueReceive(g_requests, &id, portMAX_DELAY) != pdTRUE) continue;
    if (g_wantedId != id) continue;
    uint32_t t0 = millis();
    std::string jpeg, error;
    if (!cameraCapture(jpeg, error)) {
      post(new Event{Event::Fail, id, error, 0, Failure::Camera});
      continue;
    }
    Serial.printf("[%d] photo %u bytes in %lu ms\n", id, (unsigned)jpeg.size(), millis() - t0);
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
    claudeSolve(key, jpeg, cb);
    jpeg.clear();
    jpeg.shrink_to_fit();
  }
}

void handleEvent(Event* e) {
  switch (e->kind) {
    case Event::Captured: g_dev.onCaptured(e->id); break;
    case Event::Partial: g_dev.onPartialAnswer(e->id, e->confidence, e->text); break;
    case Event::Reply: g_dev.onReply(e->id, e->text); break;
    case Event::Fail: g_dev.onFailure(e->id, e->failure, e->text); break;
  }
  delete e;
}

// ---- keys ----
// Presses are caught by interrupts so none are missed while the e-paper is
// busy refreshing (a partial refresh blocks the loop for ~0.3 s).
struct Button {
  int pin;
  DKey key;
  volatile bool pressed;
  volatile uint32_t lastMs;
};
Button g_buttons[] = {{PIN_KEY_EQ, DKey::Eq, false, 0}, {PIN_KEY_AC, DKey::AC, false, 0},
                      {PIN_KEY_UP, DKey::Up, false, 0}, {PIN_KEY_DOWN, DKey::Down, false, 0},
                      {PIN_KEY_ON, DKey::On, false, 0}};

void IRAM_ATTR onFall(void* arg) {
  Button* b = static_cast<Button*>(arg);
  uint32_t now = millis();
  if (now - b->lastMs < 150) return;  // contact bounce
  b->lastMs = now;
  b->pressed = true;
}

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

// A physical button. The tester has no MODE key, so AC on an empty
// calculator screen goes back to AI SOLVE, and BOOT toggles power.
void pressButton(DKey k) {
  if (k == DKey::On && !g_dev.isOff()) {
    press(DKey::Shift);
    press(DKey::AC);  // SHIFT AC = OFF
    return;
  }
  if (k == DKey::AC && g_dev.view() == View::Calc && g_dev.expression().empty() && !g_dev.showingResult() &&
      !g_dev.examActive()) {
    press(DKey::Mode);
    press(DKey::D4);
    return;
  }
  press(k);
}

void pollButtons() {
  for (Button& b : g_buttons) {
    if (!b.pressed) continue;
    b.pressed = false;
    pressButton(b.key);
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
void connectWifi() {
  WiFi.disconnect(true);
  if (g_settings.ssid.empty() || !g_dev.radiosAllowed()) {
    WiFi.mode(WIFI_OFF);
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(g_settings.ssid.c_str(), g_settings.password.c_str());
}

std::string statusText() {
  std::string s = "Camera: " + std::string(cameraName());
  s += "\nWi-Fi: " + (g_settings.ssid.empty() ? std::string("not set (type wifi)")
                                              : "\"" + g_settings.ssid + "\" " +
                                                    (WiFi.status() == WL_CONNECTED
                                                         ? "connected, " + std::string(WiFi.localIP().toString().c_str())
                                                         : std::string("not connected")));
  s += "\nKey: " + (g_settings.apiKey.empty() ? std::string("not set (type key)")
                                              : "saved (ends ..." + g_settings.apiKey.substr(g_settings.apiKey.size() - 4) + ")");
  s += "\nFree PSRAM: " + std::to_string(ESP.getFreePsram() / 1024) + " KB";
  return s;
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
    connectWifi();
  } else if (exam && millis() - g_examSavedAt > 60000) {
    examSave(true, g_dev.examElapsedMs());
    g_examSavedAt = millis();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nAI Calculator tester (Stage 4). Type help for setup commands.");
  for (Button& b : g_buttons) {
    pinMode(b.pin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(b.pin), onFall, &b, FALLING);
  }

  screenBegin();
  g_cameraOk = cameraBegin();
  Serial.printf("Camera: %s\n", cameraName());

  g_settings = settingsLoad();
  g_dev.setNoKeyHelp("Connect USB, open the Serial Monitor and type key.");
  g_dev.setRandomSource([] { return esp_random() / 4294967296.0; });
  g_dev.tick(millis());
  uint32_t examMs;
  if (examLoad(examMs)) g_dev.restoreExam(examMs);
  g_wasExam = g_dev.examActive();
  g_examSavedAt = millis();
  if (!g_dev.examActive()) {  // the tester starts in AI SOLVE (it has no MODE key)
    g_dev.onKey(DKey::Mode);
    g_dev.onKey(DKey::D4);
  }
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
      std::string jpeg, err;
      uint32_t t0 = millis();
      if (cameraCapture(jpeg, err))
        Serial.printf("Test photo: %u bytes in %lu ms\n", (unsigned)jpeg.size(), millis() - t0);
      else
        Serial.println(err.c_str());
      break;
    }
    case SerialCmd::None: break;
  }

  pollButtons();
  Event* e;
  while (xQueueReceive(g_events, &e, 0) == pdTRUE) handleEvent(e);

  serviceExam();
  g_dev.setOnline(WiFi.status() == WL_CONNECTED);
  g_dev.tick(millis());
  g_dev.render(g_fb);
  screenShow(g_fb, g_dev.dotMatrix());  // no-op unless the picture changed
  delay(10);
}
