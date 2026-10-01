#include "camera.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_camera.h>
#include <esp_jpg_decode.h>
#include <esp_heap_caps.h>
#include <img_converters.h>

#include <vector>

#include "enhance.h"
#include "focus.h"

#include "pins.h"

#include <ESP32_OV5640_AF.h>

namespace {
std::string g_name = "none";
bool g_ok = false;
SemaphoreHandle_t g_lock;  // one user of the sensor at a time (scan vs preview)
std::string g_last;        // last scan photo, for the preview page
std::string g_lastDetail;  // its close-up of the writing, if one was made
volatile uint32_t g_holdUntil = 0;  // see cameraHoldUntil()
constexpr uint32_t kTryMs = 750;    // one careful-scan setting: settle + frame + check
constexpr uint32_t kFramesMs = 2000; // the 8 frames to choose from

// Every setting the preview page can change: name, min, max, default.
struct Tuning {
  const char* name;
  int min, max, def;
  int (*apply)(sensor_t*, int);
};
const Tuning kTuning[] = {
    {"framesize", FRAMESIZE_VGA, FRAMESIZE_UXGA, FRAMESIZE_UXGA,
     [](sensor_t* s, int v) { return s->set_framesize(s, (framesize_t)v); }},
    {"quality", 6, 30, 12, [](sensor_t* s, int v) { return s->set_quality(s, v); }},
    {"brightness", -2, 2, 1, [](sensor_t* s, int v) { return s->set_brightness(s, v); }},
    {"contrast", -2, 2, 0, [](sensor_t* s, int v) { return s->set_contrast(s, v); }},
    {"sharpness", -2, 2, 0, [](sensor_t* s, int v) { return s->set_sharpness(s, v); }},
    {"ae_level", -2, 2, 1, [](sensor_t* s, int v) { return s->set_ae_level(s, v); }},
    {"gainceiling", 0, 6, 2, [](sensor_t* s, int v) { return s->set_gainceiling(s, (gainceiling_t)v); }},
    {"denoise", 0, 8, 0, [](sensor_t* s, int v) { return s->set_denoise(s, v); }},
    {"grayscale", 0, 1, 1, [](sensor_t* s, int v) { return s->set_special_effect(s, v ? 2 : 0); }},
    {"hmirror", 0, 1, 0, [](sensor_t* s, int v) { return s->set_hmirror(s, v); }},
    {"vflip", 0, 1, 0, [](sensor_t* s, int v) { return s->set_vflip(s, v); }},
    // Not a sensor setting: how long a scan waits (tenths of a second) for the
    // shake of the key press to die down before taking its frames.
    {"settle", 2, 30, 15, [](sensor_t*, int) { return 0; }},
    // Not a sensor setting either: 1 = careful scan (tune for the light first).
    {"careful", 0, 1, 1, [](sensor_t*, int) { return 0; }},
};
constexpr int kTuningCount = sizeof(kTuning) / sizeof(kTuning[0]);
int g_value[kTuningCount];

int tuning(const char* name) {
  for (int i = 0; i < kTuningCount; ++i)
    if (!strcmp(kTuning[i].name, name)) return g_value[i];
  return 0;
}

// The live view runs at VGA: about 4x fewer pixels than UXGA, so frames are
// small and quick to send. Scans switch back to the "framesize" setting.
constexpr framesize_t kLiveSize = FRAMESIZE_VGA;
int g_sensorSize = -1;  // what the sensor is set to now (caller holds the lock)

// OV5640 (5 MP): scans use 2560x1920, just under the 2576 px Claude reads
// without shrinking. That is already as much detail as a close-up would add,
// so it sends the one photo (no zoom + enhance) and takes 4 frames instead of
// 8 so they fit in PSRAM. The "framesize" setting then applies to tuning only.
bool g_is5640 = false;
int scanSize() { return g_is5640 ? FRAMESIZE_QSXGA : tuning("framesize"); }

bool g_asleep = false;  // powered down while the calculator is off (cameraSleep)

// The OV5640 module's lens motor: its focus firmware is loaded at start and
// runs continuous autofocus; a scan waits until it reports "focused".
OV5640 g_af;
bool g_hasAf = false;
constexpr uint8_t kFocused = 0x10;  // FW_STATUS_S_FOCUSED

void startAutofocus(sensor_t* s) {
  g_hasAf = g_af.start(s) && g_af.focusInit() == 0 && g_af.autoFocusMode() == 0;
  Serial.println(g_hasAf ? "Autofocus on." : "Autofocus not available (fixed-focus module?).");
}

// Caller holds the lock. Up to maxMs; a fixed-focus module returns at once.
void waitForFocus(uint32_t maxMs) {
  const uint32_t t0 = millis();
  while (g_hasAf && g_af.getFWStatus() != kFocused && millis() - t0 < maxMs) {
    if (camera_fb_t* fb = esp_camera_fb_get()) esp_camera_fb_return(fb);  // keeps exposure following
  }
}

void useSize(int size) {
  if (g_sensorSize == size) return;
  sensor_t* s = esp_camera_sensor_get();
  if (!s || s->set_framesize(s, (framesize_t)size) != 0) return;
  g_sensorSize = size;
  // The first frames after a size change are garbled or wrongly exposed.
  for (int i = 0; i < 2; ++i)
    if (camera_fb_t* fb = esp_camera_fb_get()) esp_camera_fb_return(fb);
}

// Caller holds the lock.
void setNamed(const char* name, int v) {
  for (int i = 0; i < kTuningCount; ++i)
    if (!strcmp(kTuning[i].name, name) && v >= kTuning[i].min && v <= kTuning[i].max &&
        kTuning[i].apply(esp_camera_sensor_get(), v) == 0)
      g_value[i] = v;
}

struct Lock {
  Lock() { xSemaphoreTake(g_lock, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(g_lock); }
};

camera_config_t configFor(const CameraPins& p) {
  camera_config_t c = {};
  c.pin_pwdn = p.pwdn;
  c.pin_reset = p.reset;
  c.pin_xclk = p.xclk;
  c.pin_sccb_sda = p.sda;
  c.pin_sccb_scl = p.scl;
  c.pin_d0 = p.d0;
  c.pin_d1 = p.d1;
  c.pin_d2 = p.d2;
  c.pin_d3 = p.d3;
  c.pin_d4 = p.d4;
  c.pin_d5 = p.d5;
  c.pin_d6 = p.d6;
  c.pin_d7 = p.d7;
  c.pin_vsync = p.vsync;
  c.pin_href = p.href;
  c.pin_pclk = p.pclk;
  c.xclk_freq_hz = 20000000;  // 20 MHz: reliable on OV3660/OV5640
  c.ledc_timer = LEDC_TIMER_0;
  c.ledc_channel = LEDC_CHANNEL_0;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_UXGA;  // 1600x1200: the size the simulator sends (also the largest allowed)
  c.jpeg_quality = 12;            // lower = better; ~150-250 KB per photo
  c.fb_count = 2;  // one frame fills while the other is sent: smoother live view
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_LATEST;
  return c;
}

const char* sensorName(uint16_t pid) {
  switch (pid) {
    case OV2640_PID: return "OV2640";
    case OV3660_PID: return "OV3660";
    case OV5640_PID: return "OV5640";
    default: return "unknown sensor";
  }
}

void loadTuning() {
  Preferences p;
  p.begin("cam", true);  // read-only: missing namespace just means defaults
  for (int i = 0; i < kTuningCount; ++i) {
    int v = p.getInt(kTuning[i].name, kTuning[i].def);
    g_value[i] = (v < kTuning[i].min || v > kTuning[i].max) ? kTuning[i].def : v;
  }
  p.end();
}

void applyAll(sensor_t* s) {
  for (int i = 0; i < kTuningCount; ++i) kTuning[i].apply(s, g_value[i]);
}

// Caller holds the lock.
bool grab(std::string& jpeg) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->format != PIXFORMAT_JPEG || fb->len == 0) {
    if (fb) esp_camera_fb_return(fb);
    return false;
  }
  jpeg.assign(reinterpret_cast<const char*>(fb->buf), fb->len);
  esp_camera_fb_return(fb);
  return true;
}
// ---- decoding for the focus check and the close-up ----
// esp_jpg_decode hands over the picture in small RGB blocks; we keep gray
// (the camera already shoots grayscale, so any channel would do).
struct Decode {
  const std::string* jpeg;
  uint32_t lastYield = millis();      // decoding is pure CPU: let other tasks run
  int fullW = 0, fullH = 0;           // size reported by the decoder
  uint8_t* out = nullptr;             // gray output
  int outW = 0, outH = 0;             // its size
  int cx = 0, cy = 0;                 // top-left of the wanted region (crop), else 0
  std::vector<uint8_t> thumb;         // used for the small whole-frame copy
};

