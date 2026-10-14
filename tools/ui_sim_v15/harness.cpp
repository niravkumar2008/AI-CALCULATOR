// UI screenshot harness for the v15 colour LCD. Every picture is drawn by the firmware's
// own code (firmware-v15-lcd/src/ui.cpp, fbtext.cpp, vf_lcd.cpp, selftest.cpp, compiled
// unchanged) into a real LovyanGFX LGFX_Sprite, fed by the real core/ calculator
// (calc::Device, App, Viewfinder). Only the panel is missing: lcd::flush() copies the
// canvas out (host_hw.cpp) and this program saves it as a PPM.
//
//   ui_sim_v15.exe <out_dir> <photo_320x240.ppm>   (build.bat runs it; make_screens.py -> PNG)
#include <chrono>
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
#include "ui_draw.h"
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
  d.setTypedSolveSupported(true);  // as main.cpp: the v15 sends Type it's PNG
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

// ---- MODE 4: the Apps menu and its apps (A* screens), and the LCD look (L* screens)

double fixedRandom() { return 0.37; }

// Moves the on-screen keyboard's cursor to the cell that types `cell` and presses =.
void kbPress(Device& d, const std::string& label) {
  int target = -1;
  for (int i = 0; i < Device::keyboardCellCount(); ++i) {
    const calc::KeyCell& c = Device::keyboardCellAt(i);
    if (label == c.label) target = i;
  }
  if (target < 0) {
    std::fprintf(stderr, "no keyboard cell '%s'\n", label.c_str());
    return;
  }
  const int row = Device::keyboardCellAt(target).row;
  for (int g = 0; g < 4 && Device::keyboardCellAt(d.keyboardCell()).row != row; ++g) key(d, DKey::Down);
  for (int g = 0; g < 30 && d.keyboardCell() != target; ++g) key(d, DKey::Right);
  key(d, DKey::Eq);
}

// The letter grids (Geography's and the Periodic Table's finders): move to a letter, press =.
template <typename Cursor>
void gridPick(Device& d, int target, Cursor cursor) {
  for (int g = 0; g < 40 && cursor() != target; ++g) key(d, DKey::Right);
  key(d, DKey::Eq);
}

// Types text on the keyboard: digits and + - × ÷ ( ) ^ . with the calculator's keys (as a
// user would), everything else (letters, x, =, π, space) by moving to its cell.
void typeOnKeyboard(Device& d, const std::string& utf8) {
  for (uint32_t cp : calc::decodeUtf8(utf8)) {
    DKey k;
    bool direct = true;
    switch (cp) {
      case '0': k = DKey::D0; break;
      case '1': k = DKey::D1; break;
      case '2': k = DKey::D2; break;
      case '3': k = DKey::D3; break;
      case '4': k = DKey::D4; break;
      case '5': k = DKey::D5; break;
      case '6': k = DKey::D6; break;
      case '7': k = DKey::D7; break;
      case '8': k = DKey::D8; break;
      case '9': k = DKey::D9; break;
      case '+': k = DKey::Add; break;
      case '-': k = DKey::Sub; break;
      case 0xD7: k = DKey::Mul; break;
      case 0xF7: k = DKey::Div; break;
      case '(': k = DKey::Open; break;
      case ')': k = DKey::Close; break;
      case '^': k = DKey::Pow; break;
      case '.': k = DKey::Dot; break;
      default: direct = false; k = DKey::Eq; break;
    }
    if (direct) {
      key(d, k);
    } else {
      std::vector<uint32_t> one = {cp};
      kbPress(d, cp == ' ' ? "SPACE" : calc::encodeUtf8(one));
    }
  }
}

// An AI answer arriving for a request (photo or typed).
void answer(Device& d, int id, const char* readAs, const char* ans, const char* steps) {
  d.onCaptured(id);
  d.onReply(id, std::string(R"({"readable":true,"confidence":0.97,"unclear":[],"read_as":")") + readAs +
                    R"(","expression":"","choice":"","answer":")" + ans + R"(","check":"","steps":[)" + steps + "]}");
}

