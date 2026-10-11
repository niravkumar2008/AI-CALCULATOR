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

#include "log.h"
#include "pins.h"
#include "power.h"

#include <ESP32_OV5640_AF.h>

namespace {
std::string g_name = "off until the first scan";
bool g_ok = false;
SemaphoreHandle_t g_lock;  // one user of the sensor at a time (scan vs preview)
std::string g_last;        // last scan photo, for the preview page
std::string g_lastDetail;  // its close-up of the writing, if one was made
volatile uint32_t g_holdUntil = 0;  // see cameraHoldUntil()
constexpr uint32_t kTryMs = 750;    // one careful-scan setting: settle + frame + check
constexpr uint32_t kFramesMs = 2000; // the 8 frames to choose from
constexpr uint32_t kCamPowerSettleMs = 5;    // regulators up before RESET is released (review N3)
constexpr uint32_t kCamResetReleaseMs = 20;  // after RESET, before the first SCCB access
constexpr uint32_t kFocusWaitMs = 2000;      // autofocus modules: wait for "focused" this long

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

// OV5640 (5 MP): scans use its full detail, so it sends the one photo (no
// zoom + enhance) and takes 4 frames instead of 8. The "framesize" setting
// then applies to tuning only.
// The board's MINI-1-N4R2 has 2 MB of PSRAM: room for one 2048x1536 frame
// buffer (~630 KB) and the photo kept, not two 2560x1920 buffers (~2 MB).
// With 4 MB or more (a module swap) it uses 2560x1920, just under the
// 2576 px Claude reads without shrinking.
bool g_is5640 = false;
bool g_lowMem = false;  // under 4 MB of PSRAM
int scanSize() { return g_is5640 ? (g_lowMem ? FRAMESIZE_QXGA : FRAMESIZE_QSXGA) : tuning("framesize"); }

bool g_asleep = true;  // powered down (at start, and while the calculator is off): the next scan powers it up

// v15: the sensor runs in one of two driver modes. Scans use the sensor's JPEG encoder at
// full size (as in v14); the LCD viewfinder takes raw 320x240 RGB565 frames (~15-25 fps,
// no decoding). The esp32-camera driver sizes its DMA and frame buffers at init, so a
// switch is a deinit + init (the AF firmware is loaded again, ~0.5-1 s).
enum class Mode : uint8_t { Jpeg, Rgb565 };
Mode g_mode = Mode::Jpeg;
const CameraPins* g_pins = nullptr;  // the layout that answered
void ensureJpeg();                   // below: back to scan mode before a capture (caller holds the lock)

// The OV5640 module's lens motor: its focus firmware is loaded at start and
// runs continuous autofocus; a scan waits until it reports "focused".
OV5640 g_af;
bool g_hasAf = false;
constexpr uint8_t kFocused = 0x10;  // FW_STATUS_S_FOCUSED

void startAutofocus(sensor_t* s) {
  g_hasAf = g_af.start(s) && g_af.focusInit() == 0 && g_af.autoFocusMode() == 0;
  LOGF("cam", "%s", g_hasAf ? "autofocus on" : "autofocus not available (fixed-focus module?)");
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

camera_config_t configFor(const CameraPins& p, Mode mode = Mode::Jpeg) {
  camera_config_t c = {};
  if (mode == Mode::Rgb565) {
    // The viewfinder: two 320x240x2 = 154 KB buffers in PSRAM, always the newest frame.
    c.pixel_format = PIXFORMAT_RGB565;
    c.frame_size = FRAMESIZE_QVGA;
    c.fb_count = 2;
    c.fb_location = CAMERA_FB_IN_PSRAM;
    c.grab_mode = CAMERA_GRAB_LATEST;
  }
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
  if (mode == Mode::Jpeg) {
    c.pixel_format = PIXFORMAT_JPEG;
    c.frame_size = FRAMESIZE_UXGA;  // 1600x1200: the size the simulator sends (also the largest allowed)
    c.jpeg_quality = 12;            // lower = better; ~150-250 KB per photo
    c.fb_count = g_lowMem ? 1 : 2;  // two: one frame fills while the other is sent (smoother live view)
    c.fb_location = CAMERA_FB_IN_PSRAM;
    c.grab_mode = g_lowMem ? CAMERA_GRAB_WHEN_EMPTY : CAMERA_GRAB_LATEST;  // LATEST needs 2 buffers
  }
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
  for (int i = 0; i < kTuningCount; ++i) {
    // Preview mode keeps the driver's QVGA frame size and shows colour: the "framesize"
    // and "grayscale" settings are for scans only.
    if (g_mode == Mode::Rgb565 && (!strcmp(kTuning[i].name, "framesize") || !strcmp(kTuning[i].name, "grayscale")))
      continue;
    kTuning[i].apply(s, g_value[i]);
  }
}

// Initialises the driver in `mode` for the pin layout `p` and sets the sensor up. Caller
// holds the lock (or is cameraBegin before anyone else can). The camera must be powered.
bool bringUp(const CameraPins& p, Mode mode) {
  camera_config_t c = configFor(p, mode);
  if (esp_camera_init(&c) != ESP_OK) {
    esp_camera_deinit();
    return false;
  }
  sensor_t* s = esp_camera_sensor_get();
  if (!s) {
    esp_camera_deinit();
    return false;
  }
  if (s->id.PID == OV5640_PID) {
    g_is5640 = true;
    if (mode == Mode::Jpeg && c.frame_size != framesize_t(scanSize())) {
      // Frame buffers are sized at init: start again with room for the scan size.
      esp_camera_deinit();
      c.frame_size = framesize_t(scanSize());
      if (esp_camera_init(&c) != ESP_OK || !(s = esp_camera_sensor_get())) {
        esp_camera_deinit();
        g_is5640 = false;
        return false;
      }
    }
    startAutofocus(s);
  }
  // White paper fools auto-exposure into darkening the whole picture. The
  // DSP exposure mode meters more evenly, and lens correction stops the
  // corners going dark. Exposure +1 (default above) lifts the paper back up.
  pinMode(PIN_CAM_RESET, OUTPUT_OPEN_DRAIN);  // released: R8 holds it at 2.8 V
  digitalWrite(PIN_CAM_RESET, HIGH);
  s->set_aec2(s, 1);
  s->set_lenc(s, 1);
  s->set_bpc(s, 1);
  s->set_wpc(s, 1);
  g_mode = mode;
  applyAll(s);  // grayscale + brighter by default: smaller photo, Claude only needs the ink
  if (mode == Mode::Rgb565) {
    s->set_special_effect(s, 0);  // colour for the viewfinder
    g_sensorSize = FRAMESIZE_QVGA;
  } else {
    g_sensorSize = -1;
  }
  g_pins = &p;
  g_name = std::string(p.name) + ", " + sensorName(s->id.PID);
  g_ok = true;
  return true;
}

// Caller holds the lock. Switches the driver between scan (JPEG) and preview (RGB565)
// mode; the sensor stays powered (its reset is pulsed by esp_camera_init).
bool switchMode(Mode mode) {
  if (!g_ok || !g_pins) return false;
  if (g_mode == mode) return true;
  const uint32_t t0 = millis();
  esp_camera_deinit();
  g_ok = false;
  g_hasAf = false;
  if (!bringUp(*g_pins, mode)) {
    LOGF("cam", "could not switch to %s mode", mode == Mode::Jpeg ? "scan" : "preview");
    return false;
  }
  LOGF("cam", "%s mode in %lu ms", mode == Mode::Jpeg ? "scan (JPEG)" : "preview (RGB565 320x240)", millis() - t0);
  return true;
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
  const char* data = nullptr;         // the JPEG being decoded
  size_t size = 0;
  uint32_t lastYield = millis();      // decoding is pure CPU: let other tasks run
  int fullW = 0, fullH = 0;           // size reported by the decoder
  uint8_t* out = nullptr;             // gray output
  int outW = 0, outH = 0;             // its size
  int cx = 0, cy = 0;                 // top-left of the wanted region (crop), else 0
  std::vector<uint8_t> thumb;         // used for the small whole-frame copy
};

size_t readJpeg(void* arg, size_t index, uint8_t* buf, size_t len) {
  const Decode* d = static_cast<Decode*>(arg);
  if (index >= d->size) return 0;
  len = std::min(len, d->size - index);
  if (buf) memcpy(buf, d->data + index, len);
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
bool thumbnail(const char* jpeg, size_t size, Decode& d, jpg_scale_t scale = JPG_SCALE_4X) {
  d.data = jpeg;
  d.size = size;
  return esp_jpg_decode(size, scale, readJpeg, writeGray, &d) == ESP_OK && !d.thumb.empty();
}
bool thumbnail(const std::string& jpeg, Decode& d, jpg_scale_t scale = JPG_SCALE_4X) {
  return thumbnail(jpeg.data(), jpeg.size(), d, scale);
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
  d.data = jpeg.data();
  d.size = jpeg.size();
  d.out = gray.data();
  d.outW = k.w;
  d.outH = k.h;
  d.cx = k.x;
  d.cy = k.y;
  if (esp_jpg_decode(d.size, JPG_SCALE_NONE, readJpeg, writeGray, &d) != ESP_OK) return false;
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
  ensureJpeg();
  return autoTuneLocked();
}

double cameraFocusScore(const std::string& jpeg) {
  Decode d;
  // Live frames are VGA: halve them (320x240) rather than quarter them.
  if (!thumbnail(jpeg, d, jpeg.size() < 120000 ? JPG_SCALE_2X : JPG_SCALE_4X)) return 0;
  return calc::analyseFocus(d.thumb.data(), d.outW, d.outH).score;
}

namespace {
bool beginIn(Mode mode);
}

bool cameraBegin() { return beginIn(Mode::Jpeg); }

namespace {
bool beginIn(Mode mode) {
  if (!g_lock) g_lock = xSemaphoreCreateMutex();
  g_asleep = false;
  if (!psramFound()) {
    g_name = "no PSRAM (check board settings)";
    return false;
  }
  loadTuning();
  g_lowMem = ESP.getPsramSize() < 4u * 1024 * 1024;
  // Power-up order (review N3): RESET and PWDN low first, then both camera
  // regulators (CAM_PWR_EN), 5 ms to settle; esp_camera_init then pulses RESET
  // and waits before talking SCCB. PWDN is never driven high: a powered-down
  // sensor is simply unpowered.
  // RESET is open-drain: R8 (10 k to CAM_2V8) pulls it high, so the pin never pushes
  // 3.3 V into the 2.8 V net (review M3). esp_camera_init pulses it push-pull for
  // 20 ms, which is harmless; it is set back to open-drain right after.
  pinMode(PIN_CAM_RESET, OUTPUT_OPEN_DRAIN);
  digitalWrite(PIN_CAM_RESET, LOW);
  pinMode(PIN_CAM_PWDN, OUTPUT);
  digitalWrite(PIN_CAM_PWDN, LOW);
  pinMode(PIN_CAM_PWR_EN, OUTPUT);
  digitalWrite(PIN_CAM_PWR_EN, HIGH);
  delay(kCamPowerSettleMs);
  digitalWrite(PIN_CAM_RESET, HIGH);
  delay(kCamResetReleaseMs);
  for (const CameraPins& p : kCameraVariants)
    if (bringUp(p, mode)) return true;
  g_name = "not found";
  powerCameraPinsSafe();  // regulators off, every camera pin disabled with no pull
  g_asleep = true;        // try again (power-up included) at the next scan
  return false;
}

// Caller holds the lock: the sensor in scan (JPEG) mode for a capture.
void ensureJpeg() {
  if (g_ok && g_mode != Mode::Jpeg) switchMode(Mode::Jpeg);
}
}  // namespace

const char* cameraName() { return g_name.c_str(); }

constexpr uint32_t kModeSwitchMs = 800;  // deinit + init + AF firmware, measured on the bench later

uint32_t cameraHoldEstimateMs() {
  return (tuning("careful") ? 9 * kTryMs : 0) + uint32_t(tuning("settle")) * 100 + kFramesMs +
         (g_mode == Mode::Rgb565 ? kModeSwitchMs : 0);
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
  // The frames are scored one at a time as they arrive and only the sharpest
  // is kept: 2 MB of PSRAM has room for one frame buffer and one kept photo,
  // not 4-8 photos. Scoring (~0.3 s a frame) also spaces them out, so one
  // wobble can't spoil them all.
  const uint32_t t1 = millis();
  calc::Focus bestFocus;
  int thumbW = 0, thumbH = 0;
  std::string scores;
  jpeg.clear();
  {
    Lock lock;
    ensureJpeg();  // v15: back from the viewfinder's RGB565 mode (~0.5-1 s, counted in the hold estimate)
    g_holdUntil = millis() + cameraHoldEstimateMs();
    // Tuning runs at the "framesize" setting (quick to decode); exposure
    // carries over to the scan size.
    if (tuning("careful")) LOGF("cam", "%s", autoTuneLocked().c_str());
    useSize(scanSize());  // also back to full size if the live view was running
    // Pressing = shakes a hand-held calculator, and exposure needs a few
    // frames to settle after idle: wait the "settle" time (1.5 s by default).
    const uint32_t t0 = millis(), settleMs = uint32_t(tuning("settle")) * 100;
    g_holdUntil = t0 + settleMs + kFramesMs;  // the real time left, now tuning is done
    while (millis() - t0 < settleMs) {
      if (camera_fb_t* fb = esp_camera_fb_get()) esp_camera_fb_return(fb);
    }
    waitForFocus(kFocusWaitMs);  // autofocus modules only
    g_holdUntil = millis() + kFramesMs;  // the countdown, if focusing took a moment
    for (int i = 0; i < kFrames; ++i) {
      camera_fb_t* fb = esp_camera_fb_get();
      if (!fb) continue;
      if (fb->format == PIXFORMAT_JPEG && fb->len) {
        const char* buf = reinterpret_cast<const char*>(fb->buf);
        Decode d;
        if (thumbnail(buf, fb->len, d)) {
          const calc::Focus f = calc::analyseFocus(d.thumb.data(), d.outW, d.outH);
          scores += (scores.empty() ? "" : ", ") + std::to_string(int(f.score));
          if (jpeg.empty() || f.score > bestFocus.score) {
            jpeg.assign(buf, fb->len);
            bestFocus = f;
            thumbW = d.outW;
            thumbH = d.outH;
          }
        } else if (jpeg.empty()) {
          jpeg.assign(buf, fb->len);  // undecodable but present: still better than nothing
        }
      }
      esp_camera_fb_return(fb);
    }
    g_holdUntil = 0;
  }
  if (grabbed) grabbed();
  if (jpeg.empty()) {
    error = "The camera didn't return a photo.";
    return false;
  }

  if (g_is5640 || g_lowMem) {  // no room for a close-up in 2 MB, and the OV5640 doesn't need one
    LOGF("cam", "focus %s -> kept %d; %u KB photo, no close-up; %lu ms", scores.c_str(),
                  int(bestFocus.score), unsigned(jpeg.size() / 1024), millis() - t1);
  } else {
    // Zoom + enhance: the writing's close-up (or the whole frame when the
    // writing fills it), cleaned up, as a second image for Claude.
    calc::Crop k = calc::detailCrop(bestFocus, thumbW, thumbH, thumbW * 4, thumbH * 4);
    const bool zoomed = k.use;
    if (!zoomed && thumbW > 0) k = calc::Crop{true, 0, 0, thumbW * 4, thumbH * 4};
    if (k.use && !enhancedCopy(jpeg, k, detail)) detail.clear();
    LOGF("cam", "focus %s -> kept %d; %s %dx%d -> %u KB enhanced; analysis %lu ms", scores.c_str(),
                  int(bestFocus.score), zoomed ? "zoomed on the writing" : "whole frame", k.w, k.h,
                  unsigned(detail.size() / 1024), millis() - t1);
  }

  // ponytail: with 2 MB of PSRAM the copy for the preview page's "last scan
  // photo" doesn't fit next to the request being sent, so it's skipped.
  if (!g_lowMem) {
    Lock lock;
    g_last = jpeg;
    g_lastDetail = detail;
  }
  return true;
}

void cameraSleep() {
  if (!g_ok || g_asleep) return;
  Lock lock;
  esp_camera_deinit();
  g_ok = false;
  g_asleep = true;
  g_sensorSize = -1;
  g_mode = Mode::Jpeg;
  g_hasAf = false;
  // Regulators off: the sensor and its autofocus motor draw nothing. Every
  // camera pin is left disabled with no pull-up or pull-down (the SCCB driver
  // turns pull-ups on, and 3.3 V on SIOD/SIOC would back-power the sensor
  // through R18/R19). The board's R9 holds PWDN low. The next scan powers it up.
  powerCameraPinsSafe();
}

bool cameraIsOn() { return g_ok && !g_asleep; }

bool cameraPreviewFrame(std::vector<uint8_t>& gray, int& w, int& h, bool& focused) {
  if (g_asleep || !g_ok) cameraBegin();  // first frame: powers the camera up (~1 s with the AF firmware)
  if (!g_ok) return false;
  Lock lock;
  // QVGA JPEG decoded at 1/2 scale = 160x120 gray: small and quick (~20 ms to decode),
  // and still more pixels than the 162x122 viewfinder needs. The frame buffer was
  // sized for the scan photo, so nothing new is allocated.
  useSize(FRAMESIZE_QVGA);
  // One buffer (2 MB PSRAM): the frame waiting in it was taken when the last one was
  // returned, possibly half a second ago. Drop it so the viewfinder shows "now".
  if (camera_fb_t* stale = esp_camera_fb_get()) esp_camera_fb_return(stale);
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return false;
  Decode d;
  const bool ok = fb->format == PIXFORMAT_JPEG && fb->len &&
                  thumbnail(reinterpret_cast<const char*>(fb->buf), fb->len, d, JPG_SCALE_2X);
  esp_camera_fb_return(fb);
  if (!ok) return false;
  gray.swap(d.thumb);
  w = d.outW;
  h = d.outH;
  focused = !g_hasAf || g_af.getFWStatus() == kFocused;  // continuous AF runs on the sensor itself
  return true;
}

bool cameraSelfTest(std::string& sensor, size_t& bytes, uint32_t& ms, bool& autofocus) {
  const uint32_t t0 = millis();
  bytes = 0;
  if (!g_ok) cameraBegin();
  sensor = g_name;
  autofocus = g_hasAf;
  if (!g_ok) {
    ms = millis() - t0;
    return false;
  }
  {
    Lock lock;
    ensureJpeg();
    useSize(FRAMESIZE_VGA);  // quick: proves the bus, the clock and the JPEG encoder
    std::string jpeg;
    for (int i = 0; i < 3 && jpeg.empty(); ++i) grab(jpeg);
    bytes = jpeg.size();
  }
  ms = millis() - t0;
  return bytes > 1000;
}

bool cameraFrame(std::string& jpeg) {
  if (g_asleep) cameraBegin();
  if (!g_ok) return false;
  Lock lock;
  ensureJpeg();
  useSize(kLiveSize);
  return grab(jpeg);
}

// ---- v15 colour viewfinder ----
bool cameraPreviewBegin() {
  if (g_asleep || !g_ok) {
    if (!beginIn(Mode::Rgb565)) return false;  // powers it up straight into preview mode
  }
  Lock lock;
  if (g_mode != Mode::Rgb565 && !switchMode(Mode::Rgb565)) return false;
  // Continuous autofocus (the AF firmware's own loop); a key can trigger a single shot.
  if (g_hasAf) g_af.autoFocusMode();
  return true;
}

bool cameraPreviewMode() { return g_ok && !g_asleep && g_mode == Mode::Rgb565; }

camera_fb_t* cameraPreviewGrab(bool& focused) {
  if (!cameraPreviewMode()) return nullptr;
  Lock lock;
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb && (fb->format != PIXFORMAT_RGB565 || fb->len < size_t(320) * 240 * 2)) {
    esp_camera_fb_return(fb);
    fb = nullptr;
  }
  focused = !g_hasAf || g_af.getFWStatus() == kFocused;
  return fb;
}

void cameraPreviewRelease(camera_fb_t* fb) {
  if (fb) esp_camera_fb_return(fb);
}

bool cameraRefocus() {
  if (!g_ok || !g_hasAf) return false;
  Lock lock;
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  // OV5640 AF firmware: 0x3023 = command ack (1 = busy), 0x3022 = command: 0x03 single-shot
  // focus (then it holds); 0x04 continuous. The status (0x3029) reads 0x10 when done.
  s->set_reg(s, 0x3023, 0xFF, 0x01);
  s->set_reg(s, 0x3022, 0xFF, 0x03);
  return true;
}

CamFocus cameraFocusState() {
  if (!g_ok || !g_hasAf) return CamFocus::NoAf;
  Lock lock;
  return g_af.getFWStatus() == kFocused ? CamFocus::Focused : CamFocus::Focusing;
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
