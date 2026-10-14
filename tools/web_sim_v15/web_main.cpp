// WebAssembly bridge for the website's v15 demo: the Board (board.cpp: main.cpp's loop
// around the real core and the real v15 screen code) driven from the page's JavaScript.
// Single-threaded: the page calls sim_step() on every animation frame.
// Build: tools/web_sim_v15/build.bat (Emscripten).
#include <emscripten/emscripten.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "board.h"
#include "calc_engine.h"
#include "sim_host.h"

using namespace calc;

// History and notes, kept in this visitor's browser (localStorage). Every access is wrapped:
// private windows and blocked storage throw, and the demo works without it.
EM_JS(void, js_save_apps, (const uint8_t* p, int n), {
  try {
    var s = "";
    for (var i = 0; i < n; i++) s += String.fromCharCode(HEAPU8[p + i]);
    window.localStorage.setItem("proofline.v15.apps", btoa(s));
  } catch (e) {}
});
EM_JS(int, js_load_apps_len, (), {
  try {
    var v = window.localStorage.getItem("proofline.v15.apps");
    if (!v) return 0;
    Module.__apps = atob(v);
    return Module.__apps.length;
  } catch (e) {
    return 0;
  }
});
EM_JS(void, js_load_apps_copy, (uint8_t* p), {
  var s = Module.__apps || "";
  for (var i = 0; i < s.length; i++) HEAPU8[p + i] = s.charCodeAt(i) & 255;
  Module.__apps = null;
});

namespace {
Board g_board;
uint8_t g_rgba[simhost::kW * simhost::kH * 4];
uint32_t g_shownFlushes = ~0u;
int g_shownLevel = -1;
std::string g_str;
std::vector<uint8_t> g_photo;

std::string loadApps() {
  const int n = js_load_apps_len();
  if (n <= 0 || n > 256 * 1024) return "";
  std::string s(size_t(n), '\0');
  js_load_apps_copy(reinterpret_cast<uint8_t*>(&s[0]));
  return s;
}
}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void sim_init() {
  g_board.onSaveApps = [](const std::string& blob) {
    js_save_apps(reinterpret_cast<const uint8_t*>(blob.data()), int(blob.size()));
  };
  g_board.boot(loadApps());
  g_board.step();
}

// One pass of the firmware's loop. Returns 1 when the screen changed (sim_rgba() is new).
EMSCRIPTEN_KEEPALIVE int sim_step() {
  g_board.step();
  const int level = simhost::lightLevel();
  if (simhost::flushes == g_shownFlushes && level == g_shownLevel) return 0;
  g_shownFlushes = simhost::flushes;
  g_shownLevel = level;
  // RGB565 -> RGBA, dimmed by the backlight (10 % still readable, 0 = dark panel).
  const int k = level <= 0 ? 0 : 300 + 7 * level;
  for (int i = 0; i < simhost::kW * simhost::kH; ++i) {
    const uint16_t v = simhost::frame[i];
    int r = (v >> 11) & 0x1F, g = (v >> 5) & 0x3F, b = v & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    uint8_t* o = &g_rgba[i * 4];
    o[0] = uint8_t(r * k / 1000);
    o[1] = uint8_t(g * k / 1000);
    o[2] = uint8_t(b * k / 1000);
    o[3] = 255;
  }
  return 1;
}

EMSCRIPTEN_KEEPALIVE const uint8_t* sim_rgba() { return g_rgba; }
EMSCRIPTEN_KEEPALIVE int sim_width() { return simhost::kW; }
EMSCRIPTEN_KEEPALIVE int sim_height() { return simhost::kH; }

EMSCRIPTEN_KEEPALIVE void sim_key(int k) {
  if (k >= 0 && k < int(DKey::Count)) g_board.press(DKey(k));
}
EMSCRIPTEN_KEEPALIVE int sim_type_char(int c) { return c > 0 && c < 128 && g_board.typeChar(char(c)) ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE void sim_cursor_to_send() { g_board.cursorToSend(); }
// 1 when the on-screen keyboard (Type it / a note) is open.
EMSCRIPTEN_KEEPALIVE int sim_on_keyboard() {
  const Device& d = g_board.dev();
  return !d.isOff() && d.view() == View::Keyboard ? 1 : 0;
}

// The camera's picture: 320 x 240 RGB888 copied into the buffer sim_photo_buffer() gives.
EMSCRIPTEN_KEEPALIVE uint8_t* sim_photo_buffer() {
  g_photo.assign(320 * 240 * 3, 0);
  return g_photo.data();
}
EMSCRIPTEN_KEEPALIVE void sim_photo_set() { g_board.setPhoto(g_photo); }

// A short description of the screen for screen readers.
EMSCRIPTEN_KEEPALIVE const char* sim_summary() {
  const Device& d = g_board.dev();
  if (d.isOff()) {
    g_str = "Calculator screen: off";
    return g_str.c_str();
  }
  switch (d.view()) {
    case View::Calc: {
      const std::string expr = exprText(d.expression()), res = d.resultText();
      g_str = "Calculator screen: " + (expr.empty() ? std::string("ready") : expr);
      if (!res.empty()) g_str += " = " + res;
      break;
    }
    case View::Error: g_str = "Calculator screen: " + d.errorMessage(); break;
    case View::ModeMenu: g_str = "Calculator screen: MODE menu"; break;
    case View::Setup: g_str = "Calculator screen: SETUP menu"; break;
    case View::Apps: g_str = std::string("Apps menu: ") + Device::appName(d.appsSelected()); break;
    case View::Keyboard:
      g_str = std::string(d.keyboardForNote() ? "Note: " : "Type it: ") +
              (d.keyboardText().empty() ? std::string("(empty)") : d.keyboardText());
      break;
    case View::History: g_str = "History"; break;
    case View::Notes:
    case View::NoteView: g_str = "Notes"; break;
    case View::Settings: g_str = std::string("Settings: ") + Device::settingsItem(d.settingsSelected()); break;
    case View::Geo: g_str = "Geography"; break;
    case View::Periodic: g_str = "Periodic Table"; break;
    case View::Link: g_str = "Link calculator: code " + d.linkCode(); break;
    case View::Ai: g_str = "AI Solve"; break;
    default: g_str = "Calculator screen"; break;
  }
  return g_str.c_str();
}

}  // extern "C"