void runApps() {
  std::printf("apps:\n");
  const ui::Status st = status(78, false, true, true);
  // 1. The carousel, all seven apps.
  {
    Device d;
    setup(d);
    type(d, "m4");
    for (int i = 0; i < Device::kAppCount; ++i) {
      shot(d, st, "A0" + std::to_string(i + 1) + "_apps_" + std::to_string(i + 1));
      type(d, ">");
    }
  }
  // 2. Type it: empty, typing, SEND, the typed solve, its answer; then History.
  {
    Device d;
    setup(d);
    type(d, "m4>=");
    shot(d, st, "A10_type_it_empty");
    typeOnKeyboard(d, "SOLVE 3x+7=22 FOR x");
    shot(d, st, "A11_type_it_text");
    kbPress(d, "SEND");  // moves there; = pressed: sends
    // (kbPress pressed =, which sent it: show the keyboard state just before with a fresh one)
    const int id = d.takeRequest();
    std::string png;
    const bool typed = d.takeTypedImage(png);
    std::printf("    typed solve: request %d, PNG %s (%u bytes)\n", id, typed ? "yes" : "NO", unsigned(png.size()));
    shot(d, st, "A13_type_it_solving");
    answer(d, id, "Solve 3x + 7 = 22 for x", "x = 5",
           R"("Subtract 7 from both sides: 3x = 15.","Divide by 3: x = 5.")");
    shot(d, st, "A14_type_it_answer");
    // two more answers for the History list (photo scans)
    for (const char* a : {"x = 4 or x = -1", "12.5 cm\xC2\xB2"}) {
      if (!d.startScan()) std::printf("    startScan failed\n");
      const int sid = d.takeRequest();
      answer(d, sid, "a problem", a, R"("Step one.")");
    }
    type(d, "$");  // AC: back from AI Solve
    // History
    type(d, "$m4");
    while (d.appsSelected() != 2) type(d, ">");
    type(d, "=");
    shot(d, st, "A20_history_list");
    type(d, "d=");
    shot(d, st, "A21_history_answer");
    type(d, "$");
    type(d, "dd");  // to "Clear history"
    shot(d, st, "A22_history_clear_row");
    type(d, "=");
    shot(d, st, "A23_confirm_clear_history");
    type(d, "=");
    shot(d, st, "A24_toast_history_cleared");
    host::nowMs += 2000;
    shot(d, st, "A25_history_empty");
  }
  {  // the keyboard with its cursor on SEND (and SAVE for a note)
    Device d;
    setup(d);
    type(d, "m4>=");
    typeOnKeyboard(d, "2^10-1");
    for (int g = 0; g < 4 && Device::keyboardCellAt(d.keyboardCell()).row != 2; ++g) key(d, DKey::Down);
    for (int g = 0; g < 10 && Device::keyboardCellAt(d.keyboardCell()).kind != calc::KeyCell::Send; ++g)
      key(d, DKey::Right);
    shot(d, st, "A12_type_it_send");
    type(d, "$");
    shot(d, st, "A15_confirm_discard");
  }
  // 3. Notes: new note, saved toast, list, view, delete confirm.
  {
    Device d;
    setup(d);
    type(d, "m4");
    while (d.appsSelected() != 3) type(d, ">");
    type(d, "=");
    shot(d, st, "A30_notes_empty");
    type(d, "=");
    typeOnKeyboard(d, "QUIZ FRIDAY CH 4 AND 5");
    shot(d, st, "A31_note_keyboard");
    kbPress(d, "SEND");
    shot(d, st, "A32_toast_saved");
    type(d, "u=");  // up to "+ New note"
    typeOnKeyboard(d, "AREA = \xCF\x80\xC3\x97R^2, CIRCUMFERENCE = 2\xCF\x80R, VOLUME OF A SPHERE = 4\xC3\xB7" "3\xCF\x80R^3");
    kbPress(d, "SEND");
    host::nowMs += 2000;
    shot(d, st, "A33_notes_list");
    type(d, "=");
    shot(d, st, "A34_note_view");
    type(d, "2");
    shot(d, st, "A35_confirm_delete_note");
  }
  // 4. Settings, Link, About.
  {
    Device d;
    setup(d);
    d.setVersion("v15lcd-2026.10.11");
    d.setBrightness(70);
    type(d, "m4");
    while (d.appsSelected() != 6) type(d, ">");
    type(d, "=");
    shot(d, st, "A40_settings");
    type(d, "d");
    shot(d, st, "A41_settings_sleep");
    type(d, "dddddd");
    shot(d, st, "A42_settings_scrolled");
    type(d, "=");
    shot(d, st, "A43_settings_about");
    type(d, "$");
    d.setLinkInfo("K7Q2MX", "https://proofline.app/", false);
    type(d, "4");
    shot(d, st, "A44_link_code");
    d.setLinkInfo("", "https://proofline.app/", false);
    shot(d, status(78), "A45_link_no_wifi");
    d.setLinkInfo("", "https://proofline.app/", true);
    shot(d, st, "A46_link_linked");
    d.setLinkPlan("Free month over: subscribe at proofline.app");
    shot(d, st, "A47_link_plan");
    d.setLinkPlan("");
  }
  // 5. Geography.
  {
    Device d;
    setup(d);
    d.setRandomSource(fixedRandom);
    type(d, "m4");
    while (d.appsSelected() != 4) type(d, ">");
    type(d, "=");
    shot(d, st, "A50_geo_world");
    {  // how long the map takes to draw (PC; see PORT_NOTES for the ESP32 estimate)
      host::nowMs = (host::nowMs / 1000 + 1) * 1000;
      d.tick(host::nowMs);
      d.render(g_fb);
      const ui::Frame f{d, st, host::nowMs, false, false};
      const auto t0 = std::chrono::steady_clock::now();
      const int n = 200;
      for (int i = 0; i < n; ++i) ui::drawGeo(f);
      const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / n;
      std::printf("    geo world map: %.2f ms per frame on this PC (%d frames)\n", ms, n);
    }
    type(d, "1");
    shot(d, st, "A51_geo_continent");
    type(d, "1");
    shot(d, st, "A52_geo_country");
    type(d, "0");
    // Japan: the letter picker (3), J, A, P, then OK opens the list there; = shows it.
    type(d, "3");
    for (char c : std::string("JAP")) gridPick(d, c - 'A', [&] { return d.geo().searchCursor(); });
    shot(d, st, "A56_geo_find");
    gridPick(d, 26, [&] { return d.geo().searchCursor(); });
    shot(d, st, "A55_geo_list");
    type(d, "=");
    shot(d, st, "A53_geo_japan");
    type(d, "=");
    shot(d, st, "A54_geo_card");
    type(d, "$");
    type(d, "1");
    shot(d, st, "A57_geo_japan_zoom");
    type(d, "0");
    type(d, "4");  // filter: Africa
    shot(d, st, "A58_geo_filter_africa");
    type(d, "444444");  // back to All
    type(d, "5");
    shot(d, st, "A59_geo_quiz");
    const int right = d.geo().quiz().correct;
    type(d, std::to_string(right + 1).c_str());
    shot(d, st, "A60_geo_quiz_right");
    type(d, "=");
    type(d, std::to_string((d.geo().quiz().correct + 1) % 4 + 1).c_str());
    shot(d, st, "A61_geo_quiz_wrong");
  }
  // 6. Periodic Table.
  {
    Device d;
    setup(d);
    type(d, "m4");
    while (d.appsSelected() != 5) type(d, ">");
    type(d, "=");
    shot(d, st, "A70_periodic_hydrogen");
    type(d, "326=");  // find by number: 26 opens its details
    shot(d, st, "A73_periodic_detail");
    type(d, "dddd");
    shot(d, st, "A74_periodic_detail_scrolled");
    type(d, "$");
    shot(d, st, "A71_periodic_iron");
    type(d, "444");  // filter: alkali metals, alkaline earth, transition metals
    shot(d, st, "A72_periodic_filter");
    type(d, "[4[4[4");  // SHIFT 4: back to All
    type(d, "1");
    shot(d, st, "A75_periodic_list");
    type(d, "$3");
    gridPick(d, 'F' - 'A', [&] { return d.periodic().searchCell(); });
    shot(d, st, "A76_periodic_find");
    type(d, "$5");
    shot(d, st, "A77_periodic_graph");
    type(d, "u");
    shot(d, st, "A78_periodic_graph_mass");
    type(d, "$0");
    shot(d, st, "A79_periodic_key");
  }
  // 7. Exam mode: MODE shows the apps are off, MODE 4 refuses (LCD look).
  {
    Device d;
    setup(d);
    type(d, "[$[7o");
    type(d, "m");
    shot(d, status(92), "A80_exam_mode_menu");
    type(d, "4");
    shot(d, status(92), "A81_exam_apps_off");
  }
  // 8. The calculator's LCD look: plain (no dots) and the SETUP > Screen look page.
  {
    Device d;
    setup(d);
    type(d, "[m7");
    shot(d, status(78), "L01_screen_look");
    type(d, "1$$");  // Plain, back to the calculator
    type(d, "2*(3+4)q/7+r16=");
    shot(d, status(78), "L02_calc_result_plain");
    type(d, "[m72$$");  // LCD dots again
    type(d, "$2^3q=");
    shot(d, status(78), "L03_calc_pow_square");
  }
}

