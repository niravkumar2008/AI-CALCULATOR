#include "camera.h"

#include <Arduino.h>
#include <esp_camera.h>

#include "pins.h"

namespace {
std::string g_name = "none";
bool g_ok = false;

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
  c.frame_size = FRAMESIZE_UXGA;  // 1600x1200: the size the simulator sends
  c.jpeg_quality = 12;            // lower = better; ~150-250 KB per photo
  c.fb_count = 1;
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
}  // namespace

bool cameraBegin() {
  if (!psramFound()) {
    g_name = "no PSRAM (check board settings)";
    return false;
  }
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
    s->set_special_effect(s, 2);  // grayscale: smaller photo, Claude only needs the ink
    s->set_brightness(s, 1);      // indoor desks tend to come out dark
    g_name = std::string(p.name) + ", " + sensorName(s->id.PID);
    g_ok = true;
    return true;
  }
  g_name = "not found";
  return false;
}

const char* cameraName() { return g_name.c_str(); }

bool cameraCapture(std::string& jpeg, std::string& error) {
  if (!g_ok) {
    error = "Camera not found. Check its ribbon is latched.";
    return false;
  }
  // The first frames after idle are often over/under-exposed: drop two.
  for (int i = 0; i < 2; ++i) {
    if (camera_fb_t* fb = esp_camera_fb_get()) esp_camera_fb_return(fb);
  }
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->format != PIXFORMAT_JPEG || fb->len == 0) {
    if (fb) esp_camera_fb_return(fb);
    error = "The camera didn't return a photo.";
    return false;
  }
  jpeg.assign(reinterpret_cast<const char*>(fb->buf), fb->len);
  esp_camera_fb_return(fb);
  return true;
}
