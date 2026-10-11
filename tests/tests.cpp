// Stage 1 tests: font, wrapping, JSON, reply parsing, app flow, and screen
// snapshots. Run with no arguments from the project root.
//   UPDATE_GOLDEN=1  rewrite tests/golden/*.txt from the current renders
//   SNAP_DIR=<dir>   also write each snapshot as a .pbm image
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>

#include "../core/app.h"
#include "../core/battery.h"
#include "../core/calc_engine.h"
#include "../core/claude_api.h"
#include "../core/device.h"
#include "../core/enhance.h"
#include "../core/focus.h"
#include "../core/font.h"
#include "../core/json.h"
#include "../core/text.h"
#include "../core/viewfinder.h"

using namespace calc;

static int g_fail = 0, g_pass = 0;
#define CHECK(cond)                                                   \
  do {                                                                \
    if (cond) ++g_pass;                                               \
    else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
  } while (0)

static std::string readFile(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static void snapshotFb(const std::string& name, const Framebuffer& fb);
static void snapshot(const std::string& name, const App& app) {
  Framebuffer fb;
  app.render(fb);
  snapshotFb(name, fb);
}
static void snapshot(const std::string& name, const Device& d) {
  Framebuffer fb;
  d.render(fb);
  snapshotFb(name, fb);
}
static void snapshotFb(const std::string& name, const Framebuffer& fb) {
  const std::string got = fb.toAscii();
  const std::string path = "tests/golden/" + name + ".txt";
  if (std::getenv("UPDATE_GOLDEN")) std::ofstream(path, std::ios::binary) << got;
  if (const char* dir = std::getenv("SNAP_DIR")) {
    std::ofstream pbm(std::string(dir) + "/" + name + ".pbm");
    pbm << "P1\n" << Framebuffer::kWidth << " " << Framebuffer::kHeight << "\n";
    for (int y = 0; y < Framebuffer::kHeight; ++y) {
      for (int x = 0; x < Framebuffer::kWidth; ++x) pbm << (fb.get(x, y) ? "1 " : "0 ");
      pbm << "\n";
    }
    std::ofstream(std::string(dir) + "/" + name + ".icons") << got.substr(0, got.find('\n'));
  }
  const bool same = readFile(path) == got;
  if (!same) std::printf("snapshot mismatch: %s\n", name.c_str());
  CHECK(same);
}

static void testFont() {
  CHECK(decodeUtf8("a√²").size() == 3);
  CHECK(encodeUtf8(decodeUtf8("v ≈ 9.90 m/s")) == "v ≈ 9.90 m/s");
  CHECK(decodeUtf8("\xE2\x88").back() == '?');  // truncated sequence
  CHECK(findGlyph(0x1F600) == findGlyph('?'));   // emoji falls back
  CHECK(normalizeCodepoint(0x2019) == '\'');
  // Every character used in the sample replies has a real glyph.
  DIR* d = opendir("samples");
  CHECK(d != nullptr);
  while (dirent* e = d ? readdir(d) : nullptr) {
    std::string n = e->d_name;
    if (n.size() < 5 || n.substr(n.size() - 5) != ".json") continue;
    for (uint32_t cp : decodeUtf8(readFile("samples/" + n)))
      if (cp >= 32 && !hasGlyph(cp)) {
        std::printf("no glyph U+%04X in %s\n", cp, n.c_str());
        CHECK(false);
      }
  }
  if (d) closedir(d);
}

static void testWrap() {
  auto w = wrapText("the quick brown fox jumps", 10);
  CHECK(w.size() == 3 && w[0] == "the quick" && w[1] == "brown fox" && w[2] == "jumps");
  w = wrapText("abcdefghijklmnop", 5);  // hard break
  CHECK(w.size() == 4 && w[0] == "abcde" && w[3] == "p");
  w = wrapText("a\nb", 19);
  CHECK(w.size() == 2 && w[1] == "b");
  CHECK(wrapText("", 19).size() == 1);
  w = wrapText("½mv² = mgh", 5);  // counts characters, not bytes
  CHECK(w.size() == 2 && w[0] == "½mv²" && w[1] == "= mgh");
}

static void testJson() {
  Json j;
  std::string err;
  CHECK(Json::parse(R"({"a":[1,2.5e1,true,null],"s":"x\"é😀"})", j, err));
  CHECK(j["a"].items().size() == 4 && j["a"].items()[1].asNumber() == 25);
  CHECK(j["s"].asString() == "x\"\xC3\xA9\xF0\x9F\x98\x80");
  CHECK(j["missing"].isNull());
  CHECK(!Json::parse("{\"a\":1,}", j, err));
  CHECK(!Json::parse("[1 2]", j, err));
  CHECK(!Json::parse("{\"a\":\"unterminated}", j, err));
  CHECK(!Json::parse("{} extra", j, err));
  CHECK(!Json::parse(std::string(100, '['), j, err));  // too deep
}

static void testParse() {
  SolveResult r;
  std::string err;
  CHECK(parseSolveResult(readFile("samples/01_physics_drop.json"), r, err));
  CHECK(r.steps.size() == 3 && r.answer == "v ≈ 9.90 m/s" && r.confidence > 0.9);
  CHECK(parseSolveResult(readFile("samples/05_not_a_problem.json"), r, err) && !r.readable);
  CHECK(!parseSolveResult(R"({"readable":true,"answer":""})", r, err));  // no answer
  CHECK(!parseSolveResult("[1]", r, err));
  CHECK(parseSolveResult(R"({"answer":"4","confidence":7})", r, err) && r.confidence == 1);
}

static int press(App& a, Key k) { a.onKey(k); return a.takeRequest(); }

static void testFlow() {
  App a;
  a.tick(0);
  snapshot("ready", a);
  CHECK(press(a, Key::Up) == 0 && a.screen() == Screen::Ready);

  // Happy path: = -> Reading -> Solving -> Answer, scroll through steps.
  int id = press(a, Key::Eq);
  CHECK(id != 0 && a.screen() == Screen::Busy);
  CHECK(a.takeRequest() == 0);  // handed out only once
  snapshot("busy_reading", a);
  a.onCaptured(id);
  a.tick(1300);
  snapshot("busy_solving", a);
  a.onReply(id, readFile("samples/01_physics_drop.json"));
  CHECK(a.screen() == Screen::Result);
  snapshot("answer_physics", a);
  for (int i = 0; i < 100; ++i) a.onKey(Key::Down);
  CHECK(a.scrollTop() == a.lineCount() - (Framebuffer::kRows - 1));  // 6 content rows
  snapshot("answer_physics_end", a);
  id = press(a, Key::Eq);
  a.onReply(id, readFile("samples/02_chem_moles.json"));
  snapshot("answer_chem", a);
  for (int i = 0; i < 4; ++i) a.onKey(Key::Down);
  snapshot("answer_chem_steps", a);
  for (int i = 0; i < 100; ++i) a.onKey(Key::Up);
  CHECK(a.scrollTop() == 0);

  // Unclear photo: the answer anyway, marked as a best guess, with what to check.
  id = press(a, Key::Eq);
  a.onReply(id, readFile("samples/03_handwritten_unclear.json"));
  CHECK(a.screen() == Screen::Result);
  snapshot("answer_unclear", a);
  for (int i = 0; i < 3; ++i) a.onKey(Key::Down);
  snapshot("answer_unclear_check", a);

  // Multiple choice: the letter leads, then the option; a label missing from
  // the answer text is added; a non-choice reply is unchanged.
  id = press(a, Key::Eq);
  a.onReply(id, readFile("samples/08_multiple_choice.json"));
  CHECK(a.screen() == Screen::Result && a.result().choice == "C");
  snapshot("answer_multiple_choice", a);
  SolveResult mc;
  std::string mcErr;
  CHECK(parseSolveResult(R"({"readable":true,"choice":"This is not a label","answer":"5"})", mc, mcErr) && mc.choice.empty());
  CHECK(parseSolveResult(R"({"readable":true,"answer":"5"})", mc, mcErr) && mc.choice.empty());

  // Retake from a guess starts a new request.
  CHECK(press(a, Key::Eq) != 0 && a.screen() == Screen::Busy);

  // Cancel with AC; the late reply is ignored.
  a.onKey(Key::AC);
  CHECK(a.screen() == Screen::Ready);
  a.onReply(id + 1, readFile("samples/01_physics_drop.json"));
  a.onReply(id, readFile("samples/01_physics_drop.json"));
  CHECK(a.screen() == Screen::Ready);

  // Nothing to solve / bad reply / API failure.
  id = press(a, Key::Eq);
  a.onReply(id, readFile("samples/05_not_a_problem.json"));
  CHECK(a.screen() == Screen::Message);
  snapshot("nothing_to_solve", a);
  id = press(a, Key::Eq);  // retry allowed
  a.onReply(id, "not json");
  CHECK(a.screen() == Screen::Message);
  id = press(a, Key::Eq);
  a.onFailure(id, Failure::ApiBusy);
  snapshot("claude_busy", a);

  // Offline and missing key block the request with a message.
  a.onKey(Key::AC);
  a.setOnline(false);
  snapshot("ready_offline", a);
  CHECK(press(a, Key::Eq) == 0 && a.screen() == Screen::Message);
  snapshot("no_connection", a);
  a.onKey(Key::AC);
  a.setOnline(true);
  a.setHasApiKey(false);
  CHECK(press(a, Key::Eq) == 0 && a.screen() == Screen::Message);
  CHECK(press(a, Key::Eq) == 0);  // not retryable
  a.setHasApiKey(true);

  // Power off and on.
  a.onKey(Key::Off);
  CHECK(a.screen() == Screen::Off);
  CHECK(press(a, Key::Eq) == 0 && a.screen() == Screen::Off);
  a.onKey(Key::On);
  CHECK(a.screen() == Screen::Ready);

  // A reply with no read_as and no steps still shows the answer.
  id = press(a, Key::Eq);
  a.onReply(id, R"({"answer":"42"})");
  CHECK(a.screen() == Screen::Result && a.lineCount() == 2);
}

// A realistic reply stream: ping, a thinking block, then the JSON text in pieces.
static std::string sampleStream(const std::string& replyJson) {
  std::string s = "event: message_start\ndata: {\"type\":\"message_start\",\"message\":{}}\n\n"
                  "event: ping\ndata: {\"type\": \"ping\"}\n\n"
                  "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"thinking\"}}\n\n"
                  "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"thinking_delta\",\"thinking\":\"hmm\"}}\n\n";
  // The JSON text arrives in 7-byte pieces (ASCII-only input keeps UTF-8 intact).
  for (size_t i = 0; i < replyJson.size(); i += 7) {
    std::string esc;
    for (char c : replyJson.substr(i, 7)) {
      if (c == '"' || c == '\\') esc += '\\';
      esc += c;
    }
    s += "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"text_delta\",\"text\":\"" + esc + "\"}}\n\n";
  }
  std::string out = s;
  out += "event: message_delta\ndata: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"end_turn\"}}\n\n"
         "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n";
  return out;
}

static void testClaudeApi() {
  CHECK(base64Encode("") == "" && base64Encode("f") == "Zg==" && base64Encode("fo") == "Zm8=");
  CHECK(base64Encode("foo") == "Zm9v" && base64Encode("foobar") == "Zm9vYmFy");
  CHECK(base64Encode(std::string("\xff\x00\x10", 3)) == "/wAQ");

  // The request is valid JSON with the expected pieces.
  Json req;
  std::string err;
  CHECK(Json::parse(buildSolveRequest("JPEGBYTES"), req, err));
  CHECK(req["model"].asString() == kModel && req["stream"].asBool());
  CHECK(req["thinking"]["type"].asString() == "adaptive");
  CHECK(req["system"].asString().find("25 characters wide") != std::string::npos);
  const Json& schema = req["output_config"]["format"]["schema"];
  CHECK(schema["required"].items().size() == 9 && schema["additionalProperties"].isBool());
  CHECK(schema["required"].items()[3].asString() == "expression" &&
        schema["required"].items()[4].asString() == "choice" &&
        schema["required"].items()[5].asString() == "answer");
  const Json& img = req["messages"].items()[0]["content"].items()[0];
  CHECK(img["source"]["data"].asString() == base64Encode("JPEGBYTES"));
  CHECK(req["messages"].items()[0]["content"].items().size() == 2);
  CHECK(req["output_config"]["effort"].asString() == "high");

  // Effort: ▲ ▼ on the Ready screen, clamped at both ends, sent with the request.
  App effortApp;
  effortApp.onKey(Key::Down);
  CHECK(effortApp.effort() == Effort::Normal);
  effortApp.onKey(Key::Up);
  effortApp.onKey(Key::Up);
  effortApp.onKey(Key::Up);
  CHECK(effortApp.effort() == Effort::Max && std::string(effortApp.effortParam()) == "max");
  effortApp.onKey(Key::Down);
  Json reqE;
  CHECK(Json::parse(buildSolveRequest("J", "", effortApp.effortParam()), reqE, err));
  CHECK(reqE["output_config"]["effort"].asString() == "xhigh");

  // Behind the proxy the calculator may leave the system prompt out (the proxy owns it);
  // the schema stays (the proxy's request check requires output_config.format).
  Json reqNoSys;
  const std::string noSys = buildSolveRequest("J", "", "high", false);
  CHECK(Json::parse(noSys, reqNoSys, err));
  CHECK(!reqNoSys["system"].isString() && noSys.find("\"system\"") == std::string::npos);
  CHECK(reqNoSys["output_config"]["format"]["schema"]["required"].items().size() == 9);
  CHECK(reqNoSys["messages"].items()[0]["content"].items().size() == 2);
  CHECK(buildSolveRequest("J").size() > noSys.size() + 1000);

  // With a close-up: two images, then the text that says which is which.
  Json req2;
  CHECK(Json::parse(buildSolveRequest("FULL", "CLOSE"), req2, err));
  const auto& parts = req2["messages"].items()[0]["content"].items();
  CHECK(parts.size() == 3 && parts[0]["source"]["data"].asString() == base64Encode("FULL") &&
        parts[1]["source"]["data"].asString() == base64Encode("CLOSE") &&
        parts[2]["text"].asString().find("enhanced copy") != std::string::npos);

  // Stream reading works however the bytes are split.
  const std::string stream = sampleStream(
      R"({"readable":true,"confidence":0.95,"unclear":[],"answer":"v \"fast\" 9.9 m/s","read_as":"x","steps":["a","b"]})");
  for (size_t step : {size_t(1), size_t(13), size_t(4096)}) {
    StreamReader r;
    for (size_t i = 0; i < stream.size(); i += step) r.feed(stream.substr(i, step));
    CHECK(r.finished() && r.stopReason() == "end_turn" && r.errorType().empty());
    SolveResult res;
    CHECK(parseSolveResult(r.text(), res, err) && res.answer == "v \"fast\" 9.9 m/s" && res.steps.size() == 2);
    Failure f;
    std::string detail;
    CHECK(!classifyFailure(200, "", r, f, detail));
  }

  // Early answer: only once the whole answer string has arrived.
  double conf;
  std::string ans;
  CHECK(!peekAnswer(R"({"readable":true,"confidence":0.9,"unclear":[],"answer":"v = 9.)", conf, ans));
  CHECK(!peekAnswer(R"({"readable":true,"confidence":0.9,"unclear":[],"answer":"a\")", conf, ans));
  CHECK(peekAnswer(R"({"readable":true,"confidence":0.9,"unclear":["the answer"],"answer":"a\"b","read)", conf, ans));
  CHECK(conf == 0.9 && ans == "a\"b");
  CHECK(!peekAnswer(R"({"readable":false,"confidence":0.1,"unclear":[],"answer":"","read_as)", conf, ans));

  // Failures.
  Failure f;
  std::string detail;
  StreamReader none;
  CHECK(classifyFailure(401, "", none, f, detail) && f == Failure::ApiError && detail == "API key was rejected.");
  CHECK(classifyFailure(529, "", none, f, detail) && f == Failure::ApiBusy);
  CHECK(classifyFailure(429, "", none, f, detail) && f == Failure::ApiBusy);
  CHECK(classifyFailure(400, R"({"type":"error","error":{"type":"invalid_request_error","message":"image too big"}})", none, f, detail) && detail == "image too big");
  CHECK(classifyFailure(200, "", none, f, detail) && f == Failure::NoConnection);  // cut off
  StreamReader over;
  over.feed("event: error\ndata: {\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\",\"message\":\"Overloaded\"}}\n\n");
  CHECK(classifyFailure(200, "", over, f, detail) && f == Failure::ApiBusy);
  StreamReader crlf;
  crlf.feed("data: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"hi\"}}\r\n\r");
  crlf.feed("\ndata: {\"type\":\"message_stop\"}\r\n\r\n");
  CHECK(crlf.text() == "hi" && crlf.finished());
  StreamReader refused;
  refused.feed("data: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"refusal\"}}\n\ndata: {\"type\":\"message_stop\"}\n\n");
  CHECK(classifyFailure(200, "", refused, f, detail) && f == Failure::ApiError);
}

static void testStreamingFlow() {
  App a;
  int id = press(a, Key::Eq);
  a.onCaptured(id);
  a.onPartialAnswer(id, 0.5, "x = 4");  // unclear reading: shown at once as a best guess
  CHECK(a.screen() == Screen::Result);
  a.onPartialAnswer(id, 0.95, "v \xE2\x89\x88 9.90 m/s");
  CHECK(a.screen() == Screen::Result);
  snapshot("answer_streaming", a);
  a.onReply(id, readFile("samples/01_physics_drop.json"));
  CHECK(a.screen() == Screen::Result && a.lineCount() > 4);

  // AC while steps are still arriving stops the request.
  id = press(a, Key::Eq);
  a.onPartialAnswer(id, 0.95, "42");
  a.onKey(Key::AC);
  CHECK(a.screen() == Screen::Ready);
  a.onReply(id, readFile("samples/01_physics_drop.json"));
  CHECK(a.screen() == Screen::Ready);

  // A failure after the early answer shows the error.
  id = press(a, Key::Eq);
  a.onPartialAnswer(id, 0.95, "42");
  a.onFailure(id, Failure::NoConnection, "Connection dropped mid-reply.");
  CHECK(a.screen() == Screen::Message);

  a.setHasApiKey(false);
  a.setNoKeyHelp("Put your key in api_key.txt");
  a.onKey(Key::AC);
  press(a, Key::Eq);
  snapshot("no_api_key_sim", a);
}


// ---------------------------------------------------------------- Stage 4: calculator

static double fixedRandom() { return 0.5; }

// Evaluates typed text the way it would be keyed in; returns the display text.
static std::string calcText(const std::string& text, AngleUnit u = AngleUnit::Deg, bool frac = true,
                            NormMode n = NormMode::Norm1) {
  std::vector<Tok> t;
  if (!parseExpression(text, t)) return "PARSE";
  Vars v;
  EvalResult r = evaluate(t, v, u, fixedRandom);
  if (r.error != CalcError::None) return errorText(r.error);
  return formatResult(r.value, frac, n);
}

static void testEngine() {
  // Every token and status symbol has a real glyph.
  for (int i = 0; i < static_cast<int>(Tok::Count); ++i)
    for (uint32_t cp : decodeUtf8(tokText(static_cast<Tok>(i)))) CHECK(hasGlyph(cp));
  for (uint32_t cp : decodeUtf8("\xE2\x8C\x9F\xE2\x96\xB2\xE2\x96\xBC\xE2\x97\x80\xE2\x96\xB6\xEE\x80\x80\xEE\x80\x81\xE2\x86\x92"))
    CHECK(hasGlyph(cp));

  // Exact fractions, like Math mode.
  CHECK(calcText("1/3") == "1\xE2\x8C\x9F" "3");
  CHECK(calcText("1/3", AngleUnit::Deg, false) == "0.3333333333");
  CHECK(calcText("1\xE2\x8C\x9F" "2+1\xE2\x8C\x9F" "3") == "5\xE2\x8C\x9F" "6");
  CHECK(calcText("0.5+0.25") == "3\xE2\x8C\x9F" "4");
  CHECK(calcText("6/3") == "2");
  // Casio operator priorities.
  CHECK(calcText("2+3*4") == "14");
  CHECK(calcText("-2^2") == "\xE2\x88\x92" "4");       // (−)2² = −4
  CHECK(calcText("(-2)^2") == "4");
  CHECK(calcText("1/2pi") == "0.1591549431");          // implicit × binds tighter than ÷
  CHECK(calcText("2(3+4") == "14");                     // auto-closed bracket
  CHECK(calcText("2^3^2") == "64");                     // ^ left to right, as on the Casio
  CHECK(calcText("10C3") == "PARSE");                   // C is not typed text; use keys below
  // Functions.
  CHECK(calcText("sin(30)") == "1\xE2\x8C\x9F" "2");
  CHECK(calcText("cos(90)") == "0");
  CHECK(calcText("tan(45)") == "1");
  CHECK(calcText("tan(90)") == "Math ERROR");
  CHECK(calcText("sin(pi/6)", AngleUnit::Rad) == "1\xE2\x8C\x9F" "2");
  CHECK(calcText("sin(100)", AngleUnit::Deg, true) != calcText("sin(100)", AngleUnit::Rad, true));
  CHECK(calcText("asin(1)") == "90");
  CHECK(calcText("asin(2)") == "Math ERROR");
  CHECK(calcText("sqrt(16)") == "4");
  CHECK(calcText("sqrt(2)") == "1.414213562");
  CHECK(calcText("sqrt(-1)") == "Math ERROR");
  CHECK(calcText("cbrt(-27)") == "\xE2\x88\x92" "3");
  CHECK(calcText("log(1000)") == "3");
  CHECK(calcText("log(2,8)") == "3");
  CHECK(calcText("ln(e)") == "1");
  CHECK(calcText("log(0)") == "Math ERROR");
  CHECK(calcText("5!") == "120");
  CHECK(calcText("70!") == "Math ERROR");
  CHECK(calcText("2.5!") == "Math ERROR");
  CHECK(calcText("abs(-3)") == "3");
  CHECK(calcText("gcd(12,18)") == "6");
  CHECK(calcText("lcm(4,6)") == "12");
  CHECK(calcText("50%") == "1\xE2\x8C\x9F" "2");
  CHECK(calcText("2^-1") == "1\xE2\x8C\x9F" "2");
  CHECK(calcText("0^-1") == "Math ERROR");
  // Errors.
  CHECK(calcText("1/0") == "Math ERROR");
  CHECK(calcText("1+") == "Syntax ERROR");
  CHECK(calcText("1..2") == "Syntax ERROR");
  CHECK(calcText("10^(100)") == "Math ERROR");         // |x| >= 1E100
  CHECK(calcText("gcd(2)") == "Syntax ERROR");
  // Big numbers leave exact fractions without overflowing (64-bit on the ESP32).
  CHECK(calcText("123456789*987654321") == "1.219326311\xC3\x97" "10\xC2\xB9\xE2\x81\xB7");
  CHECK(calcText("1/999999937+1/999999929") == "2.000000134\xC3\x97" "10\xE2\x81\xBB\xE2\x81\xB9");
  CHECK(calcText("25!/24!") == "25");
  CHECK(calcText("9999999999*9999999999*9999999999") == "9.999999997\xC3\x97" "10\xC2\xB2\xE2\x81\xB9");
  // Display: 10 significant digits, scientific notation outside Norm range.
  CHECK(formatDecimal(123456789012.0, NormMode::Norm1) == "1.23456789\xC3\x97" "10\xC2\xB9\xC2\xB9");
  CHECK(formatDecimal(0.001, NormMode::Norm1) == "1\xC3\x97" "10\xE2\x81\xBB\xC2\xB3");
  CHECK(formatDecimal(0.001, NormMode::Norm2) == "0.001");
  CHECK(formatDecimal(-2.5, NormMode::Norm1) == "\xE2\x88\x92" "2.5");
  CHECK(formatDecimal(9999999999.0, NormMode::Norm1) == "9999999999");
  CHECK(calcText("1E20*123") == "1.23\xC3\x97" "10\xC2\xB2\xC2\xB2");
  CHECK(calcText("3.2E-5*2", AngleUnit::Deg, false) == "6.4\xC3\x97" "10\xE2\x81\xBB\xE2\x81\xB5");
  // Typed text from a scan.
  std::vector<Tok> t;
  CHECK(parseExpression("2*sqrt(3)+sin(30)^2", t) && exprText(t) == "2\xC3\x97\xE2\x88\x9A(3)+sin(30)^(2)");
  CHECK(parseExpression("2^3+1", t) && exprText(t) == "2^(3)+1");
  CHECK(parseExpression("5-3", t) && t[1] == Tok::Sub);
  CHECK(parseExpression("-3", t) && t[0] == Tok::Neg);
  CHECK(!parseExpression("2x+1", t) && t.empty());
  CHECK(!parseExpression("", t));
  CHECK(!parseExpression(std::string(120, '1'), t));  // longer than 99 steps
}

using K = DKey;
static void keys(Device& d, std::initializer_list<DKey> ks) {
  for (DKey k : ks) d.onKey(k);
}

static void testDevice() {
  Device d;
  d.setRandomSource(fixedRandom);
  d.tick(0);
  d.setOnline(true);
  CHECK(d.view() == View::Calc && d.mode() == Mode::Comp);

  // Type, edit, evaluate.
  keys(d, {K::D1, K::Div, K::D3, K::Add, K::Sin, K::D3, K::D0});
  snapshot("calc_editing", d);
  keys(d, {K::Eq});
  CHECK(d.resultText() == "5\xE2\x8C\x9F" "6");
  snapshot("calc_result_fraction", d);
  keys(d, {K::SD});
  CHECK(d.resultText() == "0.8333333333");
  keys(d, {K::SD});
  // Carry on from Ans.
  keys(d, {K::Mul, K::D6, K::Eq});
  CHECK(exprText(d.expression()) == "Ans\xC3\x97" "6" && d.resultText() == "5");
  // A new number starts fresh.
  keys(d, {K::D2, K::Eq});
  CHECK(d.resultText() == "2");
  // Replay history with up / down.
  keys(d, {K::Up});
  CHECK(d.resultText() == "5");
  keys(d, {K::Up});
  CHECK(d.resultText() == "5\xE2\x8C\x9F" "6");
  keys(d, {K::Up});  // oldest: stays
  CHECK(d.resultText() == "5\xE2\x8C\x9F" "6");
  keys(d, {K::Down, K::Down});
  CHECK(d.resultText() == "2");
  // Cursor editing: go left, delete, insert.
  keys(d, {K::AC, K::D1, K::D2, K::D3, K::Left, K::Left, K::Del, K::D9, K::Eq});
  CHECK(d.resultText() == "923");  // DEL removes the character left of the cursor
  keys(d, {K::Left});  // back into the expression, cursor at the end
  CHECK(!d.showingResult() && d.cursor() == 3);
  keys(d, {K::Right});  // wraps to the start
  CHECK(d.cursor() == 0);
  keys(d, {K::Shift, K::Del, K::D5, K::Eq});  // INS: overwrite mode
  CHECK(d.resultText() == "523");
  keys(d, {K::Shift, K::Del});

  // SHIFT functions.
  keys(d, {K::AC, K::D5, K::Shift, K::Inv, K::Eq});
  CHECK(d.resultText() == "120");
  keys(d, {K::AC, K::D1, K::D0, K::Shift, K::Div, K::D3, K::Eq});  // nCr = SHIFT ÷
  CHECK(d.resultText() == "120");
  keys(d, {K::AC, K::D5, K::Shift, K::Mul, K::D2, K::Eq});  // nPr = SHIFT ×
  CHECK(d.resultText() == "20");
  keys(d, {K::AC, K::Shift, K::Hyp, K::Neg, K::D7, K::Eq});  // fx-115ES: Abs = SHIFT hyp
  CHECK(d.resultText() == "7");
  keys(d, {K::AC, K::D2, K::Shift, K::Sq, K::Eq});  // fx-115ES: x³ = SHIFT x²
  CHECK(d.resultText() == "8");
  keys(d, {K::AC, K::Shift, K::Sqrt, K::D2, K::D7, K::Eq});  // fx-115ES: ∛ = SHIFT √
  CHECK(d.resultText() == "3");
  keys(d, {K::AC, K::Calc});  // CALC isn't implemented: a clear notice, nothing else
  CHECK(d.view() == View::Notice);
  snapshot("notice_calc", d);
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc);
  keys(d, {K::Shift, K::Integral});  // d/dx: same
  CHECK(d.view() == View::Notice);
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc);
  keys(d, {K::AC, K::LogAB, K::D2, K::Shift, K::Close, K::D8, K::Eq});  // logₐb: log(2,8)
  CHECK(d.resultText() == "3");
  keys(d, {K::AC, K::Shift, K::Exp10, K::Eq});
  CHECK(d.resultText() == "3.141592654");
  keys(d, {K::AC, K::D2, K::Shift, K::Pow, K::D1, K::D6, K::Eq});  // 2ˣ√16
  CHECK(d.resultText() == "4");
  keys(d, {K::AC, K::Hyp});
  CHECK(d.view() == View::Hyp);
  snapshot("menu_hyp", d);
  keys(d, {K::D1, K::D0, K::Eq});
  CHECK(d.resultText() == "0");

  // ENG notation.
  keys(d, {K::AC, K::D1, K::D2, K::D3, K::D4, K::D5, K::Eq, K::Eng});
  CHECK(d.resultText() == "12.345\xC3\x97" "10\xC2\xB3");
  keys(d, {K::Eng});
  CHECK(d.resultText() == "12345\xC3\x97" "10\xE2\x81\xB0");

  // Errors: Math ERROR, then Goto puts the cursor at the fault.
  keys(d, {K::AC, K::D1, K::Div, K::D0, K::Eq});
  CHECK(d.view() == View::Error && d.errorMessage() == "Math ERROR");
  snapshot("calc_math_error", d);
  keys(d, {K::Right});
  CHECK(d.view() == View::Calc && !d.showingResult());
  keys(d, {K::AC, K::D1, K::Add, K::Eq});
  CHECK(d.errorMessage() == "Syntax ERROR");
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc && d.expression().empty());

  // Memory: STO, RCL, ALPHA variables, M+.
  keys(d, {K::D7, K::Shift, K::Rcl, K::Neg});  // 7 STO A
  CHECK(d.vars().a.v == 7);
  snapshot("calc_sto", d);
  keys(d, {K::AC, K::Alpha, K::Neg, K::Mul, K::D2, K::Eq});
  CHECK(d.resultText() == "14");
  keys(d, {K::AC, K::D3, K::MPlus, K::D2, K::MPlus});
  CHECK(d.vars().m.v == 5);
  keys(d, {K::AC, K::D1, K::Shift, K::MPlus});
  CHECK(d.vars().m.v == 4);
  keys(d, {K::AC, K::Rcl, K::MPlus});
  CHECK(d.resultText() == "4");
  keys(d, {K::Add, K::D1, K::Eq});  // "M+1"
  CHECK(d.resultText() == "5");

  // SETUP: angle unit and Norm.
  keys(d, {K::AC, K::Shift, K::Mode});
  CHECK(d.view() == View::Setup);
  snapshot("menu_setup", d);
  keys(d, {K::D2});
  CHECK(d.angleUnit() == AngleUnit::Rad && d.view() == View::Calc);
  keys(d, {K::Sin, K::Shift, K::Exp10, K::Div, K::D2, K::Eq});
  CHECK(d.resultText() == "1");
  keys(d, {K::Shift, K::Mode, K::D1});
  keys(d, {K::Shift, K::Mode, K::D5, K::AC, K::D1, K::Div, K::D2, K::D0, K::D0, K::D0, K::SD, K::Eq, K::SD});
  CHECK(d.resultText() == "0.0005");
  keys(d, {K::Shift, K::Mode, K::D4});
  CHECK(d.norm() == NormMode::Norm1);
  keys(d, {K::Shift, K::Mode, K::D7, K::D1});
  CHECK(!d.dotMatrix());
  keys(d, {K::Shift, K::Mode, K::D7, K::D2});
  CHECK(d.dotMatrix());

  // MODE menu.
  keys(d, {K::Mode});
  snapshot("menu_mode", d);
  keys(d, {K::D2});
  CHECK(d.view() == View::Notice);
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc);

  // CLR: memory only.
  keys(d, {K::Shift, K::D9, K::D2, K::Eq});
  CHECK(d.vars().a.v == 0 && d.vars().m.v == 0 && d.angleUnit() == AngleUnit::Deg);
  keys(d, {K::AC});

  // Power: SHIFT AC is OFF, ON turns it back on; idle for 10 min turns it off.
  keys(d, {K::Shift, K::AC});
  CHECK(d.isOff());
  keys(d, {K::D5});
  CHECK(d.isOff());
  keys(d, {K::On});
  CHECK(!d.isOff());
  d.tick(Device::kAutoOffMs + 1000);
  CHECK(d.isOff());
  keys(d, {K::On});
}