size_t readJpeg(void* arg, size_t index, uint8_t* buf, size_t len) {
  const std::string& j = *static_cast<Decode*>(arg)->jpeg;
  if (index >= j.size()) return 0;
  len = std::min(len, j.size() - index);
  if (buf) memcpy(buf, j.data() + index, len);
  return len;
}

bool writeGray(void* arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* data) {
  Decode* d = static_cast<Decode*>(arg);
  if (!data) {  // start (x=y=0, w/h = picture size) or end
    if (x == 0 && y == 0 && d->fullW == 0) {
      d->fullW = w;
      d->fullH = h;
      if (!d->out) {  // whole-frame thumbnail
        d->thumb.assign(size_t(w) * h, 0);
        d->out = d->thumb.data();
        d->outW = w;
        d->outH = h;
      }
    }
    return true;
  }
  // A big decode takes ~1 s of CPU. Pause 1 tick every 50 ms so the idle task
  // and Wi-Fi on this core keep running (the task watchdog resets the chip
  // after 5 s without the idle task).
  if (millis() - d->lastYield > 50) {
    vTaskDelay(1);
    d->lastYield = millis();
  }
  for (int r = 0; r < h; ++r) {
    const int oy = y + r - d->cy;
    if (oy < 0 || oy >= d->outH) continue;
    for (int c = 0; c < w; ++c) {
      const int ox = x + c - d->cx;
      if (ox < 0 || ox >= d->outW) continue;
      const uint8_t* px = data + (size_t(r) * w + c) * 3;
      d->out[size_t(oy) * d->outW + ox] = uint8_t((px[0] + 2 * px[1] + px[2]) / 4);
    }
  }
  return true;
}