// ---- fx-115ES modes and tools (M* screens, the LCD look): STAT, TABLE, CALC, SOLVE, ∫, d/dx.
// Keys as keysForChar: m MODE, [ SHIFT, X / Y the variables, q x², C CALC, S SOLVE,
// Q the "=" of an equation, I ∫dx, D d/dx, , comma, u d < > arrows, $ AC.
void runModes() {
  std::printf("modes:\n");
  const ui::Status st = status(78);
  {  // STAT, 1-VAR: the type list, the editor, the STAT menu, x̄ =
    Device d;
    setup(d);
    type(d, "m2");
    shot(d, st, "M01_stat_type");
    type(d, "12=4=4=4=5=5=7=9=");
    shot(d, st, "M02_stat_editor");
    type(d, "$[1");
    shot(d, st, "M03_stat_menu");
    type(d, "42=");
    shot(d, st, "M04_stat_result");
    std::printf("    STAT 1-VAR mean: %s\n", d.stat().resultText().c_str());
  }
  {  // STAT, A+BX: two columns, then the regression's B
    Device d;
    setup(d);
    type(d, "m22");
    type(d, "1=2=3=4=5=");
    type(d, ">uuuuu");
    type(d, "2.1=3.9=6.2=7.8=10.1=");
    type(d, "uu");
    shot(d, st, "M05_stat_editor_xy");
    type(d, "$[152=");
    shot(d, st, "M06_stat_reg_b");
    std::printf("    STAT A+BX B: %s\n", d.stat().resultText().c_str());
  }
  {  // SETUP ▼: the second page and STAT's Frequency?
    Device d;
    setup(d);
    type(d, "[m");
    shot(d, st, "M07_setup_page1");
    type(d, "d");
    shot(d, st, "M08_setup_page2");
    type(d, "1");
    shot(d, st, "M09_setup_frequency");
  }
  {  // TABLE: f(X)=X²+1, Start 1 End 5 Step 1
    Device d;
    setup(d);
    type(d, "m3Xq+1");
    shot(d, st, "M10_table_function");
    type(d, "=");
    shot(d, st, "M11_table_start");
    type(d, "===");
    shot(d, st, "M12_table");
    type(d, "dddd>");
    shot(d, st, "M13_table_scrolled");
  }
  {  // CALC: X²+1, X? 3 =
    Device d;
    setup(d);
    type(d, "Xq+1C");
    shot(d, st, "M20_calc_prompt");
    type(d, "3");
    shot(d, st, "M21_calc_prompt_typing");
    type(d, "=");
    shot(d, st, "M22_calc_result");
  }
  {  // SOLVE: X²−2=0 from X? 1
    Device d;
    setup(d);
    type(d, "Xq-2Q0S");
    shot(d, st, "M30_solve_prompt");
    type(d, "1=");
    shot(d, st, "M31_solve_result");
    std::printf("    SOLVE X^2-2=0: X = %.10g\n", d.solveRunner().root());
    type(d, "$$Xq+1S=");
    shot(d, st, "M32_solve_cant");
  }
  {  // ∫dx and d/dx
    Device d;
    setup(d);
    type(d, "IXq,0,3)=");
    shot(d, st, "M40_integral_result");
    type(d, "$DX[q,2=");
    shot(d, st, "M41_derivative_result");
    type(d, "$I1/X,1,2");
    shot(d, st, "M42_integral_typing");
  }
  {  // Exam mode: STAT works (the STAT indicator and the exam clock share the row)
    Device d;
    setup(d);
    type(d, "[$[7o$m213=5=");
    shot(d, status(92), "M50_exam_stat");
    type(d, "$[142=");
    shot(d, status(92), "M51_exam_stat_result");
  }
}