static void testDeviceAi() {
  Device d;
  d.tick(0);
  d.setOnline(true);
  keys(d, {K::Mode, K::D4});
  CHECK(d.mode() == Mode::Ai && d.view() == View::Ai);
  snapshot("ai_ready", d);
  keys(d, {K::Eq});
  int id = d.takeRequest();
  CHECK(id != 0 && d.activeRequest() == id);
  d.onCaptured(id);
  d.onReply(id, readFile("samples/06_arithmetic.json"));
  snapshot("ai_answer", d);
  // The scan is already in the calculator; AC goes there to check it.
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
  CHECK(exprText(d.expression()) == "3\xC3\xB7" "4+2^(5)\xC3\x97sin(30)");
  snapshot("scan_synced", d);
  keys(d, {K::Eq});
  CHECK(d.resultText() == "67\xE2\x8C\x9F" "4");

  // Hold-still countdown while the photo is taken, then the usual busy screen.
  keys(d, {K::Mode, K::D4});
  d.tick(1000);
  keys(d, {K::Eq});
  const int holdId = d.takeRequest();
  d.setHoldUntil(3500);
  d.tick(1100);
  snapshot("ai_hold_countdown", d);  // "HOLD STILL", big "3 s"
  d.tick(3600);
  snapshot("ai_hold_done", d);       // countdown over, frames being checked
  d.onCaptured(holdId);
  d.tick(3700);
  snapshot("ai_hold_captured", d);   // Solving / Claude is working
  keys(d, {K::AC});

  // The preview page's Send starts a scan from any screen: the calculator,
  // a scan still busy, or an answer.
  keys(d, {K::AC, K::D1, K::Add, K::D2});
  CHECK(d.view() == View::Calc && d.startScan());
  const int busyId = d.takeRequest();
  CHECK(busyId != 0 && d.view() == View::Ai);
  CHECK(d.startScan());
  const int againId = d.takeRequest();
  CHECK(againId != 0 && againId != busyId && d.activeRequest() == againId);
  d.onReply(againId, readFile("samples/06_arithmetic.json"));
  CHECK(d.startScan() && d.takeRequest() != 0);
  keys(d, {K::On});  // off: no scan
  keys(d, {K::Shift, K::AC});
  CHECK(d.isOff() && !d.startScan());
  keys(d, {K::On});
  keys(d, {K::Mode, K::D1});

  // An unclear scan is flagged on the calculator screen.
  keys(d, {K::Mode, K::D4, K::Eq});
  id = d.takeRequest();
  d.onReply(id, readFile("samples/07_unclear_arithmetic.json"));
  CHECK(d.ai().screen() == Screen::Result);  // a best guess, shown straight away
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc);
  snapshot("scan_unclear", d);
  keys(d, {K::Eq});
  CHECK(d.resultText() == "117\xE2\x8C\x9F" "2");

  // A word problem with no typed calculation leaves the calculator alone.
  keys(d, {K::AC, K::D4, K::D2, K::Mode, K::D4, K::Eq});
  id = d.takeRequest();
  d.onReply(id, readFile("samples/02_chem_moles.json"));
  keys(d, {K::AC});
  CHECK(d.view() == View::Ai);  // back to the AI home screen
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc && exprText(d.expression()) == "42");

  // MODE while solving cancels the request.
  keys(d, {K::Mode, K::D4, K::Eq});
  id = d.takeRequest();
  keys(d, {K::Mode, K::D1});
  CHECK(d.activeRequest() == 0 && d.mode() == Mode::Comp);
}