// The frame at 1/4 size in gray: 400x300 for UXGA.
bool thumbnail(const std::string& jpeg, Decode& d, jpg_scale_t scale = JPG_SCALE_4X) {
  d.jpeg = &jpeg;
  return esp_jpg_decode(jpeg.size(), scale, readJpeg, writeGray, &d) == ESP_OK && !d.thumb.empty();
}

// Zoom + enhance: decodes just the region (k) at full size, cleans it up
// (enhance.h: even lighting, contrast, resize to ~1.15 MP, sharpen) and
// re-encodes it as a JPEG.
bool enhancedCopy(const std::string& jpeg, const calc::Crop& k, std::string& out) {
  std::vector<uint8_t> gray;
  try {
    gray.assign(size_t(k.w) * k.h, 255);
  } catch (...) {
    return false;  // not enough memory: send the photo alone
  }
  Decode d;
  d.jpeg = &jpeg;
  d.out = gray.data();
  d.outW = k.w;
  d.outH = k.h;
  d.cx = k.x;
  d.cy = k.y;
  if (esp_jpg_decode(jpeg.size(), JPG_SCALE_NONE, readJpeg, writeGray, &d) != ESP_OK) return false;
  int w = k.w, h = k.h;
  // 2 MP: a whole UXGA frame keeps its full resolution (Claude reads up to
  // 2576 px / ~3.7 MP since Sonnet 5.5, so nothing is lost on its side), and a
  // close-up is enlarged further so small exponents and limits get more pixels.
  calc::enhanceWriting(gray, w, h, 2000000, [] { vTaskDelay(1); });
  uint8_t* jpg = nullptr;
  size_t len = 0;
  const bool ok = fmt2jpg(gray.data(), gray.size(), w, h, PIXFORMAT_GRAYSCALE, 90, &jpg, &len);
  if (ok) out.assign(reinterpret_cast<const char*>(jpg), len);
  free(jpg);
  return ok;
}
// Careful scan: try exposure, gain and contrast settings on the real page and
// keep the combination whose photo reads best (focus.h scanQuality). Caller
// holds the lock. About 4-6 s: each change needs the sensor to settle.
double tryQuality(std::string& line) {
  const uint32_t t0 = millis();
  while (millis() - t0 < 350) {  // let auto-exposure follow the change
    if (camera_fb_t* fb = esp_camera_fb_get()) esp_camera_fb_return(fb);
  }
  std::string jpeg;
  Decode d;
  if (!grab(jpeg) || !thumbnail(jpeg, d)) return 0;
  const calc::Focus f = calc::analyseFocus(d.thumb.data(), d.outW, d.outH);
  const calc::Exposure e = calc::measureExposure(d.thumb.data(), d.outW, d.outH, f);
  const double q = calc::scanQuality(f, e);
  char buf[64];
  snprintf(buf, sizeof buf, " q%.1f(paper %d ink %d)", q, e.paper, e.dark);
  line += buf;
  return q;
}

