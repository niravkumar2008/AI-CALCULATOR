// UI screenshot harness for the v15 colour LCD. Every picture is drawn by the firmware's
// own code (firmware-v15-lcd/src/ui.cpp, fbtext.cpp, vf_lcd.cpp, selftest.cpp, compiled
// unchanged) into a real LovyanGFX LGFX_Sprite, fed by the real core/ calculator
// (calc::Device, App, Viewfinder). Only the panel is missing: lcd::flush() copies the
// canvas out (host_hw.cpp) and this program saves it as a PPM.
//
//   ui_sim_v15.exe <out_dir> <photo_320x240.ppm>   (build.bat runs it; make_screens.py -> PNG)
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Arduino.h"
#include "device.h"
#include "framebuffer.h"
#include "host_capture.h"
#include "lcd.h"
#include "ui.h"
#include "vf_lcd.h"
#include "viewfinder.h"

using calc::Device;
using calc::DKey;

void hostRunSelfTest(const std::string& prefix);  // selftest_tu.cpp

namespace {

calc::Framebuffer g_fb;

void key(Device& d, DKey k) {
  host::nowMs += 120;
  d.tick(host::nowMs);
  d.onKey(k);
}

// Keys as typed in the Serial Monitor (calc::keysForChar: * / q=x² r=√ s=sin p=π
// l=log $=AC [=SHIFT m=MODE o=ON d=▼ ...).
void type(Device& d, const char* s) {
  for (; *s; ++s) {
    DKey k[2];
    int n = 0;
    if (!calc::keysForChar(*s, k, n)) {
      std::fprintf(stderr, "no key for '%c'\n", *s);
      continue;
    }
    for (int i = 0; i < n; ++i) key(d, k[i]);
  }
}

ui::Status status(int battery, bool charging = false, bool wifiOn = false, bool wifiConnected = false,
                  bool low = false) {
  ui::Status st;
  st.battery = battery;
  st.charging = charging;
  st.wifiOn = wifiOn;
  st.wifiConnected = wifiConnected;
  st.lowBattery = low;
  return st;
}

// The calculator screen as the firmware's loop() draws it: core renders its 1-bit
// picture, ui::showDevice() re-typesets it on the LCD canvas. Time is rounded to a whole
// second (cursor blink phase "on") so the cursor shows.
void shot(Device& d, const ui::Status& st, const std::string& name) {
  host::nowMs = (host::nowMs / 1000 + 1) * 1000;
  d.tick(host::nowMs);
  d.setBattery(st.battery, st.charging);
  d.render(g_fb);
  ui::showDevice(d, g_fb, st, host::nowMs, true);
  host::save(name);
}

void setup(Device& d) {
  d.setOnline(true);
  d.setHasApiKey(true);
  d.setBlinkingCursor(true);
  d.setBattery(78, false);
  d.setNetInfo("Nirav's iPhone", "saved (...a1b2)", "SETUP 6, or USB: wifi / key");
  d.tick(host::nowMs);
}

// ---- the camera
std::vector<uint8_t> g_photo;  // 320 x 240 RGB888
int g_pw = 0, g_ph = 0;

bool loadPpm(const char* path) {
  FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  int maxv = 0;
  if (std::fscanf(f, "P6 %d %d %d", &g_pw, &g_ph, &maxv) != 3 || maxv != 255) {
    std::fclose(f);
    return false;
  }
  std::fgetc(f);
  g_photo.resize(size_t(g_pw) * g_ph * 3);
  const bool ok = std::fread(g_photo.data(), 1, g_photo.size(), f) == g_photo.size();
  std::fclose(f);
  return ok && g_pw == vflcd::kFrameW && g_ph == vflcd::kFrameH;
}

// The camera's 320x240 RGB565 frame in panel byte order (high byte first), as the
// OV5640 DMA leaves it with PREVIEW_SWAP_BYTES=0. dx/dy shift the picture (a moving hand).
std::vector<uint8_t> cameraFrame(int dx = 0, int dy = 0) {
  std::vector<uint8_t> buf(size_t(vflcd::kFrameW) * vflcd::kFrameH * 2);
  for (int y = 0; y < g_ph; ++y)
    for (int x = 0; x < g_pw; ++x) {
      int sx = x - dx, sy = y - dy;
      sx = sx < 0 ? 0 : sx >= g_pw ? g_pw - 1 : sx;
      sy = sy < 0 ? 0 : sy >= g_ph ? g_ph - 1 : sy;
      const uint8_t* p = &g_photo[(size_t(sy) * g_pw + sx) * 3];
      const uint16_t v = lcd::rgb(p[0], p[1], p[2]);
      buf[(size_t(y) * g_pw + x) * 2] = uint8_t(v >> 8);
      buf[(size_t(y) * g_pw + x) * 2 + 1] = uint8_t(v & 0xFF);
    }
  return buf;
}

// As main.cpp's viewfinder step: core's Viewfinder analyses a grey thumbnail of the
// frame, then vflcd::showFrame() draws the overlay into the frame and pushes it.
void cameraShot(calc::Viewfinder& vf, std::vector<uint8_t> frame, bool afFocused, vflcd::FocusState focus,
                const std::string& mode, uint32_t msSinceKey, const std::string& name, int battery = 78,
                bool charging = false, bool wifi = true) {
  uint8_t grey[80 * 60];
  vflcd::greyThumb(frame.data(), grey);
  host::nowMs += 70;
  vf.feed(grey, 80, 60, afFocused, host::nowMs);
  vflcd::Overlay o;
  o.info = vf.info();
  o.focus = focus;
  vf.touch(host::nowMs - msSinceKey);
  o.secondsLeft = vf.secondsLeft(host::nowMs);
  o.mode = mode;
  o.fps = 14.3;
  o.battery = battery;
  o.charging = charging;
  o.wifiConnected = wifi;
  vflcd::showFrame(frame.data(), o);
  host::save(name);
  std::printf("    (focus state %d, sharpness %.0f, motion %.1f, text %s %d,%d %dx%d)\n", int(o.info.state),
              o.info.sharpness, o.info.motion, o.info.textFound ? "found" : "none", o.info.tx, o.info.ty, o.info.tw,
              o.info.th);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: ui_sim_v15 <out_dir> <photo_320x240.ppm>\n");
    return 2;
  }
  host::outDir = argv[1];
  if (!loadPpm(argv[2])) {
    std::fprintf(stderr, "can't read a 320x240 P6 PPM from %s\n", argv[2]);
    return 1;
  }
  lcd::begin();
  ui::begin();
  std::printf("calculator:\n");
  const ui::Status normal = status(78);

