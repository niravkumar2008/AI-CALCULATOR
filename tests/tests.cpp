// Stage 1 tests: font, wrapping, JSON, reply parsing, app flow, and screen
// snapshots. Run with no arguments from the project root.
//   UPDATE_GOLDEN=1  rewrite tests/golden/*.txt from the current renders
//   SNAP_DIR=<dir>   also write each snapshot as a .pbm image
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#include "../core/png.h"
#include "../core/text.h"
#include "../core/viewfinder.h"

using namespace calc;

// Separate suites (tests/geo_tests.cpp, periodic_tests.cpp, engine_regress_tests.cpp, fuzz_tests.cpp): each
// prints its own summary line and returns its number of failures.
int runGeoTests();
int runPeriodicTests();
int runStatTableTests();  // tests/stat_table_tests.cpp
int runNumericTests();    // tests/numeric_tests.cpp
int runWorkerApiTests();  // tests/worker_api_tests.cpp
int runEqnTests();        // tests/eqn_tests.cpp (MODE 5)
int runCmplxTests();      // tests/cmplx_tests.cpp (MODE 6)
int runBaseNTests();      // tests/basen_tests.cpp (MODE 7)
int runMatVecTests();     // tests/matvec_tests.cpp (MODE 8 / 9)

int runEngineRegressTests();

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
  // R-12 (bug hunt 2026-10-11): nesting up to Json::kMaxDepth (20) parses, one more doesn't
  // (was 64), which also bounds the recursion of copying and destroying a tree.
  auto nested = [](int n) { return std::string(static_cast<size_t>(n), '[') + "1" + std::string(static_cast<size_t>(n), ']'); };
  CHECK(Json::parse(nested(Json::kMaxDepth), j, err));
  CHECK(!Json::parse(nested(Json::kMaxDepth + 1), j, err));
  CHECK(Json::parse("{\"a\":" + nested(Json::kMaxDepth - 1) + "}", j, err) && j["a"].isArray());
  {
    Json deep;
    CHECK(Json::parse(nested(Json::kMaxDepth), deep, err));
    std::vector<Json> copies(200, deep);  // copied and destroyed: each 20 deep
    copies.clear();
    Json copy = deep;
    CHECK(copy.items().size() == 1 && copy.items()[0].isArray());
  }
  // R-15: a high surrogate needs a low one (DC00-DFFF) after it; lone surrogates, control
  // characters and invalid UTF-8 bytes come out as '?' (\t \r as a space, \n kept): what is
  // shown is always valid UTF-8.
  CHECK(Json::parse(R"(["😀","\ud83dA","\ude00x","\ud83d","a\u0000b\bc\td\ne"])", j, err));
  CHECK(j.items()[0].asString() == "\xF0\x9F\x98\x80");
  CHECK(j.items()[1].asString() == "?A");
  CHECK(j.items()[2].asString() == "?x");
  CHECK(j.items()[3].asString() == "?");
  CHECK(j.items()[4].asString() == "a?b?c d\ne");
  // a, FF, b, é, E2 82 (cut short), x, C0 AF (overlong), ED A0 80 (an encoded surrogate)
  CHECK(Json::parse("[\"a\xFF" "b\xC3\xA9\xE2\x82x\xC0\xAF\xED\xA0\x80\"]", j, err));
  CHECK(j.items()[0].asString() == "a?b\xC3\xA9??x?????");
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
  // x² (Casio type A) binds tighter than ^: typed 2^3² is 2^(3²) = 512, not (2^3)² = 64.
  CHECK(parseExpression("2^3\xC2\xB2", t) && exprText(t) == "2^(3\xC2\xB2)");
  CHECK(calcText("2^3\xC2\xB2") == "512");
  CHECK(calcText("2^3\xC2\xB2+1") == "513");
  CHECK(calcText("2^3!") == "64");                      // x! likewise: 2^(3!)
  CHECK(calcText("2^(3)\xC2\xB2") == "64");             // an explicit bracket still closes first
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
  keys(d, {K::AC, K::Calc});  // CALC on an empty line: nothing to calculate, nothing happens
  CHECK(d.view() == View::Calc && d.expression().empty());
  keys(d, {K::Shift, K::Integral});  // SHIFT ∫dx types the d/dx( template (testModes() has the rest)
  CHECK(d.view() == View::Calc && exprText(d.expression()) == "d/dx(");
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc && d.expression().empty());
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
  keys(d, {K::D2});  // STAT (testModes() has the rest); AC on its type list: back to COMP
  CHECK(d.view() == View::Stat);
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
  keys(d, {K::Mode, K::D4, K::D1});
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
  keys(d, {K::Mode, K::D4, K::D1});
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
  snapshot("ai_hold_captured", d);   // Solving / Working on it
  keys(d, {K::AC});

  // The preview page's Send starts a scan from any screen: the calculator,
  // a scan still busy, or an answer.
  keys(d, {K::AC, K::AC, K::D1, K::Add, K::D2});  // AI home, then the Apps menu, then the calculator
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
  keys(d, {K::Mode, K::D4, K::D1, K::Eq});
  id = d.takeRequest();
  d.onReply(id, readFile("samples/07_unclear_arithmetic.json"));
  CHECK(d.ai().screen() == Screen::Result);  // a best guess, shown straight away
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc);
  snapshot("scan_unclear", d);
  keys(d, {K::Eq});
  CHECK(d.resultText() == "117\xE2\x8C\x9F" "2");

  // A word problem with no typed calculation leaves the calculator alone.
  keys(d, {K::AC, K::D4, K::D2, K::Mode, K::D4, K::D1, K::Eq});
  id = d.takeRequest();
  d.onReply(id, readFile("samples/02_chem_moles.json"));
  keys(d, {K::AC});
  CHECK(d.view() == View::Ai);  // back to the AI home screen
  keys(d, {K::AC});             // then the Apps menu (MODE 4), then the calculator, expression intact
  CHECK(d.view() == View::Apps);
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc && exprText(d.expression()) == "42");

  // MODE while solving cancels the request.
  keys(d, {K::Mode, K::D4, K::D1, K::Eq});
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
    CHECK(d.saveState(300).size() <= 300 && d.saveState(300).size() < st.size());
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
    keys(d, {K::Mode, K::D4, K::D1, K::D1});  // MODE 4 1: Apps > AI Solve; then 1: tutor
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

// ---------------------------------------------------------------- MODE 4: Apps

static const std::string kPngSig("\x89PNG\r\n\x1a\n", 8);

static uint32_t be32(const std::string& s, size_t p) {
  return (uint32_t(uint8_t(s[p])) << 24) | (uint32_t(uint8_t(s[p + 1])) << 16) | (uint32_t(uint8_t(s[p + 2])) << 8) |
         uint32_t(uint8_t(s[p + 3]));
}

// A strict little PNG reader for what encodePngGray() writes: checks every CRC,
// the IHDR fields, the zlib header, the stored blocks and the Adler-32.
static bool decodePng(const std::string& png, int& w, int& h, std::vector<uint8_t>& px, int& blocks) {
  if (png.compare(0, 8, kPngSig) != 0) return false;
  std::string idat;
  bool ihdr = false, iend = false;
  size_t p = 8;
  while (p + 12 <= png.size()) {
    const uint32_t len = be32(png, p);
    if (p + 12 + len > png.size()) return false;
    const std::string type = png.substr(p + 4, 4);
    const std::string data = png.substr(p + 8, len);
    const uint32_t crc = be32(png, p + 8 + len);
    if (crc != pngCrc32(reinterpret_cast<const uint8_t*>(png.data() + p + 4), len + 4)) return false;
    if (type == "IHDR") {
      if (len != 13) return false;
      w = int(be32(data, 0));
      h = int(be32(data, 4));
      if (data[8] != 8 || data[9] != 0 || data[10] != 0 || data[11] != 0 || data[12] != 0) return false;
      ihdr = true;
    } else if (type == "IDAT") {
      idat += data;
    } else if (type == "IEND") {
      iend = true;
    }
    p += 12 + len;
  }
  if (!ihdr || !iend || p != png.size() || idat.size() < 6) return false;
  const uint8_t cmf = uint8_t(idat[0]), flg = uint8_t(idat[1]);
  if ((cmf & 0x0F) != 8 || (cmf * 256 + flg) % 31 != 0 || (flg & 0x20)) return false;
  std::string raw;
  size_t q = 2;
  blocks = 0;
  for (bool last = false; !last;) {
    if (q + 5 > idat.size()) return false;
    const uint8_t hdr = uint8_t(idat[q]);
    last = hdr & 1;
    if (((hdr >> 1) & 3) != 0) return false;  // stored blocks only
    const unsigned n = uint8_t(idat[q + 1]) | (uint8_t(idat[q + 2]) << 8);
    const unsigned nn = uint8_t(idat[q + 3]) | (uint8_t(idat[q + 4]) << 8);
    if ((n ^ 0xFFFF) != nn || q + 5 + n > idat.size()) return false;
    raw += idat.substr(q + 5, n);
    q += 5 + n;
    ++blocks;
  }
  if (q + 4 != idat.size() || be32(idat, q) != pngAdler32(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()))
    return false;
  if (raw.size() != size_t(w + 1) * h) return false;
  px.clear();
  for (int y = 0; y < h; ++y) {
    if (raw[size_t(y) * (w + 1)] != 0) return false;  // filter: none
    for (int x = 0; x < w; ++x) px.push_back(uint8_t(raw[size_t(y) * (w + 1) + 1 + x]));
  }
  return true;
}

static void testPng() {
  // Known check values: CRC-32 and Adler-32.
  CHECK(pngCrc32(reinterpret_cast<const uint8_t*>("123456789"), 9) == 0xCBF43926u);
  CHECK(pngAdler32(reinterpret_cast<const uint8_t*>("Wikipedia"), 9) == 0x11E60398u);
  // Blank text: no picture, so it can't be sent.
  std::vector<uint8_t> px;
  int w = 0, h = 0;
  CHECK(!renderTextImage("   ", px, w, h) && renderTextPng("") == "" && renderTextPng("  ") == "");
  CHECK(blankText(" ") && !blankText(" 1"));
  // A typed problem: decodes to exactly the pixels drawn, black text on white.
  const std::string text = "SOLVE 2x+1=7 FOR x. (\xE2\x88\x9A" "9\xC3\x97\xCF\x80)";
  CHECK(renderTextImage(text, px, w, h, 3));
  CHECK(w == 2 * 24 + kTypedCols * 15 - 3 && h == 2 * 24 + 2 * 30 - 9);  // 2 lines at 3x
  const std::string png = renderTextPng(text, 3);
  int dw = 0, dh = 0, blocks = 0;
  std::vector<uint8_t> got;
  CHECK(decodePng(png, dw, dh, got, blocks));
  CHECK(dw == w && dh == h && got == px && blocks == 1);
  CHECK(got[0] == 255 && got.back() == 255 && std::count(got.begin(), got.end(), 0) > 300);
  CHECK(std::count(got.begin(), got.end(), 0) + std::count(got.begin(), got.end(), 255) == long(got.size()));
  // Bigger than one stored block (65535 bytes): split, still exact.
  std::vector<uint8_t> big(300 * 300);
  for (size_t i = 0; i < big.size(); ++i) big[i] = uint8_t(i * 7);
  CHECK(decodePng(encodePngGray(big, 300, 300), dw, dh, got, blocks) && got == big && blocks == 2);
  CHECK(encodePngGray(big, 300, 299) == "" && encodePngGray({}, 0, 0) == "");
  // A damaged byte is caught by the CRC.
  std::string bad = png;
  bad[40] ^= 1;
  CHECK(!decodePng(bad, dw, dh, got, blocks));
  // The request marks it as a PNG; photos stay JPEG.
  Json req;
  std::string err;
  CHECK(Json::parse(buildSolveRequest(png), req, err));
  const Json& src = req["messages"].items()[0]["content"].items()[0]["source"];
  CHECK(src["media_type"].asString() == "image/png" && src["data"].asString() == base64Encode(png));
  CHECK(Json::parse(buildSolveRequest("JPEG"), req, err));
  CHECK(req["messages"].items()[0]["content"].items()[0]["source"]["media_type"].asString() == "image/jpeg");
}