std::string autoTuneLocked() {
  useSize(tuning("framesize"));
  std::string report = "Tuning for the light:";
  int bestAe = tuning("ae_level"), bestContrast = tuning("contrast"), bestGain = tuning("gainceiling");
  double best = -1;
  for (int ae = -2; ae <= 2; ++ae) {
    setNamed("ae_level", ae);
    report += "\n  exposure " + std::to_string(ae) + ":";
    const double q = tryQuality(report);
    if (q > best) best = q, bestAe = ae;
  }
  setNamed("ae_level", bestAe);
  // Dim room: allow more gain once, keep it only if it reads better.
  if (bestGain < 5) {
    setNamed("gainceiling", 5);
    report += "\n  more gain:";
    const double q = tryQuality(report);
    if (q > best) best = q, bestGain = 5;
    setNamed("gainceiling", bestGain);
  }
  for (int c = 0; c <= 2; ++c) {
    setNamed("contrast", c);
    report += "\n  contrast " + std::to_string(c) + ":";
    const double q = tryQuality(report);
    if (q > best) best = q, bestContrast = c;
  }
  setNamed("contrast", bestContrast);
  report += "\n  -> exposure " + std::to_string(bestAe) + ", contrast " + std::to_string(bestContrast) +
            ", max gain " + std::to_string(bestGain) + (best <= 0 ? " (no writing found: kept as is)" : "");
  return report;
}
}  // namespace

std::string cameraAutoTune() {
  if (!g_ok) return "Camera not found.";
  Lock lock;
  return autoTuneLocked();
}

double cameraFocusScore(const std::string& jpeg) {
  Decode d;
  // Live frames are VGA: halve them (320x240) rather than quarter them.
  if (!thumbnail(jpeg, d, jpeg.size() < 120000 ? JPG_SCALE_2X : JPG_SCALE_4X)) return 0;
  return calc::analyseFocus(d.thumb.data(), d.outW, d.outH).score;
}