static void testExam() {
  Device d;
  d.tick(0);
  d.setOnline(true);
  // From SETUP.
  keys(d, {K::Shift, K::Mode, K::D8});
  snapshot("exam_confirm", d);
  keys(d, {K::Eq});
  CHECK(d.examActive() && !d.radiosAllowed());
  keys(d, {K::AC});
  for (uint32_t t = 300000; t <= 3720000; t += 300000) {  // a key every 5 min keeps it awake
    d.tick(t);
    keys(d, {K::AC});
  }
  keys(d, {K::D5, K::Shift, K::Inv, K::Eq});
  d.tick(3723000);
  snapshot("exam_status_bar", d);
  // AI is refused, Wi-Fi reported off.
  keys(d, {K::Mode, K::D4});
  CHECK(d.view() == View::Notice && d.mode() == Mode::Comp);
  keys(d, {K::AC});
  d.setOnline(true);
  keys(d, {K::Shift, K::Mode, K::D6});
  snapshot("exam_wifi_off", d);
  keys(d, {K::AC, K::AC});
  // CLR All keeps exam mode; power off keeps it.
  keys(d, {K::Shift, K::D9, K::D3, K::Eq, K::AC});
  CHECK(d.examActive());
  keys(d, {K::Shift, K::AC, K::On});
  CHECK(d.examActive());
  // Only the USB cable ends it early.
  d.usbUnlock();
  CHECK(!d.examActive() && d.view() == View::Notice);
  keys(d, {K::AC});

  // SHIFT, 7, ON while off also starts it; it ends by itself after 12 h.
  keys(d, {K::Shift, K::AC, K::Shift, K::D7, K::On});
  CHECK(d.examActive());
  uint32_t t = 4000000;
  for (int i = 0; i < 13 * 12; ++i) {  // key presses every 5 min for 13 hours
    t += 5 * 60 * 1000;
    d.tick(t);
    keys(d, {K::D1});
  }
  CHECK(!d.examActive());
  // Tapping 7 then ON without SHIFT is just ON.
  keys(d, {K::Shift, K::AC, K::D7, K::On});
  CHECK(!d.examActive() && !d.isOff());
}