// ---- MODE 5 EQN (E* screens, the LCD look): type list, coefficient editors, results
// (real, complex a+bi big and on two rows, the quadratic's vertex), the singular messages.
// ~ is the (−) key.
void runEqn() {
  std::printf("eqn:\n");
  const ui::Status st = status(78);
  {
    Device d;
    setup(d);
    type(d, "m");
    shot(d, st, "E01_mode_menu");
    type(d, "5");
    shot(d, st, "E02_eqn_type");
    type(d, "12=3=8=1=");
    shot(d, st, "E03_lin2_editor");
    type(d, "~4");
    shot(d, st, "E04_lin2_typing");
    type(d, "=~7==");
    shot(d, st, "E05_lin2_x");
    type(d, "d");
    shot(d, st, "E06_lin2_y");
    std::printf("    2X+3Y=8, X-4Y=-7: X=%s Y=%s\n", d.eqn().resultText(0).c_str(), d.eqn().resultText(1).c_str());
    type(d, "$$2");
    type(d, "2=1=~1=8=~3=~1=2=~11=~2=1=2=~3=");
    shot(d, st, "E07_lin3_editor");
    type(d, "=dd");
    shot(d, st, "E08_lin3_z");
    type(d, "$$1");
    type(d, "1=1=1=2=2=3==");
    shot(d, st, "E09_no_solution");
    type(d, "$<<2=2=2==");
    shot(d, st, "E10_infinite_solution");
  }
  {
    Device d;
    setup(d);
    type(d, "m53");
    type(d, "1=2=5=");
    shot(d, st, "E11_quad_editor");
    type(d, "=");
    shot(d, st, "E12_quad_complex_x1");
    type(d, "d");
    shot(d, st, "E13_quad_complex_x2");
    type(d, "$$32=~3=1==");
    shot(d, st, "E14_quad_x1");
    type(d, "d");
    shot(d, st, "E15_quad_x2_fraction");
    type(d, "d");
    shot(d, st, "E16_quad_xvalue_min");
    type(d, "d");
    shot(d, st, "E17_quad_yvalue_min");
    type(d, "$$4");
    type(d, "3=0=0=1=");
    shot(d, st, "E18_cubic_editor");
    type(d, "=");
    shot(d, st, "E19_cubic_x1");
    type(d, "d");
    shot(d, st, "E20_cubic_complex_long");
    std::printf("    3X^3+1=0: X1=%s X2=%s\n", d.eqn().resultText(0).c_str(), d.eqn().resultText(1).c_str());
    type(d, "$$4");
    type(d, "1=~6=12=~8==");
    shot(d, st, "E21_cubic_triple_root");
    type(d, "$$30==");
    shot(d, st, "E22_math_error");
  }
  {  // exam mode: EQN works
    Device d;
    setup(d);
    type(d, "[$[7o$m");
    shot(d, status(92), "E30_exam_mode_menu");
    type(d, "531=~3=2==");
    shot(d, status(92), "E31_exam_eqn_result");
  }
}