static std::string reply(const std::string& answer) {
  return "{\"readable\":true,\"confidence\":0.95,\"unclear\":[],\"expression\":\"\",\"choice\":\"\",\"answer\":\"" +
         answer + "\",\"check\":\"\",\"read_as\":\"Work out " + answer + "\",\"steps\":[\"Add them\",\"So " + answer + "\"]}";
}

// Moves the keyboard's cursor onto SEND / SAVE.
static void toSend(Device& d) {
  for (int i = 0; i < 3 && Device::keyboardCellAt(d.keyboardCell()).row != 2; ++i) d.onKey(K::Up);
  for (int i = 0; i < 3 && Device::keyboardCellAt(d.keyboardCell()).kind != KeyCell::Send; ++i) d.onKey(K::Right);
}

static void testAppsMenu() {
  Device d;
  d.tick(0);
  d.setOnline(true);
  keys(d, {K::D1, K::Add, K::D2});
  keys(d, {K::Mode, K::D4});
  CHECK(d.view() == View::Apps && d.appsSelected() == 0);
  for (int i = 0; i < Device::kAppCount; ++i) {
    snapshot("apps_" + std::to_string(i + 1), d);  // one golden per app
    keys(d, {K::Right});
  }
  CHECK(d.appsSelected() == 0);  // wraps
  keys(d, {K::Left});
  CHECK(d.appsSelected() == 6);
  CHECK(std::string(Device::appName(0)) == "AI SOLVE" && std::string(Device::appName(6)) == "SETTINGS");
  CHECK(Device::appView(1) == View::Keyboard && Device::appView(4) == View::Geo &&
        Device::appView(5) == View::Periodic && Device::appView(7) == View::Off && std::string(Device::appName(7)).empty());
  for (int i = 0; i < Device::kAppCount; ++i)
    CHECK(textLength(Device::appDescription(i)) <= 25 && textLength(Device::appName(i)) <= 21);
  // AC: the calculator, expression intact; the menu remembers the last app.
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc && d.mode() == Mode::Comp && exprText(d.expression()) == "1+2");
  keys(d, {K::Mode, K::D4});
  CHECK(d.view() == View::Apps && d.appsSelected() == 6);
  // 1 opens AI Solve; AC from its home screen comes back to the menu, then the calculator.
  keys(d, {K::D1});
  CHECK(d.view() == View::Ai && d.mode() == Mode::Ai && d.appsSelected() == 0 && d.ai().screen() == Screen::Ready);
  keys(d, {K::AC});
  CHECK(d.view() == View::Apps && d.mode() == Mode::Comp);
  keys(d, {K::Eq});  // = opens the selected one
  CHECK(d.view() == View::Ai);
  keys(d, {K::AC, K::AC});
  CHECK(d.view() == View::Calc && exprText(d.expression()) == "1+2");
  // Geography and Periodic Table (testAppModules() has more): AC on the map / table leaves.
  keys(d, {K::Mode, K::D4, K::D5});
  CHECK(d.view() == View::Geo);
  keys(d, {K::AC});
  CHECK(d.view() == View::Apps);
  keys(d, {K::D6});
  CHECK(d.view() == View::Periodic);
  keys(d, {K::AC, K::D8, K::D0});  // no app 8; 0 does nothing
  CHECK(d.view() == View::Apps && d.appsSelected() == 5);
  // MODE from inside an app, then AC: back in the app.
  keys(d, {K::D7, K::Mode});
  CHECK(d.view() == View::ModeMenu);
  keys(d, {K::AC});
  CHECK(d.view() == View::Settings);
  keys(d, {K::AC, K::AC});
  CHECK(d.view() == View::Calc);
  // MODE 1 still goes straight to the calculator.
  keys(d, {K::Mode, K::D1});
  CHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
}

static void testTypeIt() {
  // App: a typed image goes out instead of a photo, once per request; retry resends it.
  {
    App a;
    CHECK(!a.startTyped("") && a.takeRequest() == 0);
    CHECK(a.startTyped(kPngSig + "x"));
    int id = a.takeRequest();
    std::string png;
    CHECK(id != 0 && a.screen() == Screen::Busy && a.takeTypedImage(png) && png == kPngSig + "x");
    CHECK(!a.takeTypedImage(png));
    a.onFailure(id, Failure::NoConnection);
    a.onKey(Key::Eq);  // retry
    id = a.takeRequest();
    CHECK(id != 0 && a.takeTypedImage(png) && png == kPngSig + "x");
    a.onReply(id, reply("4"));
    CHECK(a.screen() == Screen::Result);
    a.onKey(Key::AC);
    a.clearTyped();
    a.onKey(Key::Eq);  // a photo again
    CHECK(a.takeRequest() != 0 && !a.takeTypedImage(png));
  }

  Device k;
  k.tick(0);
  k.setOnline(true);
  k.setTypedSolveSupported(true);  // as the v15 (B-9: off by default)
  keys(k, {K::Mode, K::D4, K::D2});
  CHECK(k.view() == View::Keyboard && !k.keyboardForNote() && k.keyboardText().empty());
  snapshot("keyboard_empty", k);
  CHECK(Device::keyboardCellCount() == 53);
  // SEND with nothing typed (or only spaces) sends nothing.
  toSend(k);
  CHECK(std::string(Device::keyboardCellAt(k.keyboardCell()).label) == "SEND");
  snapshot("keyboard_send_cursor", k);
  keys(k, {K::Eq});
  CHECK(k.view() == View::Keyboard && k.takeRequest() == 0 && k.toastVisible() && k.toastText() == "Type a problem first");
  k.tick(Device::kToastMs);  // the toast goes
  keys(k, {K::Left, K::Left, K::Eq});  // SPACE
  CHECK(k.keyboardText() == " ");
  keys(k, {K::Right, K::Right, K::Eq});
  CHECK(k.view() == View::Keyboard && k.takeRequest() == 0);
  keys(k, {K::Del});
  CHECK(k.keyboardText().empty());
  // The cursor: along a row wrapping, up and down to the nearest cell.
  keys(k, {K::Down});  // from SEND (row 2) to row 0, near column 19
  CHECK(Device::keyboardCellAt(k.keyboardCell()).row == 0);
  for (int i = 0; i < 30 && k.keyboardCell() != 0; ++i) keys(k, {K::Left});
  CHECK(k.keyboardCell() == 0);
  keys(k, {K::Eq, K::Left});
  CHECK(k.keyboardText() == "A" && std::string(Device::keyboardCellAt(k.keyboardCell()).label) == "Y");
  keys(k, {K::Right, K::Up});
  CHECK(std::string(Device::keyboardCellAt(k.keyboardCell()).label) == "SPACE");
  keys(k, {K::Eq, K::Down});
  CHECK(std::string(Device::keyboardCellAt(k.keyboardCell()).label) == "D");
  keys(k, {K::Down, K::Down, K::Down});  // round the three rows
  CHECK(std::string(Device::keyboardCellAt(k.keyboardCell()).label) == "D");
  // Calculator keys type themselves; DEL erases.
  keys(k, {K::D2, K::Mul, K::D3, K::Add, K::D4, K::Sqrt, K::Shift, K::Exp10, K::Del});
  CHECK(k.keyboardText() == "A 2\xC3\x97" "3+4\xE2\x88\x9A");
  for (int i = 0; i < 20; ++i) keys(k, {K::D7});
  k.tick(5000);  // the last toast has gone
  snapshot("keyboard_text", k);
  // At most 160 characters.
  for (int i = 0; i < 200; ++i) keys(k, {K::D1});
  CHECK(textLength(k.keyboardText()) == Device::kMaxText && k.toastText() == "160 characters max");
  for (int i = 0; i < 200; ++i) keys(k, {K::Del});
  CHECK(k.keyboardText().empty());
  // AC with text asks first; AC there keeps the text.
  keys(k, {K::D6, K::Mul, K::D7, K::AC});
  CHECK(k.view() == View::Confirm && k.confirmQuestion() == "Discard text?");
  snapshot("keyboard_discard", k);
  keys(k, {K::AC});
  CHECK(k.view() == View::Keyboard && k.keyboardText() == "6\xC3\x97" "7");
  // SEND: the platform gets an id and a PNG instead of a photo.
  toSend(k);
  keys(k, {K::Eq});
  CHECK(k.view() == View::Ai && k.mode() == Mode::Ai && k.ai().typed());
  const int id = k.takeRequest();
  std::string png;
  CHECK(id != 0 && k.takeTypedImage(png) && png.compare(0, 8, kPngSig) == 0 && !k.takeTypedImage(png));
  int w, h, blocks;
  std::vector<uint8_t> px, want;
  CHECK(decodePng(png, w, h, px, blocks) && renderTextImage("6\xC3\x97" "7", want, w, h) && px == want);
  snapshot("typed_busy", k);  // no "hold still": there's no photo
  k.onReply(id, reply("42"));
  CHECK(k.ai().screen() == Screen::Result && k.savedAnswers().size() == 1 && k.savedAnswers()[0].answer == "42");
  // AC: back to the text, to edit or send again.
  keys(k, {K::AC});
  CHECK(k.view() == View::Keyboard && k.keyboardText() == "6\xC3\x97" "7" && !k.ai().typed());
  // Leaving and coming back keeps it; discarding clears it.
  keys(k, {K::AC, K::Eq});
  CHECK(k.view() == View::Apps);
  keys(k, {K::D2});
  CHECK(k.keyboardText().empty());
  keys(k, {K::AC});
  CHECK(k.view() == View::Apps);  // nothing typed: no question
  // A typed problem that fails can be retried with =, and AC goes back to the text.
  keys(k, {K::D2, K::D9});
  toSend(k);
  keys(k, {K::Eq});
  const int id2 = k.takeRequest();
  k.onFailure(id2, Failure::Timeout);
  CHECK(k.savedAnswers().size() == 1);  // failures aren't kept
  keys(k, {K::Eq});
  CHECK(k.takeRequest() != 0 && k.takeTypedImage(png));
  keys(k, {K::AC});  // cancels it
  CHECK(k.view() == View::Keyboard && k.activeRequest() == 0 && k.keyboardText() == "9");
  // startScan (the preview page's Send) is a photo again.
  CHECK(k.startScan() && k.takeRequest() != 0 && !k.takeTypedImage(png));
}