// A 400x300 "page": light paper with sensor-like texture, and a block of
// dark strokes (the writing) at x 220-340, y 60-140. blur=true smears it.
static std::vector<uint8_t> page(bool blur, bool writing = true) {
  const int w = 400, h = 300;
  std::vector<uint8_t> g(w * h);
  unsigned seed = 7;
  for (auto& p : g) {
    seed = seed * 1103515245 + 12345;
    p = static_cast<uint8_t>(200 + (seed >> 16) % 5);  // paper grain
  }
  if (writing)
    for (int y = 60; y < 140; ++y)
      for (int x = 220; x < 340; ++x)
        if ((x / 3) % 3 == 0 && (y / 10) % 2 == 0) g[y * w + x] = 30;  // strokes
  if (blur) {  // two passes of a 5x5 box blur
    for (int pass = 0; pass < 2; ++pass) {
      std::vector<uint8_t> o(g);
      for (int y = 2; y < h - 2; ++y)
        for (int x = 2; x < w - 2; ++x) {
          int s = 0;
          for (int dy = -2; dy <= 2; ++dy)
            for (int dx = -2; dx <= 2; ++dx) s += g[(y + dy) * w + x + dx];
          o[y * w + x] = static_cast<uint8_t>(s / 25);
        }
      g.swap(o);
    }
  }
  return g;
}

