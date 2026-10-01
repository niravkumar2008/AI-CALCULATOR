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

#include "../core/app.h"
#include "../core/calc_engine.h"
#include "../core/claude_api.h"
#include "../core/device.h"
#include "../core/font.h"
#include "../core/json.h"
#include "../core/text.h"

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

  // Unclear photo: warning first, then the answer marked unclear.
  id = press(a, Key::Eq);
  a.onReply(id, readFile("samples/03_handwritten_unclear.json"));
  CHECK(a.screen() == Screen::Warning);
  snapshot("warning_unclear", a);
  a.onKey(Key::Down);
  CHECK(a.screen() == Screen::Result);
  snapshot("answer_unclear", a);

  // Retake from the warning screen starts a new request.
  id = press(a, Key::Eq);
  a.onReply(id, readFile("samples/03_handwritten_unclear.json"));
  CHECK(a.screen() == Screen::Warning);
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
  CHECK(schema["required"].items().size() == 7 && schema["additionalProperties"].isBool());
  CHECK(schema["required"].items()[3].asString() == "expression" &&
        schema["required"].items()[4].asString() == "answer");
  const Json& img = req["messages"].items()[0]["content"].items()[0];
  CHECK(img["source"]["data"].asString() == base64Encode("JPEGBYTES"));

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
  a.onPartialAnswer(id, 0.5, "x = 4");  // unclear reading: wait for the full reply
  CHECK(a.screen() == Screen::Busy);
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
  keys(d, {K::AC, K::D1, K::D0, K::NCr, K::D3, K::Eq});
  CHECK(d.resultText() == "120");
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

  // An unclear scan is flagged on the calculator screen.
  keys(d, {K::Mode, K::D4, K::Eq});
  id = d.takeRequest();
  d.onReply(id, readFile("samples/07_unclear_arithmetic.json"));
  CHECK(d.ai().screen() == Screen::Warning);
  keys(d, {K::Down});  // show the answer anyway
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

int main() {
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
  std::printf("%d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