static void testHistory() {
  Device h;
  h.tick(0);
  h.setOnline(true);
  keys(h, {K::Mode, K::D4, K::D3});
  CHECK(h.view() == View::History && h.savedAnswers().empty());
  snapshot("history_empty", h);
  keys(h, {K::Eq, K::Down, K::AC});  // nothing to open
  CHECK(h.view() == View::Apps);
  // Answers are kept newest first; failures and unreadable replies are not.
  for (int i = 1; i <= 3; ++i) {
    CHECK(h.startScan());
    h.onReply(h.takeRequest(), reply("x = " + std::to_string(i)));
  }
  CHECK(h.startScan());
  h.onFailure(h.takeRequest(), Failure::ApiBusy);
  CHECK(h.startScan());
  h.onReply(h.takeRequest(), "not json");
  CHECK(h.startScan());
  h.onReply(h.takeRequest(), readFile("samples/05_not_a_problem.json"));
  CHECK(h.savedAnswers().size() == 3 && h.savedAnswers()[0].answer == "x = 3" && h.savedAnswers()[2].answer == "x = 1");
  const uint32_t rev = h.appsRevision();
  keys(h, {K::Mode, K::D4, K::D3});
  CHECK(h.view() == View::History && h.historySelected() == 0);
  snapshot("history_list", h);
  // = shows one again, read-only: = sends nothing; AC goes back to the list.
  keys(h, {K::Down, K::Eq});
  CHECK(h.view() == View::Ai && h.ai().readOnly() && h.ai().screen() == Screen::Result && h.activeRequest() == 0);
  snapshot("history_answer", h);
  keys(h, {K::Eq});
  CHECK(h.takeRequest() == 0 && h.ai().screen() == Screen::Result);
  keys(h, {K::AC});
  CHECK(h.view() == View::History && h.historySelected() == 1);
  // "Clear history" is last, with a question.
  keys(h, {K::Down, K::Down, K::Down, K::Eq});
  CHECK(h.historySelected() == 3 && h.view() == View::Confirm && h.confirmQuestion() == "Clear history?");
  keys(h, {K::AC});
  CHECK(h.view() == View::History && h.savedAnswers().size() == 3 && h.appsRevision() == rev);
  keys(h, {K::Eq, K::Eq});
  CHECK(h.view() == View::History && h.savedAnswers().empty() && h.appsRevision() != rev);
  CHECK(h.toastVisible() && h.toastText() == "History cleared");
  snapshot("toast_history_cleared", h);
  h.tick(Device::kToastMs - 1);
  CHECK(h.toastVisible());
  h.tick(Device::kToastMs);
  CHECK(!h.toastVisible());
  // At most 20, the oldest go.
  for (int i = 1; i <= 25; ++i) {
    h.startScan();
    h.onReply(h.takeRequest(), reply(std::to_string(i)));
  }
  CHECK(h.savedAnswers().size() == 20 && h.savedAnswers()[0].answer == "25" && h.savedAnswers()[19].answer == "6");
  keys(h, {K::Mode, K::D4, K::D3});
  for (int i = 0; i < 30; ++i) keys(h, {K::Down});
  CHECK(h.historySelected() == 20 && h.listTop() == 16);
  // The platform clears it when the calculator is unlinked.
  h.clearHistory();
  CHECK(h.savedAnswers().empty());
  // Multiple choice: the list shows the label.
  h.startScan();
  h.onReply(h.takeRequest(), readFile("samples/08_multiple_choice.json"));
  CHECK(h.savedAnswers().size() == 1 && h.savedAnswers()[0].answer.rfind("(C)", 0) == 0);
}

static void testNotes() {
  Device n;
  n.tick(0);
  keys(n, {K::Mode, K::D4, K::D4});
  CHECK(n.view() == View::Notes && n.notesSelected() == 0 && n.notes().empty());
  // + New note: the keyboard, SAVE instead of SEND; an empty note isn't saved.
  keys(n, {K::Eq});
  CHECK(n.view() == View::Keyboard && n.keyboardForNote());
  toSend(n);
  snapshot("keyboard_note", n);
  keys(n, {K::Eq});
  CHECK(n.view() == View::Keyboard && n.notes().empty() && n.toastText() == "Type a note first");
  keys(n, {K::AC});
  CHECK(n.view() == View::Notes);  // nothing typed: no question
  keys(n, {K::Eq, K::D1, K::D2, K::D3});
  CHECK(n.view() == View::Keyboard && n.keyboardCell() == Device::keyboardCellCount() - 1);  // still on SAVE
  keys(n, {K::Eq});
  CHECK(n.view() == View::Notes && n.notes().size() == 1 && n.notes()[0] == "123" && n.toastText() == "Saved");
  keys(n, {K::Up, K::Eq, K::AC});  // a new note with nothing typed
  keys(n, {K::Eq, K::D4, K::D5, K::Mul, K::D2, K::AC});
  CHECK(n.view() == View::Confirm && n.confirmQuestion() == "Discard note?");
  keys(n, {K::AC, K::Eq});  // no, then SAVE
  CHECK(n.notes().size() == 2 && n.notes()[0] == "45\xC3\x97" "2" && n.notesSelected() == 1);
  n.tick(5000);
  keys(n, {K::Up});
  snapshot("notes_list", n);
  // Open one: 1 edits, 2 deletes.
  keys(n, {K::Down, K::Down, K::Eq});
  CHECK(n.view() == View::NoteView && n.noteOpen() == 1);
  snapshot("note_view", n);
  keys(n, {K::D1});
  CHECK(n.view() == View::Keyboard && n.keyboardText() == "123");
  keys(n, {K::Del, K::D4, K::AC});
  CHECK(n.view() == View::Confirm && n.confirmQuestion() == "Discard changes?");
  keys(n, {K::Eq});  // discard: back to the note, unchanged
  CHECK(n.view() == View::NoteView && n.notes()[1] == "123");
  keys(n, {K::D1, K::Del, K::D4, K::Eq});
  CHECK(n.view() == View::Notes && n.notes()[1] == "124" && n.notes().size() == 2);
  keys(n, {K::Eq, K::D2});
  CHECK(n.view() == View::Confirm && n.confirmQuestion() == "Delete this note?");
  keys(n, {K::AC});
  CHECK(n.view() == View::NoteView && n.notes().size() == 2);
  keys(n, {K::D2, K::Eq});
  CHECK(n.view() == View::Notes && n.notes().size() == 1 && n.notes()[0] == "45\xC3\x97" "2" && n.toastText() == "Deleted");
  // A long note scrolls in the note view.
  keys(n, {K::Up, K::Up, K::Eq});
  for (int i = 0; i < 170; ++i) keys(n, {K::D8});
  keys(n, {K::Eq});
  CHECK(n.notes().size() == 2 && textLength(n.notes()[0]) == 160);
  keys(n, {K::Eq});
  CHECK(n.view() == View::NoteView && n.noteOpen() == 0);
  keys(n, {K::Down, K::Down, K::Down, K::Down, K::Down});
  CHECK(n.noteScroll() == 3);  // 7 lines, 4 on screen
  keys(n, {K::AC, K::AC});
  CHECK(n.view() == View::Apps);
  // At most 10.
  keys(n, {K::D4});
  for (int i = 0; i < 12; ++i) keys(n, {K::Up, K::Eq, K::D1, K::Eq});
  CHECK(n.notes().size() == 10 && n.view() == View::Notes);
  keys(n, {K::Up, K::Eq});
  CHECK(n.view() == View::Notes && n.toastText() == "Notes full");
}

static void testSettingsApp() {
  Device s;
  s.tick(0);
  s.setOnline(true);
  s.setBattery(85, true);
  s.setVersion("2026.10.11");
  keys(s, {K::Mode, K::D4, K::D7});
  CHECK(s.view() == View::Settings && s.settingsSelected() == 0 && Device::settingsCount() == 8);
  CHECK(std::string(Device::settingsItem(0)) == "Brightness" && std::string(Device::settingsItem(7)) == "About");
  CHECK(std::string(Device::settingsItem(3)) == "Link calculator" && std::string(Device::settingsItem(8)).empty());
  snapshot("settings_list", s);
  // Brightness and the sleep timer: ◀ ▶ on their rows, for the platform to read.
  CHECK(s.brightness() == 70);
  keys(s, {K::Right});
  CHECK(s.brightness() == 80);
  for (int i = 0; i < 12; ++i) keys(s, {K::Left});
  CHECK(s.brightness() == 10);
  for (int i = 0; i < 12; ++i) keys(s, {K::Right});
  CHECK(s.brightness() == 100);
  s.setBrightness(54);
  CHECK(s.brightness() == 50);
  s.setBrightness(0);
  CHECK(s.brightness() == 10);
  keys(s, {K::Down, K::Right});
  CHECK(s.settingsSelected() == 1 && s.sleepMinutes() == 10);
  keys(s, {K::Left, K::Left, K::Left, K::Left, K::Left});
  CHECK(s.sleepMinutes() == 1);
  s.setSleepMinutes(7);
  CHECK(s.sleepMinutes() == 5);
  s.setSleepMinutes(100);
  CHECK(s.sleepMinutes() == 30);
  // The existing SETUP screens, coming back to Settings.
  keys(s, {K::D3});
  CHECK(s.view() == View::NetInfo);
  keys(s, {K::AC});
  CHECK(s.view() == View::Settings && s.settingsSelected() == 2);
  // Link calculator: the platform's code, big, and where to type it; it can change while shown.
  keys(s, {K::D4});
  CHECK(s.view() == View::Link && s.settingsSelected() == 3);
  snapshot("settings_link_none", s);  // no code yet: connect to Wi-Fi
  s.setLinkInfo("K7Q4ZP", "https://proofline.app/");
  CHECK(s.linkCode() == "K7Q4ZP" && s.linkUrl() == "https://proofline.app/" && !s.linked());
  snapshot("settings_link", s);
  s.setLinkInfo("", "https://proofline.app", true);
  CHECK(s.linked() && s.linkCode().empty());
  snapshot("settings_link_linked", s);
  // The plan line from the AI server (worker_api.h planStatusText), only while linked.
  s.setLinkPlan("Free month over: subscribe at proofline.app");
  CHECK(s.linkPlan() == "Free month over: subscribe at proofline.app");
  snapshot("settings_link_plan", s);
  s.setLinkInfo("K7Q4ZP", "https://proofline.app/");  // unlinked: the code screen, no plan line
  snapshot("settings_link", s);
  s.setLinkInfo("", "https://proofline.app", true);
  s.setLinkPlan("");
  snapshot("settings_link_linked", s);
  keys(s, {K::Eq});
  CHECK(s.view() == View::Settings && s.settingsSelected() == 3);
  keys(s, {K::D4, K::AC});
  CHECK(s.view() == View::Settings);
  keys(s, {K::D5, K::D1});
  CHECK(s.view() == View::Settings && !s.dotMatrix());
  keys(s, {K::D5, K::D2});
  CHECK(s.dotMatrix());
  keys(s, {K::D7});
  CHECK(s.view() == View::Notice);
  snapshot("settings_battery", s);
  keys(s, {K::AC, K::D8});
  CHECK(s.view() == View::Notice);
  snapshot("settings_about", s);
  keys(s, {K::AC});
  CHECK(s.view() == View::Settings && s.listTop() == 3);  // About is row 8 of 8: the list scrolled
  // SETUP's own pages still go back to SETUP.
  keys(s, {K::AC, K::AC, K::Shift, K::Mode, K::D6, K::AC});
  CHECK(s.view() == View::Setup);
  keys(s, {K::AC});
  CHECK(s.view() == View::Calc);
}