  // 1. The calculator: home, typing, results, SHIFT, a fraction, an error.
  {
    Device d;
    setup(d);
    shot(d, normal, "01_calc_home");
    type(d, "2*(3+4)q/7+r16");  // 2×(3+4)²÷7+√16, cursor at the end
    shot(d, normal, "02_calc_typing");
    type(d, "=");
    shot(d, normal, "03_calc_result");
    type(d, "$s30)+p*2q-l100)");  // sin(30)+π×2²−log(100)
    shot(d, normal, "04_calc_typing_functions");
    type(d, "=");
    shot(d, normal, "05_calc_result_functions");
    type(d, "$[");
    shot(d, normal, "06_calc_shift");
    type(d, "[");  // SHIFT again: off
    type(d, "1/3+1/4=");
    shot(d, normal, "07_calc_result_fraction");
    type(d, "$1+*2=");
    shot(d, normal, "08_calc_syntax_error");
  }
  // 2. Menus.
  {
    Device d;
    setup(d);
    type(d, "m");
    shot(d, normal, "10_menu_mode");
    type(d, "$[m");
    shot(d, normal, "11_setup");
  }
  // 3. AI SOLVE: ready, hold still, reading, solving, answer pages.
  std::printf("ai solve:\n");
  {
    Device d;
    setup(d);
    const ui::Status wifi = status(78, false, true, true);
    type(d, "m4");
    shot(d, wifi, "20_ai_ready");
    type(d, "=");
    const int id = d.takeRequest();
    d.setHoldUntil(host::nowMs + 2500);
    shot(d, wifi, "21_ai_hold_still");
    d.setHoldUntil(0);
    shot(d, wifi, "22_ai_reading");
    d.onCaptured(id);
    host::nowMs += 800;
    shot(d, wifi, "23_ai_solving");
    d.onReply(id,
              R"({"readable":true,"confidence":0.97,"unclear":[],"read_as":"Solve for x: 3x + 7 = 22",)"
              R"("expression":"(22-7)/3","choice":"","answer":"x = 5","check":"(22-7)/3",)"
              R"("steps":["Subtract 7 from both sides: 3x = 15.","Divide both sides by 3: x = 5.",)"
              R"("Check: 3(5) + 7 = 15 + 7 = 22."]})");
    shot(d, wifi, "24_ai_answer_page1");
    for (int p = 2; p <= 3; ++p) {
      for (int i = 0; i < 6; ++i) key(d, DKey::Down);
      shot(d, wifi, "2" + std::to_string(3 + p) + "_ai_answer_page" + std::to_string(p));
    }
  }
  // 4. Status bar variants (on the home screen).
  std::printf("status bar:\n");
  {
    Device d;
    setup(d);
    shot(d, status(100), "30_status_full_no_wifi");
    shot(d, status(64, false, true, false), "31_status_wifi_joining");
    shot(d, status(64, false, true, true), "32_status_wifi_connected");
    shot(d, status(45, true, true, true), "33_status_charging");
    shot(d, status(15), "34_status_low_battery");
    shot(d, status(-1), "35_status_battery_unknown");
  }
  // 5. Low battery: AI locked out (main.cpp tells core with setLowBattery).
  {
    Device d;
    setup(d);
    d.setLowBattery(true);
    const ui::Status low = status(6, false, false, false, true);
    type(d, "m4");
    shot(d, low, "36_low_battery_ai_screen");
    type(d, "=");
    shot(d, low, "37_low_battery_notice");
  }
  // 6. Off with the cable in, off, and exam mode (SHIFT 7 ON while off).
  {
    Device d;
    setup(d);
    type(d, "[$");
    shot(d, status(64, true), "40_off_charging");
    shot(d, status(64, false), "41_off_blank");
    type(d, "[7o");
    shot(d, status(92), "42_exam_mode_on");
    type(d, "$2+2=");
    shot(d, status(92), "43_exam_mode_calc");
  }
  // 7. The firmware's own pages, with the strings main.cpp passes.
  std::printf("firmware pages:\n");
  {
    const ui::Status st = status(78);
    // main.cpp drawSetupScreen(): name and password in setup_portal.cpp's format.
    ui::invalidate();
    ui::showLines({"SETUP BY PHONE", "1.Join Wi-Fi AI-Calc-5C21", "  password k7wq2mxe", "2.Open the page it shows",
                   "  or go to 192.168.4.1", "AC:back", "15 min left"},
                  st);
    host::save("50_setup_by_phone");
    ui::invalidate();
    ui::showLines({"SETUP BY PHONE", "Saved. Closing the", "setup network...", "", "Next: MODE 4, then =", "", "AC:back"},
                  st);
    host::save("51_setup_by_phone_saved");
    // main.cpp's update step.
    const ui::Status chg = status(71, true, true, true);
    const int pcts[] = {0, 42, 100};
    for (int i = 0; i < 3; ++i) {
      ui::invalidate();
      ui::showProgress("UPDATING", "Keep the cable in.", "Downloading v15lcd-2026.10.20", pcts[i], chg);
      host::save("5" + std::to_string(2 + i) + "_ota_progress_" + std::to_string(pcts[i]));
    }
    ui::invalidate();
    ui::showLines({"UPDATE DONE", "Restarting with", "v15lcd-2026.10.20", "", "", ""}, chg);
    host::save("55_ota_done");
    ui::invalidate();
    ui::showLines({"UPDATE FAILED", "Download stopped (61%)", "", "The old firmware", "keeps running.", "AC:back"}, chg);
    host::save("56_ota_failed");
  }
  // 8. Self-test (selftest.cpp, run whole; every frame saved).
  std::printf("self-test:\n");
  ui::invalidate();
  hostRunSelfTest("60_selftest");
  // 9. The camera viewfinder over the sample photo.
  std::printf("camera:\n");
  {
    vflcd::showStarting("starting...");
    host::save("70_camera_starting");
    calc::Viewfinder vf;
    vf.start(host::nowMs);
    const std::vector<uint8_t> still = cameraFrame();
    uint8_t grey[80 * 60];
    vflcd::greyThumb(still.data(), grey);
    vf.feed(grey, 80, 60, true, host::nowMs);  // the first frame only primes the motion check
    cameraShot(vf, still, false, vflcd::FocusState::Focusing, "Normal", 2000, "71_camera_focusing");
    cameraShot(vf, still, true, vflcd::FocusState::Focused, "Normal", 3000, "72_camera_focused");
    cameraShot(vf, cameraFrame(14, 6), true, vflcd::FocusState::Focused, "Normal", 3500, "73_camera_hold_still");
    vflcd::greyThumb(still.data(), grey);
    vf.feed(grey, 80, 60, true, host::nowMs += 70);  // steady again after the shake
    cameraShot(vf, still, true, vflcd::FocusState::Focused, "Careful +Tutor", 4000, "74_camera_careful_tutor");
    cameraShot(vf, still, true, vflcd::FocusState::Fixed, "Normal", 22000, "75_camera_fixed_focus_timeout", 18);
  }
  std::printf("%d frames pushed\n", host::flushes);
  return 0;
}