// ---- MODE 6 CMPLX (C* screens, the LCD look): the menu, a+bi results, SHIFT 2's menu, polar
// entry and r∠θ, SETUP ▼ 2:CMPLX, a long value split with R⇔I (SHIFT = swaps), exam mode.
// j is the ENG key (i in CMPLX), @ SHIFT (−) (∠), ~ the (−) key.
void runCmplx() {
  std::printf("cmplx:\n");
  const ui::Status st = status(78);
  {
    Device d;
    setup(d);
    type(d, "m");
    shot(d, st, "C01_mode_menu");
    type(d, "6");
    shot(d, st, "C02_cmplx_home");
    type(d, "(3+4j)(1-2j)");
    shot(d, st, "C03_typing");
    type(d, "=");
    shot(d, st, "C04_result");
    std::printf("    (3+4i)(1-2i) = %s\n", d.resultText().c_str());
    type(d, "$[2");
    shot(d, st, "C05_shift2_menu");
    type(d, "$3+4j[23=");  // AC closes the menu; 3+4i▶r∠θ
    shot(d, st, "C06_polar_result");
    type(d, "$2@30=");
    shot(d, st, "C07_polar_entry");
    type(d, "$[2");
    type(d, "1j)=");  // arg(i)
    shot(d, st, "C08_arg");
    type(d, "$1/(1+j)=");
    shot(d, st, "C09_fraction_parts");
    type(d, "$r~1=");
    shot(d, st, "C10_sqrt_neg_math_error");
  }
  {
    Device d;
    setup(d);
    type(d, "[md");
    shot(d, st, "C11_setup_page2");
    type(d, "2");
    shot(d, st, "C12_setup_cmplx");
    type(d, "1m6");
    type(d, "1E~5/3-2j/3=w");  // S⇔D: 3.333333333×10⁻⁶−0.6666666667i, too long for a row
    shot(d, st, "C13_split_real");
    type(d, "[=");
    shot(d, st, "C14_split_imag");
    std::printf("    split: complexSplit=%d imag shown=%d \"%s\"\n", d.complexSplit(), d.complexImagShown(),
                d.resultText().c_str());
  }
  {  // exam mode: CMPLX works
    Device d;
    setup(d);
    type(d, "[$[7o$m6");
    type(d, "(1+j)q=");
    shot(d, status(92), "C20_exam_cmplx_result");
  }
}