static void testAppsExam() {
  Device e;
  e.tick(0);
  e.setOnline(true);
  e.startScan();
  e.onReply(e.takeRequest(), reply("7"));
  keys(e, {K::Mode, K::D4, K::D4, K::Eq, K::D5});  // a note "5"
  toSend(e);
  keys(e, {K::Eq});
  CHECK(e.notes().size() == 1 && e.savedAnswers().size() == 1);
  // Starting exam mode from inside an app (Settings > Exam mode) closes it.
  keys(e, {K::Mode, K::D4, K::D7, K::D6});
  CHECK(e.view() == View::ExamInfo);
  keys(e, {K::Eq});
  CHECK(e.examActive() && e.view() == View::Notice);
  keys(e, {K::AC});
  CHECK(e.view() == View::Calc && e.mode() == Mode::Comp);
  // MODE 4: "APPS ARE OFF", nothing opens, not even with a digit or =.
  keys(e, {K::Mode});
  snapshot("menu_mode_exam", e);
  keys(e, {K::D4});
  CHECK(e.view() == View::Notice);
  snapshot("apps_off_exam", e);
  keys(e, {K::D3, K::Eq});
  CHECK(e.view() == View::Calc);
  keys(e, {K::Mode, K::D4, K::AC});
  CHECK(e.view() == View::Calc);
  // Notes and history are kept (not readable), and still saved.
  CHECK(e.notes().size() == 1 && e.savedAnswers().size() == 1);
  Device f;
  CHECK(f.restoreApps(e.saveApps()) && f.notes().size() == 1);
  // After a restart into exam mode, no app stays open.
  Device g;
  g.tick(0);
  keys(g, {K::Mode, K::D4, K::D4});
  CHECK(g.view() == View::Notes);
  g.restoreExam(1000);
  CHECK(g.examActive() && g.view() == View::Calc);
  // Exam mode starting while a typed problem is being solved cancels it.
  Device t;
  t.tick(0);
  t.setOnline(true);
  t.setTypedSolveSupported(true);
  keys(t, {K::Mode, K::D4, K::D2, K::D1});
  toSend(t);
  keys(t, {K::Eq});
  CHECK(t.takeRequest() != 0 && t.view() == View::Ai);
  keys(t, {K::Shift, K::AC, K::Shift, K::D7, K::On});
  CHECK(t.examActive() && t.activeRequest() == 0 && !t.ai().typed());
  keys(t, {K::AC});
  CHECK(t.view() == View::Calc);
  // When it ends, everything is there again.
  e.usbUnlock();
  keys(e, {K::AC, K::Mode, K::D4, K::D4});
  CHECK(e.view() == View::Notes && e.notes().size() == 1);
  keys(e, {K::AC, K::D3});
  CHECK(e.view() == View::History && e.savedAnswers().size() == 1);
}

// Geography and Periodic Table inside the Device: keys, MODE, SHIFT, leaving and
// coming back, the quiz's seed, and exam mode closing them.
static void testAppModules() {
  using GeoScreen = geo::GeoApp::Screen;
  using PScreen = PeriodicApp::Screen;
  Device d;
  d.tick(0);
  d.setRandomSource(fixedRandom);
  keys(d, {K::Mode, K::D4, K::D5});
  CHECK(d.view() == View::Geo && d.geo().screen() == GeoScreen::Map);
  const int france = geo::findIso("FRA");
  CHECK(france >= 0 && d.geo().selected() == france);
  keys(d, {K::Right, K::D1});  // the next country east, zoomed in to its continent
  const int sel = d.geo().selected();
  CHECK(sel != france && d.geo().zoom() == 1);
  snapshot("apps_geo", d);
  keys(d, {K::Eq});
  CHECK(d.geo().screen() == GeoScreen::Card);
  snapshot("geo_card", d);
  keys(d, {K::AC});
  CHECK(d.view() == View::Geo && d.geo().screen() == GeoScreen::Map);
  // MODE opens the MODE menu; AC from it goes back into the app, where it was.
  keys(d, {K::Mode});
  CHECK(d.view() == View::ModeMenu);
  keys(d, {K::AC});
  CHECK(d.view() == View::Geo && d.geo().selected() == sel && d.geo().zoom() == 1);
  // The quiz: seeded from the platform's random source on the first open.
  keys(d, {K::D5});
  CHECK(d.geo().screen() == GeoScreen::Quiz);
  snapshot("geo_quiz", d);
  {
    geo::GeoApp ref;
    ref.setSeed(static_cast<uint32_t>(fixedRandom() * 4294967296.0));
    ref.reset();
    ref.onKey(K::D5);
    CHECK(ref.quiz().subject == d.geo().quiz().subject && ref.quiz().correct == d.geo().quiz().correct);
    // Without a source: GeoApp's fixed seed, the same every time.
    Device n;
    n.tick(0);
    n.setRandomSource(nullptr);
    keys(n, {K::Mode, K::D4, K::D5, K::D5});
    geo::GeoApp fixed;
    fixed.reset();
    fixed.onKey(K::D5);
    CHECK(n.geo().screen() == GeoScreen::Quiz && fixed.quiz().subject == n.geo().quiz().subject);
  }
  // AC: quiz -> map -> the Apps menu; opening it again comes back to the same country.
  keys(d, {K::AC, K::AC});
  CHECK(d.view() == View::Apps && d.appsSelected() == 4);
  keys(d, {K::Eq});
  CHECK(d.view() == View::Geo && d.geo().screen() == GeoScreen::Map && d.geo().selected() == sel);
  keys(d, {K::AC});

  // Periodic Table: H -> Li -> Na -> K, then along to Fe.
  keys(d, {K::D6});
  CHECK(d.view() == View::Periodic && d.periodic().screen() == PScreen::Table && d.periodic().cursor() == 1);
  keys(d, {K::Down, K::Down, K::Down});
  CHECK(d.periodic().cursor() == 19);
  for (int i = 0; i < 7; ++i) keys(d, {K::Right});
  CHECK(d.periodic().cursor() == 26);
  snapshot("periodic_table", d);
  keys(d, {K::Eq});
  CHECK(d.periodic().screen() == PScreen::Detail);
  snapshot("periodic_detail", d);
  keys(d, {K::AC});
  CHECK(d.view() == View::Periodic && d.periodic().screen() == PScreen::Table);
  // SHIFT latches in the Device (S in the status bar) and reaches the app with the next key.
  keys(d, {K::D4});
  CHECK(d.periodic().filter() == Filter::AlkaliMetal);
  {
    Framebuffer before, after;
    d.render(before);
    keys(d, {K::Shift});
    d.render(after);
    CHECK(before.toAscii() != after.toAscii() && d.view() == View::Periodic);  // the S
  }
  keys(d, {K::D4});
  CHECK(d.periodic().filter() == Filter::All);  // SHIFT 4: the previous filter
  keys(d, {K::D5});
  CHECK(d.periodic().screen() == PScreen::Graph);
  snapshot("periodic_graph", d);
  keys(d, {K::Shift, K::Right});
  CHECK(d.periodic().cursor() == 36);  // SHIFT ▶: 10 at a time
  keys(d, {K::Mode, K::AC});
  CHECK(d.view() == View::Periodic && d.periodic().screen() == PScreen::Graph);
  keys(d, {K::AC, K::AC});
  CHECK(d.view() == View::Apps);
  keys(d, {K::D6});
  CHECK(d.view() == View::Periodic && d.periodic().cursor() == 36);
  // ON: the calculator, like everywhere else.
  keys(d, {K::On});
  CHECK(d.view() == View::Calc);

  // Exam mode closes them: SETUP 8 from inside the app.
  keys(d, {K::Mode, K::D4, K::D5});
  CHECK(d.view() == View::Geo);
  keys(d, {K::Shift, K::Mode});
  CHECK(d.view() == View::Setup);
  keys(d, {K::D8, K::Eq});
  CHECK(d.examActive() && d.view() == View::Notice);
  keys(d, {K::AC});
  CHECK(d.view() == View::Calc);
  keys(d, {K::Mode, K::D4});
  CHECK(d.view() == View::Notice);  // APPS ARE OFF
  keys(d, {K::D5, K::AC});
  CHECK(d.view() == View::Calc);
  // A restart into exam mode doesn't leave one open either.
  Device g;
  g.tick(0);
  keys(g, {K::Mode, K::D4, K::D6});
  CHECK(g.view() == View::Periodic);
  g.restoreExam(1000);
  CHECK(g.examActive() && g.view() == View::Calc);
  // After exam mode, the Apps menu and the apps are back.
  d.usbUnlock();
  keys(d, {K::AC, K::Mode, K::D4});
  CHECK(d.view() == View::Apps && !d.examActive());
  snapshot("apps_after_exam", d);
  keys(d, {K::Eq});
  CHECK(d.view() == View::Geo && d.geo().selected() == sel);
}

static void testAppsPersistence() {
  Device a;
  a.tick(0);
  a.setOnline(true);
  for (int i = 1; i <= 3; ++i) {
    a.startScan();
    a.onReply(a.takeRequest(), reply("y = " + std::to_string(i) + " \xE2\x88\x9A"));
  }
  keys(a, {K::Mode, K::D4, K::D4, K::Eq, K::D4, K::D2});
  toSend(a);
  keys(a, {K::Eq, K::AC, K::AC, K::Mode, K::D4, K::Right});
  CHECK(a.notes().size() == 1 && a.appsSelected() == 4);
  const std::string blob = a.saveApps();
  Device b;
  CHECK(b.restoreApps(blob));
  CHECK(b.notes() == a.notes() && b.appsSelected() == 4 && b.savedAnswers().size() == 3);
  CHECK(b.savedAnswers()[0].answer == a.savedAnswers()[0].answer &&
        b.savedAnswers()[2].replyJson == a.savedAnswers()[2].replyJson);
  CHECK(b.saveApps() == blob);
  // A restored answer opens like any other.
  b.tick(0);
  keys(b, {K::Mode, K::D4, K::D3, K::Eq});
  CHECK(b.view() == View::Ai && b.ai().readOnly() && b.ai().result().answer == "y = 3 \xE2\x88\x9A");
  // Damaged data: false, nothing changes.
  Device c;
  CHECK(!c.restoreApps("") && !c.restoreApps("garbage") && !c.restoreApps(blob.substr(0, blob.size() - 1)));
  for (size_t i = 0; i < blob.size(); i += 7) {
    std::string bad = blob;
    bad[i] ^= 0x40;
    CHECK(!c.restoreApps(bad));
  }
  CHECK(c.notes().empty() && c.savedAnswers().empty() && c.appsSelected() == 0);
  // Valid checksum but impossible contents (app 9, then 30 answers): refused.
  auto sealed = [](std::string body) {
    const uint32_t s = pngAdler32(reinterpret_cast<const uint8_t*>(body.data()), body.size());
    for (int i = 0; i < 4; ++i) body += char((s >> (8 * i)) & 0xFF);
    return body;
  };
  CHECK(!c.restoreApps(sealed(std::string("\x01\x09\x00\x00", 4))));
  CHECK(!c.restoreApps(sealed(std::string("\x01\x00\x1E\x00", 4))));
  CHECK(!c.restoreApps(sealed(std::string("\x02\x00\x00\x00", 4))));  // a newer version
  CHECK(c.restoreApps(sealed(std::string("\x01\x02\x00\x00", 4))) && c.appsSelected() == 2);
  // History and notes survive saveState()/restoreState() untouched (they aren't in it).
  CHECK(a.saveState().size() <= 2048);
  CHECK(b.restoreState(a.saveState()) && b.notes().size() == 1);
}