static void testFocus() {
  const auto sharp = page(false), soft = page(true), blank = page(false, false);
  const Focus a = analyseFocus(sharp.data(), 400, 300);
  const Focus b = analyseFocus(soft.data(), 400, 300);
  const Focus c = analyseFocus(blank.data(), 400, 300);
  CHECK(a.found && a.score > 3 * b.score);  // sharper photo wins clearly
  CHECK(a.x0 <= 225 && a.x1 >= 336 && a.y0 <= 60 && a.y1 >= 130);  // box holds the ink (x 225-337, y 60-129)
  CHECK(a.x0 >= 200 && a.x1 <= 352 && a.y0 >= 48 && a.y1 <= 152);  // and not much else
  CHECK(!c.found);                                                  // blank page: nothing to box
  CHECK(!analyseFocus(nullptr, 0, 0).found);

  // Picking settings for the light: the well-lit photo beats a dark one, a
  // washed-out one and a grainy one of the same page.
  auto relit = [](std::vector<uint8_t> g, double gain, int offset, int grain) {
    unsigned seed = 3;
    for (auto& p : g) {
      seed = seed * 1103515245 + 12345;
      int v = int(p * gain) + offset + (grain ? int((seed >> 16) % (2 * grain + 1)) - grain : 0);
      p = static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v);
    }
    return g;
  };
  const auto good = relit(sharp, 1.15, 0, 0), dark = relit(sharp, 0.55, 0, 0),
             washed = relit(sharp, 1.6, 0, 0), grainy = relit(sharp, 1.15, 0, 25);
  auto quality = [](const std::vector<uint8_t>& g) {
    const Focus f = analyseFocus(g.data(), 400, 300);
    return scanQuality(f, measureExposure(g.data(), 400, 300, f));
  };
  const Exposure eg = measureExposure(good.data(), 400, 300, analyseFocus(good.data(), 400, 300));
  CHECK(eg.paper >= 225 && eg.paper <= 240 && eg.dark <= 40 && eg.clipped < 0.01);
  CHECK(measureExposure(washed.data(), 400, 300, Focus{}).clipped > 0.5);
  CHECK(quality(good) > quality(dark) && quality(good) > quality(washed) && quality(good) > quality(grainy));
  CHECK(quality(blank) == 0);

  // Close-up on a 1600x1200 photo: scaled x4, padded, multiples of 8, inside the frame.
  const Crop k = detailCrop(a, 400, 300, 1600, 1200);
  CHECK(k.use && k.x % 8 == 0 && k.y % 8 == 0 && k.w % 8 == 0 && k.h % 8 == 0);
  CHECK(k.x <= 900 && k.x + k.w >= 1348 && k.y <= 240 && k.y + k.h >= 520);
  CHECK(k.x >= 0 && k.y >= 0 && k.x + k.w <= 1600 && k.y + k.h <= 1200 && k.w >= 640 && k.h >= 480);
  // Writing filling most of the frame: no close-up. Nothing found: none either.
  Focus big = a;
  big.x0 = 8; big.y0 = 8; big.x1 = 392; big.y1 = 292;
  CHECK(!detailCrop(big, 400, 300, 1600, 1200).use);
  CHECK(!detailCrop(c, 400, 300, 1600, 1200).use);
  // A tiny box in a corner grows to 640x480 and stays inside the frame.
  Focus corner = a;
  corner.x0 = 0; corner.y0 = 0; corner.x1 = 16; corner.y1 = 16;
  const Crop m = detailCrop(corner, 400, 300, 1600, 1200);
  CHECK(m.use && m.x == 0 && m.y == 0 && m.w == 640 && m.h == 480);
}