// ---- MODE 7 BASE-N (B* screens, the LCD look): the menu, DEC, the same result in HEX / BIN /
// OCT, A-F, SHIFT 3's two pages, logic in BIN, prefixes, errors, exam mode.
// ^ x^□ (HEX), l log (BIN), n ln (OCT), q x² (DEC); ~ " h s c t are A-F; [3 SHIFT 3.
void runBaseN() {
  std::printf("basen:\n");
  const ui::Status st = status(78);
  {
    Device d;
    setup(d);
    type(d, "m");
    shot(d, st, "B01_mode_menu");
    type(d, "7");
    shot(d, st, "B02_basen_home");
    type(d, "15*37");
    shot(d, st, "B03_typing");
    type(d, "=");
    shot(d, st, "B04_dec_result");
    type(d, "^");
    shot(d, st, "B05_hex_result");
    type(d, "l");
    shot(d, st, "B06_bin_result");
    type(d, "n");
    shot(d, st, "B07_oct_result");
    std::printf("    555 = %s (OCT)\n", d.resultText().c_str());
    type(d, "^~\"hsct");  // HEX, then A B C D E F
    shot(d, st, "B08_hex_digits");
    type(d, "=$[3");
    shot(d, st, "B09_base_menu");
    type(d, "d");
    shot(d, st, "B10_base_menu_prefixes");
    type(d, "$l1111[341010=");  // BIN: 1111 xor 1010
    shot(d, st, "B11_bin_xor");
    type(d, "[36101101=");  // Neg(101101
    shot(d, st, "B12_bin_neg");
    type(d, "q[3d110+[3d210+[3d310+[3d410=");  // DEC: d10+h10+b10+o10
    shot(d, st, "B13_dec_prefixes");
    std::printf("    d10+h10+b10+o10 = %s\n", d.resultText().c_str());
    type(d, "100000=l");  // can't show in BIN's 16 bits
    shot(d, st, "B14_bin_range_math_error");
    type(d, "$12=");
    shot(d, st, "B15_bin_digit_syntax_error");
  }
  {  // exam mode: BASE-N works
    Device d;
    setup(d);
    type(d, "[$[7o$m7");
    type(d, "^tt+1=");  // HEX FF+1
    shot(d, status(92), "B20_exam_basen_result");
  }
}