// Regressions: F-10, F-12, F-13, F-6.
static void testDeviceFixes() {
  Device d;
  d.tick(0);
  // F-10: SHIFT ▲ ▼ is the platform's (brightness), not replay.
  keys(d, {K::D2, K::Eq, K::D3, K::Eq, K::Shift, K::Up});
  CHECK(d.resultText() == "3");
  keys(d, {K::Shift, K::Down, K::Up});
  CHECK(d.resultText() == "2");
  // F-12: ALPHA then a key with no letter does that key's job.
  keys(d, {K::AC, K::D1, K::D2, K::Alpha, K::AC});
  CHECK(d.expression().empty());
  keys(d, {K::D1, K::D2, K::Alpha, K::Del});
  CHECK(exprText(d.expression()) == "1");
  keys(d, {K::Add, K::D4, K::Alpha, K::Eq});
  CHECK(d.resultText() == "5");
  keys(d, {K::AC, K::D7, K::Alpha, K::Sqrt});  // ALPHA √: nothing (no letter there)
  CHECK(exprText(d.expression()) == "7");
  // F-13: SHIFT ENG to a mantissa below 1 keeps 10 significant digits.
  keys(d, {K::AC, K::D1, K::Div, K::D3, K::Eq, K::SD, K::Shift, K::Eng});
  CHECK(d.resultText() == "0.3333333333\xC3\x97" "10\xE2\x81\xB0");
  // ∫dx types "∫(" (the old notice's title was double-encoded UTF-8: the glyph must draw).
  keys(d, {K::AC, K::Integral});
  CHECK(d.view() == View::Calc && exprText(d.expression()) == "\xE2\x88\xAB(");
  snapshot("integral_typed", d);
  keys(d, {K::AC});
  // F-6: a very long sleep can't wrap the exam clock round to "still on".
  Device x;
  x.tick(0);
  keys(x, {K::Shift, K::AC, K::Shift, K::D7, K::On});
  x.tick(100000);
  keys(x, {K::AC, K::Shift, K::AC});
  CHECK(x.examActive() && x.examElapsedMs() == 100000);
  const std::string st = x.saveState();
  Device y;
  y.tick(0);
  CHECK(y.restoreState(st, 0xFFFFFF00u) && !y.examActive() && y.examExpiredOnRestore());
  Device z;
  z.tick(0);
  CHECK(z.restoreState(st, 60000) && z.examActive() && !z.examExpiredOnRestore());
  CHECK(z.restoreState(st, Device::kExamMaxMs) && !z.examActive() && z.examExpiredOnRestore());
}

// ---- Bugs the fuzz campaign found (B-1 ... B-11; tools/fuzz/README.md) ----

// A saveState() with one Num replaced (slot 0 = A ... 9 = Ans): layout 11 header bytes, then
// 11 Nums of 25 bytes (double v, exact, num, den).
// A saveState() blob (version 2) with its last 4 bytes, the Adler-32, recomputed after an edit.
static std::string stateResigned(std::string s) {
  s.resize(s.size() - 4);
  const uint32_t a = pngAdler32(reinterpret_cast<const uint8_t*>(s.data()), s.size());
  for (int i = 0; i < 4; ++i) s += static_cast<char>((a >> (8 * i)) & 0xFF);
  return s;
}

static std::string stateWithNum(const std::string& st, int slot, double v, bool exact, long long num, long long den) {
  std::string s = st;
  const size_t off = 11 + static_cast<size_t>(slot) * 25;
  uint64_t bits;
  std::memcpy(&bits, &v, 8);
  for (int i = 0; i < 8; ++i) s[off + i] = static_cast<char>((bits >> (8 * i)) & 0xFF);
  s[off + 8] = static_cast<char>(exact ? 1 : 0);
  for (int i = 0; i < 8; ++i) s[off + 9 + i] = static_cast<char>((static_cast<uint64_t>(num) >> (8 * i)) & 0xFF);
  for (int i = 0; i < 8; ++i) s[off + 17 + i] = static_cast<char>((static_cast<uint64_t>(den) >> (8 * i)) & 0xFF);
  if (s[0] == 2) s = stateResigned(s);  // version 2 ends with an Adler-32 of the rest
  return s;
}

static bool validUtf8(const std::string& s) { return encodeUtf8(decodeUtf8(s)) == s; }

// Recalls every memory and does maths with it, as the persistence fuzzer did: an exact
// LLONG_MIN trapped here (integer overflow in gcd / division). Returns the shown texts.
static std::string useMemories(Device& d) {
  static const DKey vars[] = {K::Neg, K::Dms, K::Hyp, K::Sin, K::Cos, K::Tan, K::Close, K::SD, K::MPlus};
  std::string shown;
  Framebuffer fb;
  keys(d, {K::On});
  for (DKey v : vars) {
    keys(d, {K::Rcl, v});
    shown += d.resultText() + "|";
    keys(d, {K::AC, K::Alpha, v, K::Mul, K::D3, K::Add, K::Alpha, v, K::Div, K::D7, K::Eq, K::SD, K::Eng, K::AC,
             K::Shift, K::Hyp, K::Alpha, v, K::Eq});
    shown += d.resultText() + "|";
    d.render(fb);
    keys(d, {K::AC, K::Ans, K::Div, K::Ans, K::Eq});
    shown += d.resultText() + "|";
  }
  return shown;
}