static void testEnhance() {
  // A page lit from the right: paper 110 on the left to 220 on the right,
  // ink strokes 40 darker than the paper around them (hard to read on the left).
  const int w = 640, h = 480;
  std::vector<uint8_t> g(w * h);
  auto inkAt = [](int x, int y) { return y > 200 && y < 280 && (x / 4) % 4 == 0; };
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      const int paper = 110 + 110 * x / w;
      g[y * w + x] = static_cast<uint8_t>(inkAt(x, y) ? paper - 40 : paper);
    }
  int ow = w, oh = h;
  int calls = 0;
  static int* counter = nullptr;
  counter = &calls;
  enhanceWriting(g, ow, oh, 1150000, [] { ++*counter; });
  CHECK(calls > 0);
  // Enlarged ~1.9x to about 1.15 MP, aspect kept.
  CHECK(ow > 1200 && ow < 1260 && oh > 900 && oh < 950 && long(ow) * oh <= 1150000);
  // Paper is now evenly white on both sides; ink is dark on both sides.
  auto at = [&](int x, int y) { return int(g[size_t(y) * ow + x]); };
  const double sx = double(ow) / w, sy = double(oh) / h;
  CHECK(at(int(30 * sx), int(100 * sy)) >= 230 && at(int(600 * sx), int(100 * sy)) >= 230);
  int darkLeft = 255, darkRight = 255;
  for (int x = int(20 * sx); x < int(80 * sx); ++x) darkLeft = std::min(darkLeft, at(x, int(240 * sy)));
  for (int x = int(560 * sx); x < int(620 * sx); ++x) darkRight = std::min(darkRight, at(x, int(240 * sy)));
  CHECK(darkLeft < 110 && darkRight < 180);  // right-side ink was only 18% darker than its paper
  // A whole UXGA frame is shrunk to the same budget; tiny inputs are left alone.
  std::vector<uint8_t> big(1600 * 1200, 200);
  int bw2 = 1600, bh2 = 1200;
  enhanceWriting(big, bw2, bh2);
  CHECK(long(bw2) * bh2 <= 1150000 && bw2 > 1200);
  std::vector<uint8_t> tiny(4, 9);
  int tw = 2, th = 2;
  enhanceWriting(tiny, tw, th);
  CHECK(tw == 2 && th == 2);
}