bool cameraBegin() {
  if (!g_lock) g_lock = xSemaphoreCreateMutex();
  g_asleep = false;
  if (!psramFound()) {
    g_name = "no PSRAM (check board settings)";
    return false;
  }
  loadTuning();
  for (const CameraPins& p : kCameraVariants) {
    camera_config_t c = configFor(p);
    if (esp_camera_init(&c) != ESP_OK) {
      esp_camera_deinit();
      continue;
    }
    sensor_t* s = esp_camera_sensor_get();
    if (!s) {
      esp_camera_deinit();
      continue;
    }
    if (s->id.PID == OV5640_PID) {
      // Frame buffers are sized at init: start again with room for 2560x1920.
      esp_camera_deinit();
      c.frame_size = FRAMESIZE_QSXGA;
      if (esp_camera_init(&c) != ESP_OK || !(s = esp_camera_sensor_get())) {
        esp_camera_deinit();
        continue;
      }
      g_is5640 = true;
      startAutofocus(s);
    }
    // White paper fools auto-exposure into darkening the whole picture. The
    // DSP exposure mode meters more evenly, and lens correction stops the
    // corners going dark. Exposure +1 (default above) lifts the paper back up.
    s->set_aec2(s, 1);
    s->set_lenc(s, 1);
    s->set_bpc(s, 1);
    s->set_wpc(s, 1);
    applyAll(s);  // grayscale + brighter by default: smaller photo, Claude only needs the ink
    g_sensorSize = -1;
    g_name = std::string(p.name) + ", " + sensorName(s->id.PID);
    g_ok = true;
    return true;
  }
  g_name = "not found";
  return false;
}

const char* cameraName() { return g_name.c_str(); }

uint32_t cameraHoldEstimateMs() {
  return (tuning("careful") ? 9 * kTryMs : 0) + uint32_t(tuning("settle")) * 100 + kFramesMs;
}

uint32_t cameraHoldUntil() { return g_holdUntil; }

bool cameraCapture(std::string& jpeg, std::string& detail, std::string& error,
                   const std::function<void()>& grabbed) {
  detail.clear();
  if (g_asleep) cameraBegin();  // powered down while the calculator was off
  if (!g_ok) {
    error = "Camera not found. Check its ribbon is latched.";
    return false;
  }
  const int kFrames = g_is5640 ? 4 : 8;
  std::string frames[8];
  {
    Lock lock;
    g_holdUntil = millis() + cameraHoldEstimateMs();
    // Tuning runs at the "framesize" setting (quick to decode); exposure
    // carries over to the scan size.
    if (tuning("careful")) Serial.println(autoTuneLocked().c_str());
    useSize(scanSize());  // also back to full size if the live view was running
    // Pressing = shakes a hand-held calculator, and exposure needs a few
    // frames to settle after idle: wait the "settle" time (1.5 s by default).
    const uint32_t t0 = millis(), settleMs = uint32_t(tuning("settle")) * 100;
    g_holdUntil = t0 + settleMs + kFramesMs;  // the real time left, now tuning is done
    while (millis() - t0 < settleMs) {
      if (camera_fb_t* fb = esp_camera_fb_get()) esp_camera_fb_return(fb);
    }
    waitForFocus(2000);  // autofocus modules only
    g_holdUntil = millis() + kFramesMs;  // the countdown, if focusing took a moment
    // Spread the frames over ~2 s so one wobble can't spoil all of them.
    for (int i = 0; i < kFrames; ++i) {
      grab(frames[i]);
      delay(g_is5640 ? 400 : 150);
    }
    g_holdUntil = 0;
  }
  if (grabbed) grabbed();

  // Keep the frame whose writing is sharpest (not just the biggest file: a
  // shaky frame can be big from noise). Analysis runs without the camera lock.
  const uint32_t t1 = millis();
  int best = -1;
  calc::Focus bestFocus;
  int thumbW = 0, thumbH = 0;
  std::string scores;
  for (int i = 0; i < kFrames; ++i) {
    if (frames[i].empty()) continue;
    vTaskDelay(1);
    Decode d;
    if (!thumbnail(frames[i], d)) {
      if (best < 0) best = i;  // undecodable but present: still better than nothing
      continue;
    }
    const calc::Focus f = calc::analyseFocus(d.thumb.data(), d.outW, d.outH);
    scores += (scores.empty() ? "" : ", ") + std::to_string(int(f.score));
    if (best < 0 || f.score > bestFocus.score) {
      best = i;
      bestFocus = f;
      thumbW = d.outW;
      thumbH = d.outH;
    }
  }
  if (best < 0) {
    error = "The camera didn't return a photo.";
    return false;
  }
  jpeg.swap(frames[best]);
  for (auto& f : frames) std::string().swap(f);  // free the others now

  if (g_is5640) {
    Serial.printf("Focus %s -> kept %d; %u KB photo, no close-up (5 MP); analysis %lu ms\n", scores.c_str(),
                  int(bestFocus.score), unsigned(jpeg.size() / 1024), millis() - t1);
  } else {
    // Zoom + enhance: the writing's close-up (or the whole frame when the
    // writing fills it), cleaned up, as a second image for Claude.
    calc::Crop k = calc::detailCrop(bestFocus, thumbW, thumbH, thumbW * 4, thumbH * 4);
    const bool zoomed = k.use;
    if (!zoomed && thumbW > 0) k = calc::Crop{true, 0, 0, thumbW * 4, thumbH * 4};
    if (k.use && !enhancedCopy(jpeg, k, detail)) detail.clear();
    Serial.printf("Focus %s -> kept %d; %s %dx%d -> %u KB enhanced; analysis %lu ms\n", scores.c_str(),
                  int(bestFocus.score), zoomed ? "zoomed on the writing" : "whole frame", k.w, k.h,
                  unsigned(detail.size() / 1024), millis() - t1);
  }

  Lock lock;
  g_last = jpeg;
  g_lastDetail = detail;
  return true;
}