static void testFuzzFindings() {
  Device src;
  src.tick(0);
  const std::string st = src.saveState();
  CHECK(st.size() == 11 + 11 * 25 + 1 + 4);  // + the Adler-32 (state version 2)

  // B-1 / B-7: restoreState() refuses a Num the calculator can't make (it trapped or showed "nan").
  {
    const std::string crash = readFile("tests/fuzz/crash_state_llong_min.bin");
    CHECK(crash.size() == 287);
    {
      Device c;
      c.tick(5);
      if (c.restoreState(crash)) {
        const std::string shown = useMemories(c);  // survived (it used to trap)
        CHECK(shown.find("nan") == std::string::npos && shown.find("inf") == std::string::npos);
      }
    }
    struct Bad { double v; bool exact; long long num, den; };
    const Bad bad[] = {
        {-9.2e18, true, LLONG_MIN, 1},  {9.2e18, true, LLONG_MIN, -1}, {0.0, true, 1, LLONG_MIN},
        {-2.0, true, 2, -1},            {2.0, true, 2, 0},            {std::nan(""), false, 0, 1},
        {HUGE_VAL, false, 0, 1},         {1e100, false, 0, 1},         {1e18, true, 1000000000000000000LL, 1},
        {0.5, true, 1, 3},  // exact, but its fraction disagrees with its value
    };
    for (const Bad& b : bad)
      for (int slot : {0, 9}) {
        Device d;
        d.tick(5);
        keys(d, {K::D4, K::Eq});
        const Num ansBefore = d.vars().ans;
        CHECK(!d.restoreState(stateWithNum(st, slot, b.v, b.exact, b.num, b.den)));
        CHECK(d.vars().ans.v == ansBefore.v);  // refused: nothing changed
      }
    // Real values still come back: exact 1/3 and -2/7, an inexact pi, the largest exact.
    Device d;
    d.tick(5);
    std::string ok = stateWithNum(st, 0, 1.0 / 3.0, true, 1, 3);
    ok = stateWithNum(ok, 9, -2.0 / 7.0, true, -2, 7);
    ok = stateWithNum(ok, 1, 3.141592653589793, false, 0, 1);
    ok = stateWithNum(ok, 2, 1e15, true, 1000000000000000LL, 1);
    CHECK(d.restoreState(ok));
    CHECK(d.vars().a.exact && d.vars().a.num == 1 && d.vars().a.den == 3 && d.vars().ans.num == -2);
    keys(d, {K::On, K::Ans, K::Div, K::D7, K::Eq});
    CHECK(d.resultText() == "\xE2\x88\x92" "2\xE2\x8C\x9F" "49");
    // The engine itself: an exact LLONG_MIN handed in (no longer reachable) can't crash it.
    Vars v;
    v.a.exact = true;
    v.a.num = LLONG_MIN;
    v.a.den = 1;
    v.a.v = -9.2233720368547758e18;
    for (const std::vector<Tok>& e : std::vector<std::vector<Tok>>{{Tok::VarA, Tok::Div, Tok::D7},
                                                                  {Tok::VarA, Tok::Mul, Tok::VarA},
                                                                  {Tok::Abs, Tok::VarA},
                                                                  {Tok::VarA, Tok::Div, Tok::VarA},
                                                                  {Tok::VarA, Tok::Add, Tok::D1, Tok::Frac, Tok::D3}}) {
      const EvalResult r = evaluate(e, v, AngleUnit::Deg, fixedRandom);
      CHECK(r.error != CalcError::None || std::isfinite(r.value.v));
    }
    // A history entry longer than 99 keys is damage too.
    std::string longHist = st;
    longHist.back() = 1;  // one history item
    longHist += static_cast<char>(120);
    longHist += std::string(120, '\x01');
    longHist += std::string(25, '\0');
    CHECK(!Device().restoreState(longHist));
  }

  // B-2: ON on a typed problem's answer: = on the AI home screen takes a photo, not the old text.
  {
    Device d;
    d.tick(0);
    d.setOnline(true);
    d.setTypedSolveSupported(true);
    keys(d, {K::Mode, K::D4, K::D2, K::D1, K::Add, K::D2});
    toSend(d);
    keys(d, {K::Eq});
    std::string png;
    const int id = d.takeRequest();
    CHECK(id != 0 && d.takeTypedImage(png));
    d.onReply(id, reply("3"));
    keys(d, {K::On});
    CHECK(d.view() == View::Ai && d.ai().screen() == Screen::Ready && !d.ai().typed() && d.mode() == Mode::Ai);
    keys(d, {K::Eq});
    CHECK(d.takeRequest() != 0 && !d.takeTypedImage(png));  // a photo
    // ON while it is still being solved: cancelled, and the same.
    keys(d, {K::AC, K::Mode, K::D4, K::D2});
    CHECK(d.view() == View::Keyboard && d.keyboardText() == "1+2");
    toSend(d);
    keys(d, {K::Eq, K::On});
    CHECK(d.activeRequest() == 0 && !d.ai().typed());
    keys(d, {K::Eq});
    CHECK(d.takeRequest() != 0 && !d.takeTypedImage(png));
    // ON on a saved answer from History: the photo home screen too.
    keys(d, {K::AC, K::Mode, K::D4, K::D3, K::Eq});
    CHECK(d.view() == View::Ai && d.ai().readOnly());
    keys(d, {K::On});
    CHECK(d.view() == View::Ai && d.ai().screen() == Screen::Ready);
    keys(d, {K::Eq});
    CHECK(d.takeRequest() != 0 && !d.takeTypedImage(png));
  }

  // B-3: MODE / SETUP from Link, Confirm and Error come back to them; a deleted note's
  // view is never returned to.
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D4, K::D4, K::Eq, K::D1});  // new note "1", SAVE
    toSend(d);
    keys(d, {K::Eq, K::Down, K::Eq});                  // open it
    CHECK(d.view() == View::NoteView);
    keys(d, {K::Mode, K::AC});                         // back_ = NoteView
    CHECK(d.view() == View::NoteView);
    keys(d, {K::D2, K::Eq});                           // delete it
    CHECK(d.view() == View::Notes && d.notes().empty());
    keys(d, {K::AC, K::D7, K::D4});                    // Settings > Link calculator
    CHECK(d.view() == View::Link);
    keys(d, {K::Mode, K::AC});
    CHECK(d.view() == View::Link);
    keys(d, {K::Shift, K::Mode, K::AC});
    CHECK(d.view() == View::Link);
    keys(d, {K::AC});
    CHECK(d.view() == View::Settings);
    // Confirm "Discard note?" -> SETUP 1 (Deg) -> back to the question.
    keys(d, {K::AC, K::D4, K::Eq, K::D1, K::D2, K::AC});
    CHECK(d.view() == View::Confirm);
    keys(d, {K::Shift, K::Mode, K::D1});
    CHECK(d.view() == View::Confirm && d.confirmQuestion() == "Discard note?");
    keys(d, {K::AC});
    CHECK(d.view() == View::Keyboard && d.keyboardText() == "12");
    // Error -> MODE -> AC: the error again, not an app left behind earlier.
    keys(d, {K::On, K::D1, K::Div, K::D0, K::Eq});
    CHECK(d.view() == View::Error);
    keys(d, {K::Mode, K::AC});
    CHECK(d.view() == View::Error);
    keys(d, {K::AC});
    CHECK(d.view() == View::Calc);
    // A note deleted while MODE waits on top of its question: AC goes to the list.
    keys(d, {K::Mode, K::D4, K::D4, K::Eq, K::D5});
    toSend(d);
    keys(d, {K::Eq, K::Down, K::Eq, K::D2});
    CHECK(d.view() == View::Confirm && d.confirmQuestion() == "Delete this note?");
    keys(d, {K::Eq, K::Mode, K::AC});
    CHECK(d.view() == View::Notes);
  }

  // B-4: ENG stops where the mantissa no longer fits (it went on to wrong digits, then inf).
  {
    Device d;
    d.tick(0);
    keys(d, {K::D1, K::D2, K::D3, K::Eq});
    for (int i = 0; i < 120; ++i) {
      keys(d, {K::Eng});
      CHECK(textLength(d.resultText()) <= Framebuffer::kCols && d.resultText().find("inf") == std::string::npos);
    }
    CHECK(d.resultText() == "123000000\xC3\x97" "10\xE2\x81\xBB\xE2\x81\xB6");  // 123000000×10⁻⁶, the last that fits
    keys(d, {K::AC, K::D1, K::Eq});
    for (int i = 0; i < 120; ++i) keys(d, {K::Shift, K::Eng});
    CHECK(d.resultText() == "0.000000001\xC3\x97" "10\xE2\x81\xB9");  // 0.000000001×10⁹
    keys(d, {K::Eng});  // and back the other way
    CHECK(d.resultText() == "0.000001\xC3\x97" "10\xE2\x81\xB6");
    keys(d, {K::AC, K::D9, K::Exp10, K::D9, K::D9, K::Eq});
    for (int i = 0; i < 40; ++i) keys(d, {K::Eng});
    CHECK(d.resultText() == "9000000000\xC3\x97" "10\xE2\x81\xB9\xE2\x81\xB0");  // 9000000000×10⁹⁰: 10 digits
  }

  // B-5: cuts at a character boundary.
  {
    CHECK(utf8Prefix("ab\xC3\x97" "c", 3) == "ab" && utf8Prefix("ab\xC3\x97" "c", 4) == "ab\xC3\x97");
    CHECK(utf8Prefix("\xF0\x9F\x98\x80x", 3).empty() && utf8Prefix("abc", 10) == "abc" && utf8Prefix("abc", 0).empty());
    Device d;
    d.tick(0);
    d.setOnline(true);
    d.startScan();
    const std::string ans = std::string(999, 'x') + "\xC3\x97" "5";  // × straddles byte 1000
    d.onReply(d.takeRequest(), "{\"readable\":true,\"confidence\":0.95,\"answer\":\"" + ans + "\",\"read_as\":\"" +
                                   std::string(7000, 'r') + "\",\"steps\":[\"s\"]}");
    CHECK(d.savedAnswers().size() == 1);
    const Device::SavedAnswer& a = d.savedAnswers()[0];
    CHECK(a.replyJson.size() <= Device::kMaxSavedJson && validUtf8(a.replyJson) && validUtf8(a.answer));
    CHECK(a.answer.size() <= Device::kMaxSavedAnswer);
    SolveResult r;
    std::string err;
    CHECK(parseSolveResult(a.replyJson, r, err) && validUtf8(r.answer) && r.answer.compare(0, 10, ans, 0, 10) == 0);
    // Toasts and the notes' 160 count characters, not bytes.
    d.toast(std::string(30, '\xCF') + "x");  // broken text: still at most 23 characters
    CHECK(textLength(d.toastText()) <= Framebuffer::kCols - 2);
    d.toast("\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A"
            "\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A"
            "\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A\xE2\x88\x9A");
    CHECK(textLength(d.toastText()) == 23 && validUtf8(d.toastText()));
  }

  // B-6: the apps blob stays within kMaxAppsBytes under the worst input, and restores.
  {
    Device d;
    d.tick(0);
    d.setOnline(true);
    d.setTypedSolveSupported(true);
    // 20 replies just under kMaxSavedJson (kept whole), with quotes (escaped: 2 bytes each)
    // and 4-byte characters in the answer and question.
    const std::string emoji = "\xF0\x9F\x98\x80";
    for (int i = 0; i < 20; ++i) {
      std::string big;
      for (int k = 0; k < 150; ++k) big += emoji;
      std::string steps = "[";
      for (int k = 0; k < 8; ++k) steps += std::string(k ? "," : "") + "\"" + std::string(100, 's') + "\"";
      steps += "]";
      std::string json = "{\"readable\":true,\"confidence\":0.95,\"answer\":\"" + big + std::to_string(i) +
                         "\",\"read_as\":\"" + big + "\",\"steps\":" + steps + "}";
      while (json.size() < Device::kMaxSavedJson - 1) json.insert(json.size() - 1, " ");
      d.startScan();
      d.onReply(d.takeRequest(), json);
    }
    CHECK(d.savedAnswers().size() == 20);
    for (const auto& s : d.savedAnswers()) CHECK(s.replyJson.size() <= Device::kMaxSavedJson);
    keys(d, {K::On});
    for (int n = 0; n < 10; ++n) {  // 10 notes of 160 √ (3 bytes each)
      keys(d, {K::Mode, K::D4, K::D4, K::Eq});
      for (int k = 0; k < 160; ++k) d.onKey(K::Sqrt);
      toSend(d);
      keys(d, {K::Eq, K::On});
    }
    CHECK(d.notes().size() == 10);
    const std::string blob = d.saveApps();
    std::printf("apps blob, worst case: %zu bytes (limit %zu)\n", blob.size(), Device::kMaxAppsBytes);
    CHECK(blob.size() <= Device::kMaxAppsBytes);
    Device e;
    CHECK(e.restoreApps(blob) && e.notes() == d.notes() && e.savedAnswers().size() == 20);
    CHECK(e.savedAnswers()[0].replyJson == d.savedAnswers()[0].replyJson);  // the newest kept whole
    for (size_t i = 0; i < e.savedAnswers().size(); ++i) CHECK(e.savedAnswers()[i].answer == d.savedAnswers()[i].answer);
    CHECK(e.saveApps().size() <= Device::kMaxAppsBytes);
    // The oldest (short form) still opens, with its answer.
    keys(e, {K::Mode, K::D4, K::D3});
    for (int i = 0; i < 19; ++i) keys(e, {K::Down});
    keys(e, {K::Eq});
    CHECK(e.view() == View::Ai && e.ai().readOnly() && e.ai().result().answer.find("0") != std::string::npos);
    // An older firmware's blob (replies up to 6000 bytes) restores, shortened.
    std::string old;
    old += '\x01';
    old += '\x00';
    old += '\x01';
    const std::string bigReply = "{\"readable\":true,\"confidence\":0.9,\"answer\":\"x = 7\",\"read_as\":\"" +
                                 std::string(5800, 'q') + "\",\"steps\":[\"s\"]}";
    for (const std::string& t : {std::string("x = 7"), bigReply}) {
      old += static_cast<char>(t.size() & 0xFF);
      old += static_cast<char>(t.size() >> 8);
      old += t;
    }
    old += '\x00';
    const uint32_t ad = pngAdler32(reinterpret_cast<const uint8_t*>(old.data()), old.size());
    for (int i = 0; i < 4; ++i) old += static_cast<char>((ad >> (8 * i)) & 0xFF);
    Device f;
    CHECK(f.restoreApps(old) && f.savedAnswers().size() == 1 && f.savedAnswers()[0].replyJson.size() <= Device::kMaxSavedJson);
    SolveResult r;
    std::string err;
    CHECK(parseSolveResult(f.savedAnswers()[0].replyJson, r, err) && r.answer == "x = 7");
  }

  // B-8: a restart into exam mode leaves no app behind MODE (MODE then AC opened it).
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D4, K::D3, K::Mode, K::AC, K::On});  // back_ = History
    d.restoreExam(0);
    keys(d, {K::AC, K::D1, K::Div, K::D0, K::Eq, K::Mode, K::AC});
    CHECK(d.examActive() && d.view() == View::Error);
    keys(d, {K::AC, K::Mode, K::AC});
    CHECK(d.view() == View::Calc);
  }

  // B-9: Type it only where the platform sends its picture; a cancelled or failed typed
  // request lets go of its PNG.
  {
    Device d;
    d.tick(0);
    d.setOnline(true);
    CHECK(!d.typedSolveSupported());
    keys(d, {K::Mode, K::D4, K::D2});
    CHECK(d.view() == View::Notice);
    snapshot("type_it_unavailable", d);
    keys(d, {K::D1, K::Eq});
    CHECK(d.view() == View::Apps && d.takeRequest() == 0);
    keys(d, {K::Eq});  // = on TYPE IT (still selected) too
    CHECK(d.view() == View::Notice);
    App a;
    a.setOnline(true);
    std::string png;
    CHECK(a.startTyped(kPngSig + "y") && a.takeRequest() != 0);
    a.onKey(Key::AC);  // cancelled before the platform took it
    CHECK(!a.takeTypedImage(png));
    a.onKey(Key::Eq);  // still the typed problem: = sends it again
    const int id = a.takeRequest();
    a.onFailure(id, Failure::LowBattery);  // the platform refused before taking it
    CHECK(!a.takeTypedImage(png));
  }

  // B-10: the Apps menu from AI SOLVE leaves AI mode: no "AI" in the status bar, ON goes to
  // the calculator, and a scan in flight is cancelled (its late reply is ignored).
  {
    Device d;
    d.tick(0);
    d.setOnline(true);
    keys(d, {K::Mode, K::D4, K::D1});
    CHECK(d.mode() == Mode::Ai && d.view() == View::Ai);
    keys(d, {K::Eq});
    const int id = d.takeRequest();
    CHECK(id != 0);
    keys(d, {K::Mode, K::D4});
    CHECK(d.view() == View::Apps && d.mode() == Mode::Comp && d.activeRequest() == 0);
    keys(d, {K::D3});
    d.onReply(id, reply("5"));  // B-11: it can't land in History (or under its selection)
    CHECK(d.savedAnswers().empty() && d.historySelected() == 0);
    keys(d, {K::On});
    CHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
  }

  // B-11: an answer arriving keeps History's selection on the same answer (it could slide
  // onto "Clear history"). With B-10 a scan can't run behind the open list any more; the
  // preview page's Send (startScan()) still brings one in from History.
  {
    Device d;
    d.tick(0);
    d.setOnline(true);
    for (int i = 1; i <= 3; ++i) {
      d.startScan();
      d.onReply(d.takeRequest(), reply(std::to_string(i)));
    }
    keys(d, {K::Mode, K::D4, K::D3, K::Down});
    CHECK(d.historySelected() == 1 && d.savedAnswers()[1].answer == "2");
    d.startScan();
    d.onReply(d.takeRequest(), reply("4"));
    CHECK(d.savedAnswers().size() == 4 && d.historySelected() == 2 && d.savedAnswers()[2].answer == "2");
    keys(d, {K::AC, K::AC, K::Mode, K::D4, K::D3, K::Down, K::Down, K::Down, K::Down});
    CHECK(d.historySelected() == 4);  // "Clear history"
    for (int i = 5; i <= 25; ++i) {  // a full list: the oldest go, the selection stays on the row
      d.startScan();
      d.onReply(d.takeRequest(), reply(std::to_string(i)));
      CHECK(d.historySelected() == static_cast<int>(d.savedAnswers().size()));
    }
    CHECK(d.savedAnswers().size() == 20);
  }

  // restoreApps() while a note is being edited: the keyboard closes (it might edit another note).
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D4, K::D4, K::Eq, K::D1});
    toSend(d);
    keys(d, {K::Eq, K::Down, K::Eq, K::D1});  // editing note 1
    CHECK(d.view() == View::Keyboard && d.keyboardForNote());
    Device other;
    other.tick(0);
    keys(other, {K::Mode, K::D4, K::D4, K::Eq, K::D7});
    toSend(other);
    keys(other, {K::Eq, K::Up, K::Eq, K::D8});
    toSend(other);
    keys(other, {K::Eq});
    CHECK(d.restoreApps(other.saveApps()));
    CHECK(d.view() == View::Apps && !d.keyboardForNote() && d.notes().size() == 2);
    keys(d, {K::D4, K::Eq, K::D9});  // a new note now, not an edit of one of these
    toSend(d);
    keys(d, {K::Eq});
    CHECK(d.notes().size() == 3 && d.notes()[0] == "9");
  }
}