// Stage 13: battery maths, deep-sleep state, proxy errors, verified answers, tutor mode.
static void testStage13() {
  // LiPo curve: ends, flat middle, monotonic.
  CHECK(lipoPercent(3000) == 0 && lipoPercent(4250) == 100);
  CHECK(lipoPercent(4200) == 100 && lipoPercent(3270) == 0);
  CHECK(lipoPercent(3840) == 50);
  CHECK(lipoPercent(3700) >= 10 && lipoPercent(3700) <= 15);
  bool mono = true;
  for (int mv = 3200; mv < 4300; mv += 5) mono &= lipoPercent(mv) <= lipoPercent(mv + 5);
  CHECK(mono);
  CHECK(chargeState(false, false) == Charge::NoCable && chargeState(false, true) == Charge::NoCable);
  CHECK(chargeState(true, false) == Charge::Charging && chargeState(true, true) == Charge::Full);
  const int samples[] = {2000, 2010, 1990, 1500, 2600, 2000};
  CHECK(robustAverage(samples, 6) == 2000);

  // Deep sleep keeps memories, settings, history and exam mode.
  {
    Device d;
    d.tick(0);
    keys(d, {K::D7, K::Eq});                       // Ans = 7, history 1
    keys(d, {K::D2, K::Shift, K::Rcl, K::Neg});    // 2 -> A (STO A)
    keys(d, {K::Shift, K::Mode, K::D2});           // SETUP 2: Rad
    keys(d, {K::Shift, K::AC});                    // off
    CHECK(d.isOff());
    const std::string st = d.saveState();
    CHECK(st.size() > 20 && st.size() <= 2048);
    Device e;
    e.tick(0);
    CHECK(e.restoreState(st, 1000));
    CHECK(e.isOff() && e.angleUnit() == AngleUnit::Rad);
    CHECK(e.vars().ans.v == d.vars().ans.v && e.vars().a.v == d.vars().a.v);
    keys(e, {K::On, K::Up});                       // replay the history
    CHECK(exprText(e.expression()) == exprText(d.expression()) || !e.expression().empty());
    CHECK(!e.restoreState("garbage"));
    CHECK(!e.restoreState(""));
    // exam mode: survives, and 12 h asleep ends it
    Device x;
    x.tick(0);
    keys(x, {K::Shift, K::AC, K::Shift, K::D7, K::On});
    CHECK(x.examActive());
    keys(x, {K::AC, K::Shift, K::AC});
    const std::string xs = x.saveState();
    Device y;
    y.tick(0);
    CHECK(y.restoreState(xs, 60000) && y.examActive());
    Device z;
    z.tick(0);
    CHECK(z.restoreState(xs, Device::kExamMaxMs) && !z.examActive());
    // SHIFT then a deep sleep then 7, ON still starts exam mode
    Device w;
    w.tick(0);
    keys(w, {K::Shift, K::AC, K::Shift});
    Device w2;
    w2.tick(0);
    CHECK(w2.restoreState(w.saveState()));
    keys(w2, {K::D7, K::On});
    CHECK(w2.examActive());
    // a tiny budget still saves the settings (no history)
    CHECK(d.saveState(290).size() <= 290 && d.saveState(290).size() < st.size());
  }

  // Proxy account errors show the proxy's message (with the pairing code).
  {
    StreamReader sr;
    Failure f;
    std::string det;
    CHECK(classifyFailure(402, "{\"type\":\"error\",\"error\":{\"type\":\"device_not_linked\",\"message\":\"Link code K7Q2PX\"}}",
                          sr, f, det));
    CHECK(f == Failure::Account && det == "Link code K7Q2PX");
    CHECK(classifyFailure(429, "{\"type\":\"error\",\"error\":{\"type\":\"fair_use_exceeded\",\"message\":\"500 solves used\"}}",
                          sr, f, det));
    CHECK(f == Failure::Account);
    // The proxy's burst limit (firmware with this check) shows its message, not "busy".
    CHECK(classifyFailure(429, "{\"type\":\"error\",\"error\":{\"type\":\"rate_limited\",\"message\":\"Wait a moment, then try again.\"}}",
                          sr, f, det));
    CHECK(f == Failure::Account && det == "Wait a moment, then try again.");
    // Without a message it is still just "busy".
    CHECK(classifyFailure(429, "{\"type\":\"error\",\"error\":{\"type\":\"rate_limited\"}}", sr, f, det));
    CHECK(f == Failure::ApiBusy);
    CHECK(classifyFailure(429, "{\"type\":\"error\",\"error\":{\"type\":\"rate_limit_error\",\"message\":\"x\"}}", sr, f, det));
    CHECK(f == Failure::ApiBusy);
    CHECK(std::string(solveSchema()).find("\"check\"") != std::string::npos);
  }

  // Verified answers: the calculator re-computes Claude's number.
  {
    std::string c;
    CHECK(verifyAnswer("24.5 m", "9.81*2.5", c) == Verify::Verified);
    CHECK(verifyAnswer("x = 24.5 m", "9.81*2.5", c) == Verify::Verified);
    CHECK(verifyAnswer("30 m", "9.81*2.5", c) == Verify::Mismatch && !c.empty());
    CHECK(verifyAnswer("(C) 9.90 m/s", "sqrt(98)", c) == Verify::Verified);
    CHECK(verifyAnswer("3.2\xC3\x97" "10^-5 mol", "3.2E-5", c) == Verify::Verified);
    CHECK(verifyAnswer("\xE2\x88\x92" "4", "2-6", c) == Verify::Verified);
    CHECK(verifyAnswer("1/3", "1/3", c) == Verify::Verified);
    CHECK(verifyAnswer("1,200 N", "1200", c) == Verify::Verified);
    CHECK(verifyAnswer("H2O", "2", c) == Verify::None);
    CHECK(verifyAnswer("24.5 m", "", c) == Verify::None);
    CHECK(verifyAnswer("24.5 m", "9.81*", c) == Verify::None);
    App a;
    a.onKey(Key::Eq);
    int id = a.takeRequest();
    a.onReply(id, "{\"readable\":true,\"confidence\":0.95,\"unclear\":[],\"expression\":\"\",\"choice\":\"\","
                  "\"answer\":\"24.5 m\",\"check\":\"9.81*2.5\",\"read_as\":\"How far?\",\"steps\":[\"d=g*t\",\"=24.5 m\"]}");
    CHECK(a.verified() == Verify::Verified);
    snapshot("ai_verified", a);
    a.onKey(Key::AC);
    a.onKey(Key::Eq);
    id = a.takeRequest();
    a.onReply(id, "{\"readable\":true,\"confidence\":0.95,\"unclear\":[],\"expression\":\"\",\"choice\":\"\","
                  "\"answer\":\"30 m\",\"check\":\"9.81*2.5\",\"read_as\":\"How far?\",\"steps\":[\"d=g*t\"]}");
    CHECK(a.verified() == Verify::Mismatch);
    snapshot("ai_mismatch", a);
  }

  // Tutor mode: hints one at a time, the answer last; no early answer.
  {
    App a;
    a.onKey(Key::Tutor);
    CHECK(a.tutor());
    snapshot("ai_tutor_ready", a);
    a.onKey(Key::Eq);
    const int id = a.takeRequest();
    a.onCaptured(id);
    a.onPartialAnswer(id, 0.9, "x = 3");
    CHECK(a.screen() == Screen::Busy);  // not shown early
    a.onReply(id, "{\"readable\":true,\"confidence\":0.9,\"unclear\":[],\"expression\":\"\",\"choice\":\"\","
                  "\"answer\":\"x = 3\",\"check\":\"3\",\"read_as\":\"solve 2x+1=7\",\"steps\":[\"2x = 6\",\"x = 3\"]}");
    CHECK(a.screen() == Screen::Result);
    snapshot("ai_tutor_0", a);
    a.onKey(Key::Eq);
    snapshot("ai_tutor_1", a);
    a.onKey(Key::Eq);
    a.onKey(Key::Eq);  // past the last step: the answer
    snapshot("ai_tutor_answer", a);
    CHECK(a.takeRequest() == 0);
    a.onKey(Key::Eq);  // now = takes a new photo
    CHECK(a.takeRequest() != 0);
  }
  // Device: 1 on the AI home screen toggles tutor mode.
  {
    Device d;
    d.tick(0);
    d.setOnline(true);
    keys(d, {K::Mode, K::D4, K::D1});
    CHECK(d.ai().tutor());
    keys(d, {K::D1});
    CHECK(!d.ai().tutor());
  }
}