// ---- MODE 8 MATRIX / MODE 9 VECTOR (X* screens, the LCD look): the MODE menu, Dim's lists, the
// editors, SHIFT 4 / SHIFT 5's menus, the MatAns / VctAns screens, det, inverse, Dim ERROR, the dot
// product, Abs, exam mode. [4 SHIFT 4 (MATRIX), [5 SHIFT 5 (VECTOR), i x⁻¹, b Abs, ~ (−).
void runMatVec() {
  std::printf("matrix / vector:\n");
  const ui::Status st = status(78);
  {
    Device d;
    setup(d);
    type(d, "m");
    shot(d, st, "X01_mode_menu");
    type(d, "8");
    shot(d, st, "X02_matrix_select");
    type(d, "1");
    shot(d, st, "X03_matrix_dim");
    type(d, "d");
    shot(d, st, "X04_matrix_dim_page2");
    type(d, "u5");
    shot(d, st, "X05_matrix_editor_empty");
    type(d, "2=1=1=1");  // the guide's MatA, the last element still being typed
    shot(d, st, "X06_matrix_editor_typing");
    type(d, "=$[4");
    shot(d, st, "X07_matrix_menu");
    type(d, "22");  // Data, MatB (no size yet): its sizes first
    type(d, "52=~1=~1=2=$");
    type(d, "[43*[44");
    shot(d, st, "X08_calc_matA_x_matB");
    type(d, "=");
    shot(d, st, "X09_matans_screen");
    type(d, "$");
    shot(d, st, "X10_calc_after_matans");
    type(d, "[47[43=");
    shot(d, st, "X11_det_result");
    std::printf("    det(MatA) = %s\n", d.resultText().c_str());
    type(d, "[4111");  // Dim MatA 3x3
    type(d, "2=~1=0=1=3=2=0=1=4=$");
    type(d, "[43i=");
    shot(d, st, "X12_inverse_3x3_fractions");
    type(d, "$[4114");  // Dim MatA 2x3
    type(d, "$[43+[44=");
    shot(d, st, "X13_dim_error");
    type(d, "$[4114");
    type(d, "1/3=");
    shot(d, st, "X14_editor_fraction_cell");
  }
  {
    Device d;
    setup(d);
    type(d, "m9");
    shot(d, st, "X15_vector_select");
    type(d, "1");
    shot(d, st, "X16_vector_dim");
    type(d, "21=2=$[51223=4=$[5");
    shot(d, st, "X17_vector_menu");
    type(d, "3[57[54");
    shot(d, st, "X18_vector_dot_typing");
    type(d, "=");
    shot(d, st, "X19_vector_dot_result");
    type(d, "$[5111");  // Dim VctA 3
    type(d, "1=2=3=$[5121");  // Dim VctB 3
    type(d, "4=5=6=$[53*[54=");
    shot(d, st, "X20_vector_cross_vctans");
    type(d, "$b[53=");  // Abs(VctA) = √14
    shot(d, st, "X21_vector_abs");
  }
  {  // exam mode: MATRIX works
    Device d;
    setup(d);
    type(d, "[$[7o$m8");
    type(d, "15");
    type(d, "1=2=3=4=$[47[43=");  // det(MatA)
    shot(d, status(92), "X22_exam_matrix_det");
  }
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
  runModes();  // STAT, TABLE, CALC, SOLVE, ∫, d/dx (M* screens)
  runEqn();    // MODE 5 EQN (E* screens)
  runCmplx();  // MODE 6 CMPLX (C* screens)
  runBaseN();  // MODE 7 BASE-N (B* screens)
  runMatVec();  // MODE 8 MATRIX / MODE 9 VECTOR (X* screens)
  // 3. AI SOLVE: ready, hold still, reading, solving, answer pages.
  std::printf("ai solve:\n");
  {
    Device d;
    setup(d);
    const ui::Status wifi = status(78, false, true, true);
    type(d, "m4=");  // MODE 4 = Apps, = opens AI Solve (app 1)
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

    // Failures: the AI busy, then too slow (the texts name no AI provider). AC leaves the
    // answer for the calculator (its expression); MODE 4 = is AI Solve again, = scans.
    type(d, "$m4==");
    d.onFailure(d.takeRequest(), calc::Failure::ApiBusy);
    shot(d, wifi, "27_ai_failure_busy");
    type(d, "=");
    d.onFailure(d.takeRequest(), calc::Failure::Timeout);
    shot(d, wifi, "28_ai_failure_timeout");
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
    type(d, "m4=");  // MODE 4 = Apps, = opens AI Solve (app 1)
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
    ui::showLines({"SETUP BY PHONE", "Saved. Closing the", "setup network...", "", "Next: MODE 4, AI SOLVE", "", "AC:back"},
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
    // main.cpp's cold-start loading screen, and the core's splash after ON (via showDevice).
    ui::invalidate();
    ui::showSplash(45, "Loading apps");
    host::save("57_splash_boot");
    Device d;
    setup(d);
    d.setSplash(true);
    d.tick(host::nowMs);
    d.onKey(DKey::Shift);
    d.onKey(DKey::AC);  // off
    d.onKey(DKey::On);
    host::nowMs += Device::kOnSplashMs / 2;
    d.tick(host::nowMs);
    d.render(g_fb);
    ui::showDevice(d, g_fb, normal, host::nowMs, true);
    host::save("58_splash_after_on");
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
  runApps();
  std::printf("%d frames pushed\n", host::flushes);
  return 0;
}