int runFuzzTests();  // tests/fuzz_tests.cpp

// ---- fx-115ES calculator modes and tools: STAT (MODE 2), TABLE (MODE 3), CALC, SOLVE, ∫dx, d/dx
// Types a number (digits, '.', '-' as (−)) and presses =.
static void enterNumber(Device& d, const char* s) {
  static const DKey kDigit[] = {K::D0, K::D1, K::D2, K::D3, K::D4, K::D5, K::D6, K::D7, K::D8, K::D9};
  for (; *s; ++s) {
    if (*s == '-') d.onKey(K::Neg);
    else if (*s == '.') d.onKey(K::Dot);
    else d.onKey(kDigit[*s - '0']);
  }
  d.onKey(K::Eq);
}

static bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) <= tol * std::max(1.0, std::fabs(b)); }

static void testModes() {
  // MODE 2: 1-VAR, the data 2 4 4 4 5 5 7 9, AC, SHIFT 1 > 4:Var > 2:x̄ = 5
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D2});
    CHECK(d.view() == View::Stat && d.mode() == Mode::Stat && d.stat().screen() == StatApp::Screen::TypeSelect);
    snapshot("stat_type", d);
    keys(d, {K::D1});
    CHECK(d.stat().screen() == StatApp::Screen::Editor && d.stat().type() == StatType::OneVar);
    for (const char* v : {"2", "4", "4", "4", "5", "5", "7", "9"}) enterNumber(d, v);
    CHECK(d.stat().rows() == 8 && d.stat().x(7) == 9);
    snapshot("stat_editor", d);
    keys(d, {K::AC, K::Shift, K::D1});
    CHECK(d.stat().screen() == StatApp::Screen::Menu);
    snapshot("stat_menu", d);
    keys(d, {K::D4, K::D2, K::Eq});
    CHECK(d.stat().showingResult() && d.stat().resultText() == "5");
    CHECK(d.vars().ans.v == 5);  // = in STAT updates Ans, as on the Casio
    snapshot("stat_result", d);
    // σx with the keys: SHIFT 1 4 3 =
    keys(d, {K::AC, K::Shift, K::D1, K::D4, K::D3, K::Eq});
    CHECK(d.stat().resultText() == "2");
    // SETUP over STAT comes back to it; so do the Apps (the data stays)
    keys(d, {K::Shift, K::Mode});
    CHECK(d.view() == View::Setup);
    keys(d, {K::AC});
    CHECK(d.view() == View::Stat);
    keys(d, {K::Mode, K::D4});
    CHECK(d.view() == View::Apps && d.mode() == Mode::Stat);  // STAT waits under the apps
    keys(d, {K::AC});
    CHECK(d.view() == View::Stat && d.mode() == Mode::Stat && d.stat().rows() == 8);
    keys(d, {K::Mode, K::D4, K::Eq});  // AI Solve, then AC: the Apps menu, AC: STAT again
    CHECK(d.view() == View::Ai && d.mode() == Mode::Ai);
    keys(d, {K::AC});
    CHECK(d.view() == View::Apps);
    keys(d, {K::AC});
    CHECK(d.view() == View::Stat && d.mode() == Mode::Stat && d.stat().rows() == 8);
    // ON in STAT clears like AC and stays in STAT
    keys(d, {K::On});
    CHECK(d.view() == View::Stat && d.stat().rows() == 8);
    // deep sleep keeps STAT and its data
    {
      Device e;
      e.tick(0);
      CHECK(e.restoreState(d.saveState()));
      CHECK(e.mode() == Mode::Stat && e.view() == View::Stat && e.stat().rows() == 8 && e.stat().x(0) == 2 &&
            e.stat().x(7) == 9 && e.stat().type() == StatType::OneVar);
      keys(e, {K::Shift, K::D1, K::D4, K::D2, K::Eq});
      CHECK(e.stat().resultText() == "5");
      std::string st = d.saveState();
      st.back() ^= 0x40;  // a damaged value: still restores (a different number) or is refused, never garbage
      Device f;
      if (f.restoreState(st)) CHECK(f.stat().rows() == 8);
      st = d.saveState();
      st.resize(st.size() - 3);  // cut short: refused
      CHECK(!f.restoreState(st));
    }
    // MODE 1: COMP; MODE 2 then AC on the type list: COMP too
    keys(d, {K::Mode, K::D1});
    CHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
    keys(d, {K::Mode, K::D2, K::AC});
    CHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
  }
  // SETUP ▼ 1:STAT: Frequency? 1:ON adds the FREQ column; kept across deep sleep
  {
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::Mode});
    snapshot("setup_page1", d);  // ▼ in the status row: there is a second page
    keys(d, {K::Down});
    CHECK(d.view() == View::Setup && d.setupPage() == 1);
    snapshot("setup_page2", d);
    keys(d, {K::Up});
    CHECK(d.setupPage() == 0);
    keys(d, {K::Down, K::D1});
    CHECK(d.view() == View::SetupFreq);
    snapshot("setup_frequency", d);
    keys(d, {K::D1});
    CHECK(d.statFrequency() && d.view() == View::Calc);
    keys(d, {K::Mode, K::D2, K::D2});  // A+BX: X, Y, FREQ
    CHECK(d.stat().columns() == 3 && d.stat().maxRows() == 26);
    for (const char* v : {"1", "3", "2"}) enterNumber(d, v);  // three X values (Y 0, FREQ 1)
    keys(d, {K::AC});
    Device e;
    e.tick(0);
    CHECK(e.restoreState(d.saveState()) && e.statFrequency() && e.stat().type() == StatType::Linear &&
          e.stat().rows() == d.stat().rows());
    keys(d, {K::Shift, K::Mode, K::Down, K::D1, K::D2});
    CHECK(!d.statFrequency() && d.stat().columns() == 2 && d.stat().rows() == 0);  // the Casio clears the data
    keys(d, {K::Shift, K::Mode, K::Down, K::D1, K::AC});  // AC: back to SETUP
    CHECK(d.view() == View::Setup);
  }
  // MODE 3: f(X)=X²+1, = = = (Start 1, End 5, Step 1)
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D3});
    CHECK(d.view() == View::Table && d.mode() == Mode::Table);
    keys(d, {K::Alpha, K::Close, K::Sq, K::Add, K::D1});
    CHECK(d.table().functionText() == "X\xC2\xB2+1");
    snapshot("table_function", d);
    keys(d, {K::Eq});
    CHECK(d.table().screen() == TableApp::Screen::Start);
    snapshot("table_start", d);
    keys(d, {K::Eq, K::Eq, K::Eq});
    CHECK(d.table().screen() == TableApp::Screen::Table && d.table().rows() == 5 && d.table().rowY(4) == 26);
    snapshot("table_rows", d);
    keys(d, {K::Down, K::Down, K::Down, K::Down, K::Right});
    CHECK(d.table().cursorRow() == 4 && d.table().cursorCol() == 1);
    snapshot("table_scrolled", d);
    Device e;
    e.tick(0);
    CHECK(e.restoreState(d.saveState()) && e.mode() == Mode::Table && e.view() == View::Table &&
          e.table().functionText() == "X\xC2\xB2+1" && e.table().end() == 5);
    keys(d, {K::AC});
    CHECK(d.table().screen() == TableApp::Screen::Function && d.table().function().size() == 4);
    keys(d, {K::AC});
    CHECK(d.view() == View::Table && d.table().function().empty());
    keys(d, {K::AC});  // AC on an empty f(X)=: COMP
    CHECK(d.view() == View::Calc && d.mode() == Mode::Comp);
  }
  // CALC: X²+1 with X? 3 = 10, = again (the Casio loop) X? 4 = 17, AC back to the expression
  {
    Device d;
    d.tick(0);
    keys(d, {K::Alpha, K::Close, K::Sq, K::Add, K::D1, K::Calc});
    CHECK(d.view() == View::CalcRun);
    snapshot("calc_prompt", d);
    keys(d, {K::D3, K::Eq});
    CHECK(d.calcRunner().showingResult() && d.calcRunner().result().v == 10);
    CHECK(d.vars().x.v == 3 && d.vars().ans.v == 10);
    snapshot("calc_result", d);
    keys(d, {K::Eq, K::D4, K::Eq});
    CHECK(d.calcRunner().result().v == 17);
    keys(d, {K::AC});
    CHECK(d.view() == View::Calc && !d.showingResult() && exprText(d.expression()) == "X\xC2\xB2+1" &&
          d.cursor() == 4);
    keys(d, {K::Calc, K::Eq, K::Left});  // ◀ on the result: edit with the cursor at the end
    CHECK(d.view() == View::Calc && d.cursor() == 4);
    // Y=X+1 (ALPHA CALC types "="): CALC assigns Y
    keys(d, {K::AC, K::Alpha, K::SD, K::Alpha, K::Calc, K::Alpha, K::Close, K::Add, K::D1});
    CHECK(exprText(d.expression()) == "Y=X+1");
    keys(d, {K::Calc, K::D5, K::Eq});
    CHECK(d.vars().y.v == 6 && d.vars().x.v == 5);
    keys(d, {K::AC, K::Eq});  // a plain = on an equation: Syntax ERROR, as on the Casio
    CHECK(d.view() == View::Error && d.errorMessage() == "Syntax ERROR");
    keys(d, {K::AC});
    keys(d, {K::D2, K::Add, K::Alpha, K::Calc, K::D1, K::Calc});  // CALC on "2+=1": its Syntax ERROR, ▶ Goto
    CHECK(d.view() == View::CalcRun);
    keys(d, {K::Right});
    CHECK(d.view() == View::Calc && d.cursor() >= 0 && d.cursor() <= 4);
    keys(d, {K::AC, K::D2, K::Calc});  // nothing to prompt: the value, at once
    CHECK(d.calcRunner().showingResult() && d.calcRunner().result().v == 2);
    keys(d, {K::Mode});  // MODE ends CALC
    CHECK(d.view() == View::ModeMenu);
    keys(d, {K::AC});
    CHECK(d.view() == View::Calc);
  }
  // SOLVE: X²−2=0 from X? 1: X = √2, L−R = 0; X keeps the root, Ans is unchanged
  {
    Device d;
    d.tick(0);
    keys(d, {K::D7, K::Eq});  // Ans = 7
    keys(d, {K::AC, K::Alpha, K::Close, K::Sq, K::Sub, K::D2, K::Alpha, K::Calc, K::D0, K::Shift, K::Calc});
    CHECK(d.view() == View::SolveRun && d.solveRunner().solveVar() == Tok::VarX);
    snapshot("solve_prompt", d);
    keys(d, {K::D1, K::Eq});
    CHECK(d.solveRunner().showingResult() && near(d.solveRunner().root(), std::sqrt(2.0)));
    CHECK(near(d.vars().x.v, std::sqrt(2.0)) && d.vars().ans.v == 7);
    snapshot("solve_result", d);
    keys(d, {K::Eq, K::Neg, K::D1, K::Eq});  // = solves again: from -1 the other root
    CHECK(near(d.solveRunner().root(), -std::sqrt(2.0)));
    keys(d, {K::Left});
    CHECK(d.view() == View::Calc && d.cursor() == static_cast<int>(d.expression().size()));
    // no root: X²+1=0 says Can't Solve
    keys(d, {K::AC, K::Alpha, K::Close, K::Sq, K::Add, K::D1, K::Shift, K::Calc, K::Eq});
    CHECK(d.view() == View::SolveRun && !d.solveRunner().showingResult());
    snapshot("solve_cant", d);
    keys(d, {K::AC});
    CHECK(d.view() == View::Calc);
  }
  // ∫dx and d/dx typed with the keys: ∫(X²,0,3) = 9, d/dx(X³,2) = 12; DEL takes "∫(" in one go
  {
    Device d;
    d.tick(0);
    keys(d, {K::Integral, K::Alpha, K::Close, K::Sq, K::Shift, K::Close, K::D0, K::Shift, K::Close, K::D3, K::Close});
    CHECK(exprText(d.expression()) == "\xE2\x88\xAB(X\xC2\xB2,0,3)");
    keys(d, {K::Eq});
    CHECK(d.resultText() == "9" && d.vars().ans.v == 9);
    snapshot("integral_result", d);
    keys(d, {K::AC, K::Shift, K::Integral, K::Alpha, K::Close, K::Shift, K::Sq, K::Shift, K::Close, K::D2, K::Eq});
    CHECK(exprText(d.expression()) == "d/dx(X\xC2\xB3,2" && d.resultText() == "12");
    snapshot("derivative_result", d);
    keys(d, {K::AC, K::D2, K::Integral, K::Del});
    CHECK(exprText(d.expression()) == "2");
  }
  // Exam mode: STAT, TABLE, CALC, SOLVE, ∫ and d/dx all work; the apps stay off
  {
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::AC, K::Shift, K::D7, K::On, K::AC});
    CHECK(d.examActive() && d.view() == View::Calc);
    keys(d, {K::Mode, K::D2, K::D1, K::D3, K::Eq, K::D5, K::Eq});
    CHECK(d.view() == View::Stat && d.stat().rows() == 2);
    snapshot("exam_stat", d);  // STAT and the exam clock share the status row
    keys(d, {K::AC, K::Shift, K::D1, K::D4, K::D2, K::Eq});
    CHECK(d.stat().resultText() == "4");
    keys(d, {K::Mode, K::D4});
    CHECK(d.view() == View::Notice);  // APPS ARE OFF
    keys(d, {K::AC});
    CHECK(d.view() == View::Stat);  // back to STAT, not an app
    keys(d, {K::Mode, K::D3, K::Alpha, K::Close, K::Eq, K::Eq, K::Eq, K::Eq});
    CHECK(d.view() == View::Table && d.table().rows() == 5);
    keys(d, {K::Mode, K::D1, K::Alpha, K::Close, K::Add, K::D1, K::Calc, K::D2, K::Eq});
    CHECK(d.view() == View::CalcRun && d.calcRunner().result().v == 3);
    keys(d, {K::AC, K::Shift, K::Calc, K::D1, K::Eq});  // X+1 = 0 -> X = -1
    CHECK(d.view() == View::SolveRun && near(d.solveRunner().root(), -1));
    keys(d, {K::AC, K::AC, K::Integral, K::Alpha, K::Close, K::Shift, K::Close, K::D0, K::Shift, K::Close, K::D2, K::Eq});
    CHECK(d.resultText() == "2");
    CHECK(d.examActive() && d.ai().activeRequest() == 0);
    // exam mode started from STAT (SETUP 8 =) keeps STAT; restoreExam() too
    Device s;
    s.tick(0);
    keys(s, {K::Mode, K::D2, K::D1, K::D4, K::Eq, K::Shift, K::Mode, K::D8, K::Eq});
    CHECK(s.examActive() && s.view() == View::Notice);
    keys(s, {K::AC});
    CHECK(s.view() == View::Stat && s.mode() == Mode::Stat && s.stat().rows() == 1);
    Device r;
    r.tick(0);
    CHECK(r.restoreState(s.saveState()) && r.examActive() && r.view() == View::Stat && r.stat().rows() == 1);
  }
}