void cameraSleep() {
  if (!g_ok || g_asleep) return;
  Lock lock;
  esp_camera_deinit();
  g_ok = false;
  g_asleep = true;
  g_sensorSize = -1;
  // A power-down pin, where the board has one, switches the sensor (and its
  // autofocus motor) off completely; the next scan starts it again.
  for (const CameraPins& p : kCameraVariants)
    if (p.pwdn >= 0) {
      pinMode(p.pwdn, OUTPUT);
      digitalWrite(p.pwdn, HIGH);
    }
}

bool cameraFrame(std::string& jpeg) {
  if (g_asleep) cameraBegin();
  if (!g_ok) return false;
  Lock lock;
  useSize(kLiveSize);
  return grab(jpeg);
}

bool cameraLastPhoto(std::string& jpeg) {
  if (!g_ok) return false;
  Lock lock;
  jpeg = g_last;
  return !jpeg.empty();
}

bool cameraLastDetail(std::string& jpeg) {
  if (!g_ok) return false;
  Lock lock;
  jpeg = g_lastDetail;
  return !jpeg.empty();
}

bool cameraSet(const std::string& name, int value) {
  if (!g_ok) return false;
  for (int i = 0; i < kTuningCount; ++i) {
    if (name != kTuning[i].name) continue;
    if (value < kTuning[i].min || value > kTuning[i].max) return false;
    Lock lock;
    if (kTuning[i].apply(esp_camera_sensor_get(), value) != 0) return false;
    g_value[i] = value;
    if (!strcmp(kTuning[i].name, "framesize")) g_sensorSize = value;
    return true;
  }
  return false;
}

std::string cameraTuningJson() {
  std::string j = "{";
  for (int i = 0; i < kTuningCount; ++i) {
    if (i) j += ",";
    j += "\"" + std::string(kTuning[i].name) + "\":" + std::to_string(g_value[i]);
  }
  return j + "}";
}

void cameraSaveTuning() {
  Preferences p;
  p.begin("cam", false);
  for (int i = 0; i < kTuningCount; ++i) p.putInt(kTuning[i].name, g_value[i]);
  p.end();
}

void cameraResetTuning() {
  Preferences p;
  p.begin("cam", false);
  p.clear();
  p.end();
  for (int i = 0; i < kTuningCount; ++i) g_value[i] = kTuning[i].def;
  if (!g_ok) return;
  Lock lock;
  applyAll(esp_camera_sensor_get());
  g_sensorSize = -1;
}