// ---- camera viewfinder (core/viewfinder.*)
static void snapshotPanel(const std::string& name, const Panel& p) {
  const std::string got = p.toAscii();
  const std::string path = "tests/golden/" + name + ".txt";
  if (std::getenv("UPDATE_GOLDEN")) std::ofstream(path, std::ios::binary) << got;
  if (const char* dir = std::getenv("SNAP_DIR")) {
    std::ofstream pbm(std::string(dir) + "/" + name + ".pbm");
    pbm << "P1\n" << Panel::kWidth << " " << Panel::kHeight << "\n";
    for (int y = 0; y < Panel::kHeight; ++y) {
      for (int x = 0; x < Panel::kWidth; ++x) pbm << (p.get(x, y) ? "1 " : "0 ");
      pbm << "\n";
    }
  }
  const bool same = readFile(path) == got;
  if (!same) std::printf("snapshot mismatch: %s\n", name.c_str());
  CHECK(same);
}

// A fake camera: a 160x120 gray "worksheet" with uneven light, a ruled border and
// three lines of character-sized dark strokes around the middle. dx shifts it
// (hand shake); blur > 0 box-blurs it (out of focus).
static std::vector<uint8_t> fakeFrame(int dx = 0, int blur = 0) {
  const int w = 160, h = 120;
  std::vector<uint8_t> g(w * h);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) g[y * w + x] = uint8_t(150 + x * 40 / w + y * 20 / h);  // paper, lit from one side
  auto ink = [&](int x0, int y0, int ww, int hh) {
    for (int y = y0; y < y0 + hh; ++y)
      for (int x = x0 + dx; x < x0 + dx + ww; ++x)
        if (x >= 0 && x < w && y >= 0 && y < h) g[y * w + x] = 40;
  };
  for (int line = 0; line < 3; ++line)
    for (int c = 0; c < 9; ++c) {
      const int x = 34 + c * 10, y = 40 + line * 14;
      ink(x, y, 2, 8);                     // a stem
      ink(x, y + (c % 3) * 3, 6, 2);       // a bar at a different height per "letter"
      if (c % 2) ink(x + 5, y, 2, 8);
    }
  ink(10, 10, 140, 1);  // the worksheet's printed rule
  if (blur > 0) {
    std::vector<uint8_t> o(g);
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x) {
        int sum = 0, n = 0;
        for (int j = -blur; j <= blur; ++j)
          for (int i = -blur; i <= blur; ++i)
            if (x + i >= 0 && x + i < w && y + j >= 0 && y + j < h) sum += g[(y + j) * w + x + i], ++n;
        o[y * w + x] = uint8_t(sum / n);
      }
    g.swap(o);
  }
  return g;
}

static void testViewfinder() {
  Panel p;
  p.set(3, 4, true);
  p.set(249, 121, true);
  CHECK(p.get(3, 4) && p.get(249, 121) && !p.get(4, 4) && !p.get(250, 0));
  p.invert(3, 4);
  CHECK(!p.get(3, 4));
  Panel q;
  CHECK(p.diff(q) == 1);
  CHECK(sizeof(Panel) < 4096);  // bit-packed: fits next to TLS in the ESP32's RAM

  const auto sharp = fakeFrame(), soft = fakeFrame(0, 3), moved = fakeFrame(8);
  const double fs = focusMeasure(sharp.data(), 160, 120, 40, 190);
  const double fb = focusMeasure(soft.data(), 160, 120, 40, 190);
  CHECK(fs > Viewfinder::kSharpMin && fb < Viewfinder::kSharpMin && fs > 3 * fb);

  Viewfinder v;
  v.start(0);
  CHECK(v.running() && !v.timedOut(29000) && v.timedOut(30000));
  v.touch(20000);
  CHECK(!v.timedOut(30000) && v.secondsLeft(30000) == 20);
  v.start(0);
  Panel out;
  v.renderStarting(out);
  snapshotPanel("viewfinder_starting", out);
  CHECK(v.feed(sharp.data(), 160, 120, true, 100).state == Focus3::Starting);
  CHECK(v.takeFullRefresh(out));  // first frame: full refresh
  CHECK(v.feed(sharp.data(), 160, 120, false, 500).state == Focus3::Focusing);  // AF not done yet
  const FrameInfo& ok = v.feed(sharp.data(), 160, 120, true, 900);
  CHECK(ok.state == Focus3::Sharp && ok.motion < 1);
  CHECK(ok.textFound && ok.tx >= 24 && ok.tx <= 40 && ok.tw >= 80 && ok.ty >= 32 && ok.ty + ok.th <= 88);
  v.render(out, 900, "Tutor on");
  snapshotPanel("viewfinder_sharp", out);
  CHECK(v.feed(moved.data(), 160, 120, true, 1300).state == Focus3::Moving);
  v.render(out, 1300);
  snapshotPanel("viewfinder_moving", out);
  v.feed(soft.data(), 160, 120, true, 1700);  // changed a lot from the shaken frame: "moving"
  CHECK(v.feed(soft.data(), 160, 120, true, 2000).state == Focus3::Focusing);
  v.render(out, 1700);
  snapshotPanel("viewfinder_soft", out);
  // A blank wall: no text box, and the stretch doesn't turn noise into a pattern.
  std::vector<uint8_t> flat(160 * 120, 128);
  const FrameInfo& f = v.feed(flat.data(), 160, 120, true, 2100);
  CHECK(!f.textFound && f.hi - f.lo >= 32);

  // Full refreshes: every `fullEvery` frames when the picture barely changes.
  Viewfinder r;
  r.fullEvery = 3;
  r.start(0);
  Panel a;
  int fulls = 0;
  for (int i = 0; i < 8; ++i) fulls += r.takeFullRefresh(a);
  CHECK(fulls == 2);  // frame 1, then after 3 partials (frame 5)
  // ...or sooner when many pixels flipped (ghosting builds up).
  Viewfinder gh;
  gh.start(0);
  gh.takeFullRefresh(a);
  Panel b;
  b.fillRect(0, 0, 162, 122, true);
  int quick = 0;
  for (int i = 0; i < 4; ++i) quick += gh.takeFullRefresh(i % 2 ? a : b);
  CHECK(quick >= 1);
}

int main() {
  testFocus();
  testEnhance();
  testFont();
  testWrap();
  testJson();
  testParse();
  testFlow();
  testClaudeApi();
  testStreamingFlow();
  testEngine();
  testDevice();
  testDeviceAi();
  testExam();
  testStage13();
  testViewfinder();
  std::printf("%d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