// Bug hunt 2026-10-11 (device side): R-11 saved state, R-16 SETUP reaching STAT / TABLE.
static void testBugHunt15() {
  // R-16: SETUP's angle unit / Norm reach STAT and TABLE at once (the screen SETUP returns to is
  // drawn with them), and a restored state hands STAT / TABLE the restored ones.
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D2, K::D1});  // STAT 1-VAR
    CHECK(d.view() == View::Stat && d.stat().normMode() == NormMode::Norm1);
    keys(d, {K::Shift, K::Mode, K::D5});  // SETUP 5: Norm 2
    CHECK(d.view() == View::Stat && d.stat().normMode() == NormMode::Norm2);
    keys(d, {K::Shift, K::Mode, K::D2});  // SETUP 2: Rad
    CHECK(d.stat().angleUnit() == AngleUnit::Rad);
    keys(d, {K::Mode, K::D3});  // TABLE
    CHECK(d.view() == View::Table && d.table().normMode() == NormMode::Norm2 && d.table().angleUnit() == AngleUnit::Rad);
    keys(d, {K::Shift, K::Mode, K::D4, K::Shift, K::Mode, K::D3});  // Norm 1, Gra
    CHECK(d.table().normMode() == NormMode::Norm1 && d.table().angleUnit() == AngleUnit::Gra);
    keys(d, {K::Shift, K::Mode, K::D5});
    Device e;
    e.tick(0);
    CHECK(e.restoreState(d.saveState()));
    CHECK(e.view() == View::Table && e.table().normMode() == NormMode::Norm2 && e.table().angleUnit() == AngleUnit::Gra);
  }
  // R-11: version 2 states end with an Adler-32 (a flipped bit is refused); STAT values the
  // calculator can't hold (0 < |x| < 1E-99, denormals) are refused; version 1 is still read.
  {
    Device d;
    d.tick(0);
    keys(d, {K::Mode, K::D2, K::D1, K::D5, K::Eq});  // STAT 1-VAR, x = 5
    CHECK(d.stat().rows() == 1);
    const std::string st = d.saveState();
    CHECK(st[0] == 2);
    Device ok;
    ok.tick(0);
    CHECK(ok.restoreState(st) && ok.stat().rows() == 1 && ok.stat().x(0) == 5);
    auto withX = [&](double x) {  // the row's X: the 8 bytes before the Adler-32
      std::string s = st;
      std::memcpy(&s[s.size() - 12], &x, 8);
      return stateResigned(s);
    };
    for (double bad : {-1.2e-163, 4.9e-324, 1e-100, -5e-101}) {
      Device e;
      e.tick(0);
      CHECK(!e.restoreState(withX(bad)));
    }
    for (double fine : {1e-99, -2.5e-50, 0.0}) {
      Device e;
      e.tick(0);
      CHECK(e.restoreState(withX(fine)) && e.stat().x(0) == fine);
    }
    int refused = 0, tried = 0;
    for (size_t i = 0; i < st.size(); ++i, ++tried) {
      std::string s = st;
      s[i] = static_cast<char>(s[i] ^ (1 << (i % 8)));
      Device e;
      e.tick(0);
      refused += e.restoreState(s) ? 0 : 1;
    }
    CHECK(refused == tried);
    std::string old = st.substr(0, st.size() - 4);
    old[0] = 1;
    Device o;
    o.tick(0);
    CHECK(o.restoreState(old) && o.stat().rows() == 1 && o.stat().x(0) == 5);
  }
}

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);  // every line out, even if a test crashes
  testModes();
  testFuzzFindings();
  testBugHunt15();
  testDeviceFixes();
  testPng();
  testAppsMenu();
  testTypeIt();
  testHistory();
  testNotes();
  testSettingsApp();
  testAppsExam();
  testAppModules();
  testAppsPersistence();
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
  // The app modules' and the engine regressions' own suites (tests/*_tests.cpp): each prints
  // its own line and returns its failures, which count in the total below.
  const int moduleFails = runGeoTests() + runPeriodicTests() + runEngineRegressTests() + runStatTableTests() +
                          runNumericTests() + runWorkerApiTests() + runEqnTests() + runCmplxTests() + runBaseNTests() +
                          runMatVecTests() +
                          runFuzzTests();
  g_fail += moduleFails;
  std::printf("%d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
