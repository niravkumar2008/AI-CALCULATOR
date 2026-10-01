// WebAssembly bridge for the browser simulator: the same Device the firmware
// runs, driven from JavaScript. Build with sim/web/build.sh (Emscripten).
#include <emscripten/emscripten.h>

#include <cstdlib>
#include <string>

#include "../../core/claude_api.h"
#include "../../core/device.h"

using namespace calc;

namespace {
Device g_device;
Framebuffer g_fb;
std::string g_str;  // backing store for strings returned to JS

std::string jsonEscape(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += c;
    else if (c == '\n') o += "\\n";
    else if (static_cast<unsigned char>(c) < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

std::string numText(const Num& n) { return jsonEscape(formatResult(n, true, NormMode::Norm1)); }
}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void sim_init(double seed) {
  (void)seed;
  g_device.setBlinkingCursor(true);
  g_device.setHasApiKey(true);
  g_device.setNoKeyHelp("Allow Claude for this page (it asks on the first scan).");
  g_device.setNetInfo("Browser", "via your Claude account", "Simulator: nothing to set.");
}

EMSCRIPTEN_KEEPALIVE void sim_key(int k) {
  if (k >= 0 && k < static_cast<int>(DKey::Count)) g_device.onKey(static_cast<DKey>(k));
}
EMSCRIPTEN_KEEPALIVE void sim_tick(double ms) { g_device.tick(static_cast<uint32_t>(ms)); }

// 125x61 bytes, 1 = dark pixel.
EMSCRIPTEN_KEEPALIVE const uint8_t* sim_render() {
  g_device.render(g_fb);
  return g_fb.pixels();
}
EMSCRIPTEN_KEEPALIVE int sim_width() { return Framebuffer::kWidth; }
EMSCRIPTEN_KEEPALIVE int sim_height() { return Framebuffer::kHeight; }

EMSCRIPTEN_KEEPALIVE int sim_take_request() { return g_device.takeRequest(); }
EMSCRIPTEN_KEEPALIVE int sim_active_request() { return g_device.activeRequest(); }
EMSCRIPTEN_KEEPALIVE void sim_on_captured(int id) { g_device.onCaptured(id); }
EMSCRIPTEN_KEEPALIVE void sim_on_reply(int id, const char* json) { g_device.onReply(id, json); }
EMSCRIPTEN_KEEPALIVE void sim_on_failure(int id, int failure, const char* detail) {
  if (failure < 0 || failure > static_cast<int>(Failure::Camera)) failure = static_cast<int>(Failure::ApiError);
  g_device.onFailure(id, static_cast<Failure>(failure), detail);
}
// Feeds the reply text so far; shows the answer as soon as it has arrived.
EMSCRIPTEN_KEEPALIVE void sim_on_partial(int id, const char* partialJson) {
  double confidence;
  std::string answer;
  if (peekAnswer(partialJson, confidence, answer)) g_device.onPartialAnswer(id, confidence, answer);
}

EMSCRIPTEN_KEEPALIVE void sim_set_online(int on) { g_device.setOnline(on != 0); }
EMSCRIPTEN_KEEPALIVE void sim_set_has_key(int has) { g_device.setHasApiKey(has != 0); }
EMSCRIPTEN_KEEPALIVE void sim_usb_unlock() { g_device.usbUnlock(); }
EMSCRIPTEN_KEEPALIVE int sim_exam_active() { return g_device.examActive(); }
EMSCRIPTEN_KEEPALIVE int sim_dot_matrix() { return g_device.dotMatrix(); }
EMSCRIPTEN_KEEPALIVE int sim_is_off() { return g_device.isOff(); }

EMSCRIPTEN_KEEPALIVE const char* sim_instructions() { return solveInstructions(); }
EMSCRIPTEN_KEEPALIVE const char* sim_schema() { return solveSchema(); }
EMSCRIPTEN_KEEPALIVE const char* sim_key_name(int k) {
  return k >= 0 && k < static_cast<int>(DKey::Count) ? dkeyName(static_cast<DKey>(k)) : "?";
}

// Everything the test bench shows beside the calculator.
EMSCRIPTEN_KEEPALIVE const char* sim_state() {
  static const char* views[] = {"Off", "Calc", "Error", "ModeMenu", "Setup", "Hyp", "Clr", "ClrConfirm",
                                "NetInfo", "Look", "ExamInfo", "Notice", "Ai"};
  const Vars& v = g_device.vars();
  const Device& d = g_device;
  g_str = "{\"mode\":\"" + std::string(d.mode() == Mode::Ai ? "AI SOLVE" : "COMP") + "\"," +
          "\"view\":\"" + views[static_cast<int>(d.view())] + "\"," +
          "\"angle\":\"" + (d.angleUnit() == AngleUnit::Deg ? "Deg" : d.angleUnit() == AngleUnit::Rad ? "Rad" : "Gra") + "\"," +
          "\"norm\":\"" + (d.norm() == NormMode::Norm1 ? "Norm1" : "Norm2") + "\"," +
          "\"expr\":\"" + jsonEscape(exprText(d.expression())) + "\"," +
          "\"result\":\"" + jsonEscape(d.resultText()) + "\"," +
          "\"error\":\"" + jsonEscape(d.errorMessage()) + "\"," +
          "\"exam\":" + (d.examActive() ? "true" : "false") + "," +
          "\"examMs\":" + std::to_string(d.examElapsedMs()) + "," +
          "\"vars\":{\"Ans\":\"" + numText(v.ans) + "\",\"A\":\"" + numText(v.a) + "\",\"B\":\"" + numText(v.b) +
          "\",\"C\":\"" + numText(v.c) + "\",\"D\":\"" + numText(v.d) + "\",\"E\":\"" + numText(v.e) +
          "\",\"F\":\"" + numText(v.f) + "\",\"X\":\"" + numText(v.x) + "\",\"Y\":\"" + numText(v.y) +
          "\",\"M\":\"" + numText(v.m) + "\"}}";
  return g_str.c_str();
}

// ---- Direct API path: the exact request, stream reader and error mapping the
// ESP32 uses, so a photo scan here behaves like one on the device. ----
std::string g_request;
StreamReader g_stream;

EMSCRIPTEN_KEEPALIVE uint8_t* sim_alloc(int n) { return static_cast<uint8_t*>(malloc(n > 0 ? n : 1)); }
EMSCRIPTEN_KEEPALIVE void sim_free(uint8_t* p) { free(p); }

// Request body for a JPEG already copied into WASM memory.
EMSCRIPTEN_KEEPALIVE const char* sim_build_request(const uint8_t* jpeg, int len) {
  g_request = buildSolveRequest(std::string(reinterpret_cast<const char*>(jpeg), static_cast<size_t>(len)), "",
                                g_device.ai().effortParam());
  return g_request.c_str();
}
EMSCRIPTEN_KEEPALIVE const char* sim_api_url() { return "https://api.anthropic.com/v1/messages"; }
EMSCRIPTEN_KEEPALIVE const char* sim_api_version() { return kApiVersion; }
EMSCRIPTEN_KEEPALIVE const char* sim_model() { return kModel; }

EMSCRIPTEN_KEEPALIVE void sim_stream_begin() { g_stream = StreamReader(); }
EMSCRIPTEN_KEEPALIVE void sim_stream_feed(const char* chunk) { g_stream.feed(chunk); }
EMSCRIPTEN_KEEPALIVE const char* sim_stream_text() { return g_stream.text().c_str(); }

// After the stream ends: -1 if it succeeded, else a Failure code; the detail
// text is in sim_fail_detail().
std::string g_failDetail;
EMSCRIPTEN_KEEPALIVE int sim_classify(int httpStatus, const char* body) {
  Failure f;
  if (!classifyFailure(httpStatus, body, g_stream, f, g_failDetail)) return -1;
  return static_cast<int>(f);
}
EMSCRIPTEN_KEEPALIVE const char* sim_fail_detail() { return g_failDetail.c_str(); }

}  // extern "C"
