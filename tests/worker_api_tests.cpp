// Worker API tests (core/worker_api.*): request bodies and reply parsing for the
// calculator's /v1/* device endpoints, with examples from docs/web/openapi.yaml and the
// Worker's own fixtures (web/api/src/http.ts messages, web/api/test/helpers.ts cannedSse,
// the mid-stream error of solve.ts). Call runWorkerApiTests() from the test runner's main();
// it prints a summary line and returns the number of failures.
#include <cstdio>
#include <string>

#include "../core/json.h"
#include "../core/worker_api.h"

using namespace calc;

namespace {

int g_fail = 0, g_pass = 0;
#define WCHECK(cond)                                                               \
  do {                                                                             \
    if (cond) ++g_pass;                                                            \
    else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
  } while (0)

const std::string kToken = "dt_" + std::string(43, 'A');
const std::string kSecret = "hs_abcdefghijklmnopqrstuvwxyzABCDEFGHIJ-_01234";  // 43 after hs_

std::string sse(const std::string& type, const std::string& data) {
  return "event: " + type + "\ndata: {\"type\":\"" + type + "\"" + (data.empty() ? "" : "," + data) + "}\n\n";
}

// web/api/test/helpers.ts cannedSse(): the Worker passes this through byte for byte.
std::string cannedSse(const std::string& textJsonEscaped) {
  return sse("message_start",
             "\"message\":{\"id\":\"msg_test\",\"type\":\"message\",\"role\":\"assistant\",\"model\":\"m\",\"content\":[],"
             "\"stop_reason\":null,\"usage\":{\"input_tokens\":1500,\"output_tokens\":1}}") +
         sse("content_block_start", "\"index\":0,\"content_block\":{\"type\":\"text\",\"text\":\"\"}") +
         "event: ping\ndata: {\"type\": \"ping\"}\n\n" +
         sse("content_block_delta", "\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":" + textJsonEscaped + "}") +
         sse("content_block_stop", "\"index\":0") +
         sse("message_delta", "\"delta\":{\"stop_reason\":\"end_turn\"},\"usage\":{\"output_tokens\":800}") +
         sse("message_stop", "");
}

std::string errBody(const std::string& type, const std::string& message) {
  return "{\"type\":\"error\",\"error\":{\"type\":\"" + type + "\",\"message\":\"" + message + "\"}}";
}

void testIdentity() {
  WCHECK(isWorkerHwId("calc-0123456789ab"));
  WCHECK(!isWorkerHwId("calc-0123456789AB"));  // MAC hex is lower-case
  WCHECK(!isWorkerHwId("calc-0123456789a"));
  WCHECK(isWorkerHwSecret(kSecret));
  WCHECK(!isWorkerHwSecret("hs_short"));
  WCHECK(!isWorkerHwSecret(kSecret + "x"));
  WCHECK(isWorkerDeviceToken(kToken));
  WCHECK(!isWorkerDeviceToken("dt_" + std::string(42, 'A') + "+"));
  WCHECK(isWorkerDeviceId("dev_0123456789abcdef"));
  WCHECK(!isWorkerDeviceId("dev_0123"));
}

void testRegister() {
  // Request: hw_secret only when it is well-formed (the contract rejects a malformed one with 400).
  WCHECK(buildRegisterBody("calc-0123456789ab", "") == "{\"hw_id\":\"calc-0123456789ab\"}");
  WCHECK(buildRegisterBody("calc-0123456789ab", "hs_bad") == "{\"hw_id\":\"calc-0123456789ab\"}");
  const std::string withSecret = buildRegisterBody("calc-0123456789ab", kSecret);
  WCHECK(withSecret == "{\"hw_id\":\"calc-0123456789ab\",\"hw_secret\":\"" + kSecret + "\"}");
  Json j;
  std::string err;
  WCHECK(Json::parse(withSecret, j, err) && j["hw_secret"].asString() == kSecret);

  // 201 Registered (device.ts register()).
  const std::string ok = "{\"device_id\":\"dev_00112233aabbccdd\",\"token\":\"" + kToken +
                         "\",\"pair_code\":\"ABCD-EFGH\",\"link_url\":\"https://proofline.example/link?code=ABCD-EFGH\","
                         "\"expires_at\":1760000000}";
  WorkerRegistration r;
  WCHECK(parseRegisterReply(201, ok, r, err));
  WCHECK(r.token == kToken && r.deviceId == "dev_00112233aabbccdd");
  WCHECK(r.pairCode == "ABCD-EFGH");
  WCHECK(r.site == "https://proofline.example");
  WCHECK(r.expiresAt == 1760000000);
  // Local dev Worker: http and a port.
  const std::string local = "{\"device_id\":\"dev_00112233aabbccdd\",\"token\":\"" + kToken +
                            "\",\"pair_code\":\"WXYZ-2345\",\"link_url\":\"http://localhost:8787/link?code=WXYZ-2345\","
                            "\"expires_at\":1}";
  WCHECK(parseRegisterReply(201, local, r, err) && r.site == "http://localhost:8787" && r.pairCode == "WXYZ-2345");
  // A bad token is refused, even with 201.
  WCHECK(!parseRegisterReply(201, "{\"device_id\":\"dev_00112233aabbccdd\",\"token\":\"dt_x\"}", r, err));
  WCHECK(r.token.empty());
  // 429 rate_limited: the server's own message.
  WCHECK(!parseRegisterReply(429, errBody("rate_limited", "Too many registrations from this network. Try again in an hour."), r,
                             err));
  WCHECK(err == "Too many registrations from this network. Try again in an hour.");
  WCHECK(!parseRegisterReply(503, "<html>bad gateway</html>", r, err) && err.find("503") != std::string::npos);
  WCHECK(!parseRegisterReply(0, "", r, err) && !err.empty());
}

void testStatus() {
  WorkerStatus s;
  std::string err;
  // Not linked (device.ts status(): reason = msgNotLinked with the current code).
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":false,\"plan\":\"none\",\"can_solve\":false,\"reason\":\"Link this calculator: "
                           "go to proofline.example/link and enter code K7MP-Q2RX\"}",
                           s, err));
  WCHECK(!s.linked && !s.canSolve && s.plan == "none");
  WCHECK(s.pairCode == "K7MP-Q2RX");
  WCHECK(s.site == "proofline.example");
  // Linked, in the free month.
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":true,\"plan\":\"trial\",\"can_solve\":true,\"account_hint\":\"n***@gmail.com\","
                           "\"solves_this_month\":3,\"solves_limit\":300,\"trial_ends_at\":1762592000}",
                           s, err));
  WCHECK(s.linked && s.canSolve && s.plan == "trial" && s.accountHint == "n***@gmail.com");
  WCHECK(s.solvesThisMonth == 3 && s.solvesLimit == 300 && s.trialEndsAt == 1762592000);
  WCHECK(s.pairCode.empty());
  // Linked, free month over: can_solve false with the screen text.
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":true,\"plan\":\"inactive\",\"can_solve\":false,\"reason\":\"Your free month is over. "
                           "Subscribe at proofline.example/account to keep using AI.\",\"account_hint\":\"n***@gmail.com\"}",
                           s, err));
  WCHECK(s.linked && !s.canSolve && s.reason.find("free month is over") != std::string::npos);
  // Linked, plan none (unenrolled board): no free month.
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":true,\"plan\":\"none\",\"can_solve\":false,\"reason\":\"No free month for this "
                           "calculator. Subscribe at proofline.example/account to use AI.\"}",
                           s, err));
  WCHECK(s.reason.rfind("No free month", 0) == 0);
  // 401s.
  WCHECK(!parseWorkerStatus(401, errBody("device_revoked",
                                         "This calculator was removed from its account. Link it again in Apps > Settings."),
                            s, err));
  WCHECK(err.find("removed from its account") != std::string::npos);
  WCHECK(!parseWorkerStatus(200, "{\"plan\":\"none\"}", s, err));  // missing required fields
  WCHECK(parseWorkerStatus(200, "{\"linked\":true,\"plan\":\"trial\",\"can_solve\":true}", s, err));
  WCHECK(s.planReason.empty() && s.linkUrl.empty());  // an older server: no plan_reason / link_url
}

// Fields added 2026-10-11 (all optional): pair_code, link_url, plan_reason.
void testStatusNewFields() {
  WorkerStatus s;
  std::string err;
  // Not linked: pair_code and link_url win over the reason (here naming another code and host).
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":false,\"plan\":\"none\",\"can_solve\":false,\"reason\":\"Link this calculator: "
                           "go to old.example/link and enter code WXYZ-2345\",\"pair_code\":\"K7QM-3XTR\","
                           "\"link_url\":\"https://proofline.example/link/\",\"plan_reason\":\"not_linked\"}",
                           s, err));
  WCHECK(s.pairCode == "K7QM-3XTR" && s.site == "https://proofline.example");
  WCHECK(s.linkUrl == "https://proofline.example/link/" && s.planReason == "not_linked");
  WCHECK(pairQrUrl(s.linkUrl, s.pairCode) == "https://proofline.example/link/?code=K7QM-3XTR");
  WCHECK(planStatusText(s.planReason, s.site, s.reason).empty());  // the code screen says it all
  // Not linked, pair_code null or absent: back to the reason.
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":false,\"plan\":\"none\",\"can_solve\":false,\"reason\":\"Link this calculator: "
                           "go to proofline.example/link and enter code K7MP-Q2RX\",\"pair_code\":null}",
                           s, err));
  WCHECK(s.pairCode == "K7MP-Q2RX" && s.site == "proofline.example" && s.linkUrl.empty());
  // Linked: pair_code is null; link_url still sent.
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":true,\"plan\":\"trial\",\"can_solve\":true,\"pair_code\":null,"
                           "\"link_url\":\"https://proofline.example/link/\",\"plan_reason\":\"trial\"}",
                           s, err));
  WCHECK(s.linked && s.pairCode.empty() && s.site == "https://proofline.example" && s.planReason == "trial");
  WCHECK(planStatusText(s.planReason, s.site, s.reason) == "Free month active");
  // A linked status never takes a code, even a stray one.
  WCHECK(parseWorkerStatus(200, "{\"linked\":true,\"plan\":\"active\",\"can_solve\":true,\"pair_code\":\"K7QM-3XTR\"}", s,
                           err));
  WCHECK(s.pairCode.empty());
  // Every plan_reason's line.
  const std::string site = "https://proofline.example";
  WCHECK(planStatusText("active", site, "") == "AI tutor plan active");
  WCHECK(planStatusText("past_due", site, "") == "Payment problem: check your account");
  WCHECK(planStatusText("free_month_over", site, "") == "Free month over: subscribe at proofline.example");
  WCHECK(planStatusText("inactive", site + "/", "") == "Plan ended: subscribe at proofline.example");
  WCHECK(planStatusText("inactive", "", "") == "Plan ended: subscribe on the website");
  WCHECK(planStatusText("no_trial", site, "") == "No free month for this calculator");
  // Unknown plan_reason: dropped, the reason is shown instead.
  const std::string r = "Your subscription has ended. Subscribe at proofline.example/account to use AI.";
  WCHECK(parseWorkerStatus(200,
                           "{\"linked\":true,\"plan\":\"inactive\",\"can_solve\":false,\"reason\":\"" + r +
                               "\",\"plan_reason\":\"suspended\"}",
                           s, err));
  WCHECK(s.planReason.empty() && planStatusText(s.planReason, site, s.reason) == r);
  WCHECK(planStatusText("", site, "").empty());
  // Wrong types are ignored, not fatal.
  WCHECK(parseWorkerStatus(200, "{\"linked\":false,\"plan\":\"none\",\"can_solve\":false,\"pair_code\":5,"
                                "\"link_url\":false,\"plan_reason\":7}", s, err));
  WCHECK(s.pairCode.empty() && s.linkUrl.empty() && s.planReason.empty());
  WCHECK(pairQrUrl("", "K7QM-3XTR").empty() && pairQrUrl("https://x.example/link/", "").empty());
}

void testPairStart() {
  WorkerPairing p;
  std::string err;
  WCHECK(parsePairStart(200,
                        "{\"already_linked\":false,\"code\":\"ABCD-EFGH\",\"link_url\":\"https://proofline.example/link?"
                        "code=ABCD-EFGH\",\"expires_at\":1760000900}",
                        p, err));
  WCHECK(!p.alreadyLinked && p.code == "ABCD-EFGH" && p.site == "https://proofline.example" && p.expiresAt == 1760000900);
  WCHECK(parsePairStart(200, "{\"already_linked\":true}", p, err) && p.alreadyLinked && p.code.empty());
  WCHECK(!parsePairStart(200, "{\"already_linked\":false}", p, err));
  WCHECK(!parsePairStart(429, errBody("rate_limited", "Too many requests. Please try again later."), p, err));
  WCHECK(err == "Too many requests. Please try again later.");
}

void testSolveRequest() {
  const std::string jpeg("\xFF\xD8\xFF\xE0" "abc", 7);
  const std::string png("\x89PNG\r\n\x1a\n" "xyz", 11);
  const std::string one = buildWorkerSolveRequest(jpeg);
  WCHECK(one == "{\"images\":[{\"media_type\":\"image/jpeg\",\"data\":\"" + base64Encode(jpeg) + "\"}]}");
  Json j;
  std::string err;
  WCHECK(Json::parse(one, j, err) && j["images"].items().size() == 1 && j["tutor"].isNull());
  // Only images and tutor (additionalProperties: false): no model, system, effort or prompt.
  WCHECK(one.find("model") == std::string::npos && one.find("system") == std::string::npos);
  const std::string two = buildWorkerSolveRequest(jpeg, png, true);
  WCHECK(Json::parse(two, j, err));
  WCHECK(j["images"].items().size() == 2);
  WCHECK(j["images"].items()[0]["media_type"].asString() == "image/jpeg");
  WCHECK(j["images"].items()[1]["media_type"].asString() == "image/png");
  WCHECK(j["images"].items()[1]["data"].asString() == base64Encode(png));
  WCHECK(j["tutor"].isBool() && j["tutor"].asBool());
  WCHECK(j.members().size() == 2);
  // A typed problem alone is a PNG.
  WCHECK(Json::parse(buildWorkerSolveRequest(png), j, err) && j["images"].items()[0]["media_type"].asString() == "image/png");
}

void testSolveReply() {
  // A full stream: the reply JSON arrives as the text, peekAnswer sees the answer early.
  StreamReader st;
  const std::string text = "{\"readable\":true,\"confidence\":0.9,\"unclear\":[],\"expression\":\"6*7\",\"choice\":\"\","
                           "\"answer\":\"42\",\"check\":\"6*7\",\"read_as\":\"6 times 7\",\"steps\":[\"6×7 = 42\",\"done\"]}";
  std::string escaped;
  for (char c : text) {
    if (c == '"' || c == '\\') escaped += '\\';
    escaped += c;
  }
  const std::string stream = cannedSse("\"" + escaped + "\"");
  for (size_t i = 0; i < stream.size(); i += 7) st.feed(stream.substr(i, 7));  // any split
  WCHECK(st.finished() && st.text() == text);
  double conf;
  std::string answer;
  WCHECK(peekAnswer(st.text(), conf, answer) && answer == "42" && conf > 0.89);
  WorkerFailure f;
  WCHECK(!classifyWorkerFailure(200, "", st, f));

  // The Worker's own mid-stream error (solve.ts midStreamError): neutral wording.
  StreamReader cut;
  cut.feed(sse("message_start", "\"message\":{\"usage\":{\"input_tokens\":10,\"output_tokens\":1}}") +
           "event: content_block_delta\ndata: {\"type\":\"content_bl" +
           "\n\nevent: error\ndata: {\"type\":\"error\",\"error\":{\"type\":\"upstream_error\",\"message\":\"The AI reply "
           "was cut off. Please try again.\"}}\n\n");
  WCHECK(classifyWorkerFailure(200, "", cut, f));
  WCHECK(f.failure == Failure::ApiError && f.detail == "The AI reply was cut off. Please try again.");
  // An upstream error passed through as is: overloaded = busy; vendor text never reaches the screen.
  StreamReader over;
  over.feed("event: error\ndata: {\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\",\"message\":\"Overloaded\"}}\n\n");
  WCHECK(classifyWorkerFailure(200, "", over, f) && f.failure == Failure::ApiBusy);
  StreamReader other;
  other.feed("event: error\ndata: {\"type\":\"error\",\"error\":{\"type\":\"api_error\",\"message\":\"Anthropic internal\"}}\n\n");
  WCHECK(classifyWorkerFailure(200, "", other, f) && f.failure == Failure::ApiError);
  WCHECK(f.detail.find("Anthropic") == std::string::npos);
  // Refusal and a stream that just stops.
  StreamReader refused;
  refused.feed(sse("message_delta", "\"delta\":{\"stop_reason\":\"refusal\"}") + sse("message_stop", ""));
  WCHECK(classifyWorkerFailure(200, "", refused, f) && f.failure == Failure::ApiError);
  StreamReader dropped;
  dropped.feed(sse("message_start", "\"message\":{}"));
  WCHECK(classifyWorkerFailure(200, "", dropped, f) && f.failure == Failure::NoConnection);
}

void testSolveErrors() {
  const StreamReader none;
  WorkerFailure f;
  // 402 device_not_linked: the message carries the code (plan.ts notLinkedError).
  WCHECK(classifyWorkerFailure(402, errBody("device_not_linked", "Link this calculator: go to proofline.example/link and enter code ABCD-EFGH"),
                               none, f));
  WCHECK(f.failure == Failure::Account && f.action == WorkerAction::Unlinked);
  WCHECK(f.pairCode == "ABCD-EFGH" && f.site == "proofline.example");
  WCHECK(f.detail.rfind("Link this calculator", 0) == 0);
  // 402 subscription_inactive: the three plan messages.
  for (const char* m : {"Your free month is over. Subscribe at proofline.example/account to keep using AI.",
                        "Your subscription has ended. Renew at proofline.example/account to keep using AI.",
                        "No free month for this calculator. Subscribe at proofline.example/account to use AI."}) {
    WCHECK(classifyWorkerFailure(402, errBody("subscription_inactive", m), none, f));
    WCHECK(f.failure == Failure::Account && f.detail == m && f.action == WorkerAction::None);
  }
  // 429s.
  WCHECK(classifyWorkerFailure(429, errBody("quota_exceeded", "Monthly AI limit reached. It resets on the 1st."), none, f));
  WCHECK(f.failure == Failure::Account && f.detail == "Monthly AI limit reached. It resets on the 1st.");
  WCHECK(classifyWorkerFailure(429, errBody("rate_limited", "Too many solves in a minute. Please wait a moment."), none, f));
  WCHECK(f.failure == Failure::Account && f.detail.find("Too many solves") == 0);
  WCHECK(classifyWorkerFailure(429, errBody("rate_limited", "Another solve is still running. Please wait for it to finish."), none, f));
  WCHECK(f.failure == Failure::Account);
  // 401s: unknown token -> register again; revoked -> clear and register again.
  WCHECK(classifyWorkerFailure(401, errBody("unauthorized", "This calculator isn't registered. It will register again on its own."),
                               none, f));
  WCHECK(f.failure == Failure::Account && f.action == WorkerAction::Reregister);
  WCHECK(classifyWorkerFailure(401, errBody("device_revoked", "This calculator was removed from its account. Link it again in Apps > Settings."),
                               none, f));
  WCHECK(f.failure == Failure::Account && f.action == WorkerAction::Revoked && f.detail.find("removed") != std::string::npos);
  // Server trouble: busy, whatever the message.
  WCHECK(classifyWorkerFailure(503, errBody("spend_cap", "AI is busy right now. Please try again later."), none, f));
  WCHECK(f.failure == Failure::ApiBusy);
  WCHECK(classifyWorkerFailure(503, errBody("unavailable", "The AI server is having trouble. Please try again later."), none, f));
  WCHECK(f.failure == Failure::ApiBusy);
  WCHECK(classifyWorkerFailure(502, errBody("upstream_error", "The AI service had a problem. Please try again."), none, f));
  WCHECK(f.failure == Failure::ApiBusy);
  WCHECK(classifyWorkerFailure(502, "<html>Bad gateway</html>", none, f) && f.failure == Failure::ApiBusy);
  // Request problems: the message.
  WCHECK(classifyWorkerFailure(413, errBody("payload_too_large", "Photo too large. Please take it again."), none, f));
  WCHECK(f.failure == Failure::ApiError && f.detail == "Photo too large. Please take it again.");
  WCHECK(classifyWorkerFailure(400, errBody("bad_request", "A photo is damaged or not a JPEG/PNG."), none, f));
  WCHECK(f.failure == Failure::ApiError && f.detail == "A photo is damaged or not a JPEG/PNG.");
  WCHECK(classifyWorkerFailure(404, "", none, f) && f.failure == Failure::ApiError && f.detail.find("404") != std::string::npos);
  WCHECK(classifyWorkerFailure(403, "", none, f) && f.failure == Failure::Account && f.action == WorkerAction::None);
  // Vendor names never reach the screen, and messages stay within 110 bytes.
  WCHECK(classifyWorkerFailure(400, errBody("bad_request", "Claude rejected it"), none, f));
  WCHECK(f.detail.find("Claude") == std::string::npos);
  WCHECK(classifyWorkerFailure(402, errBody("subscription_inactive", std::string(300, 'x')), none, f) && f.detail.size() <= 110);
}

void testFirmware() {
  const std::string running = "v15lcd-2026.10.11";
  const std::string sha(64, 'a');
  FirmwareManifest m;
  std::string err;
  WCHECK(parseFirmwareManifest(200, "{\"update\":false}", running, m, err) && !m.available);  // nothing published
  const auto manifest = [&](const std::string& v) {
    return "{\"update\":true,\"version\":\"" + v + "\",\"size\":1200000,\"sha256\":\"" + sha +
           "\",\"path\":\"/v1/firmware/image\"}";
  };
  WCHECK(parseFirmwareManifest(200, manifest("v15lcd-2026.10.20"), running, m, err) && m.available);
  WCHECK(m.version == "v15lcd-2026.10.20" && m.size == 1200000 && m.sha256 == sha && m.path == "/v1/firmware/image");
  WCHECK(!m.rollback);
  // The Worker announces whatever was published last: a v14 image is not for this board.
  WCHECK(parseFirmwareManifest(200, manifest("v14-2026.10.20"), running, m, err) && !m.available && !m.note.empty());
  // Same version: up to date even if the server said update.
  WCHECK(parseFirmwareManifest(200, manifest(running), running, m, err) && !m.available);
  // Bad manifests.
  WCHECK(!parseFirmwareManifest(200, "{\"update\":true,\"version\":\"v15lcd-2\"}", running, m, err) && !m.available);
  WCHECK(!parseFirmwareManifest(200, "{\"update\":true,\"version\":\"v15lcd-2\",\"size\":5,\"sha256\":\"" + sha +
                                         "\",\"path\":\"/v1/firmware/image\"}", running, m, err));
  WCHECK(!parseFirmwareManifest(200, "{\"update\":true,\"version\":\"v15lcd-2\",\"size\":1200000,\"sha256\":\"zz\","
                                     "\"path\":\"/v1/firmware/image\"}", running, m, err));
  WCHECK(!parseFirmwareManifest(401, errBody("unauthorized", "This calculator isn't registered. It will register again on its own."),
                                running, m, err));
  WCHECK(err.find("isn't registered") != std::string::npos);
  WCHECK(!parseFirmwareManifest(500, "oops", running, m, err));
  // ?family= (Worker since 2026-10-11): the check path, and the manifest's own path as is.
  WCHECK(versionFamily(running) == "v15lcd" && versionFamily("v15lcd") == "v15lcd");
  WCHECK(firmwareCheckPath(running) == "/v1/firmware?family=v15lcd");
  WCHECK(firmwareCheckPath("stage14-2026.10.06") == "/v1/firmware?family=stage14");
  WCHECK(firmwareCheckPath("-2026") == "/v1/firmware");                      // no family
  WCHECK(firmwareCheckPath("v15_lcd-2026") == "/v1/firmware");               // not letters and digits
  WCHECK(firmwareCheckPath(std::string(25, 'a') + "-1") == "/v1/firmware");  // longer than 24
  const auto famManifest = [&](const std::string& v, const std::string& extra) {
    return "{\"update\":true,\"version\":\"" + v + "\",\"size\":1200000,\"sha256\":\"" + sha +
           "\",\"path\":\"/v1/firmware/image?family=v15lcd\"" + extra + "}";
  };
  WCHECK(parseFirmwareManifest(200, famManifest("v15lcd-2026.10.20", ",\"family\":\"v15lcd\",\"rollback\":false"), running,
                               m, err) && m.available);
  WCHECK(m.path == "/v1/firmware/image?family=v15lcd" && m.family == "v15lcd" && !m.rollback);
  WCHECK(parseFirmwareManifest(200, famManifest("v15lcd-2026.10.01", ",\"family\":\"v15lcd\",\"rollback\":true"), running,
                               m, err) && m.available && m.rollback);  // ota.cpp lets an older image in only then
  // Legacy channel: family null.
  const std::string legacy = manifest("v15lcd-2026.10.20");
  WCHECK(parseFirmwareManifest(200, legacy.substr(0, legacy.size() - 1) + ",\"family\":null,\"rollback\":false}", running,
                               m, err) && m.available && m.family.empty());
  // A family field that disagrees with ours: skipped even when the version looks like ours.
  WCHECK(parseFirmwareManifest(200, famManifest("v15lcd-2026.10.20", ",\"family\":\"v14\""), running, m, err) &&
         !m.available && !m.note.empty());
  // Nothing published for this family.
  WCHECK(parseFirmwareManifest(200, "{\"update\":false,\"family\":\"v15lcd\"}", running, m, err) && !m.available);
  // Paths outside the contract's pattern are refused.
  for (const char* bad : {"/v1/firmware/image?family=v15_lcd", "/v1/firmware/image?x=1", "https://evil.example/fw.bin",
                          "/v1/firmware/image?family=", "/v1/firmware/image ?family=v15lcd", "/v1/other"}) {
    const std::string b = "{\"update\":true,\"version\":\"v15lcd-2026.10.20\",\"size\":1200000,\"sha256\":\"" + sha +
                          "\",\"path\":\"" + bad + "\"}";
    WCHECK(!parseFirmwareManifest(200, b, running, m, err) && !m.available);
  }
}

void testHelpers() {
  WCHECK(siteFromLinkUrl("https://proofline.example/link?code=ABCD-EFGH") == "https://proofline.example");
  WCHECK(siteFromLinkUrl("https://proofline.example/link") == "https://proofline.example");
  WCHECK(siteFromLinkUrl("https://proofline.example/") == "https://proofline.example");
  WCHECK(siteFromLinkUrl("https://linker.example/linkage") == "https://linker.example/linkage");
  WCHECK(findPairCode("enter code ABCD-EFGH") == "ABCD-EFGH");
  WCHECK(findPairCode("ABCDEFGH") == "ABCD-EFGH");
  WCHECK(findPairCode("Code: AB0D-EFGH") == "");     // 0 is not in the alphabet
  WCHECK(findPairCode("XABCD-EFGHX") == "");          // part of a longer word
  WCHECK(findPairCode("Go to SITE.EXAMPLE and enter code WXYZ-2345") == "WXYZ-2345");
  WCHECK(neutralMessage("Claude is overloaded") == "AI is overloaded");
  WCHECK(neutralMessage("The Anthropic Claude API failed") == "The AI API failed");
  WCHECK(neutralMessage("") == "");
  // R-14 (bug hunt 2026-10-11): one case-insensitive pass over a capped input (a 16 KB body of
  // "claude " was O(n²)); still no vendor name, and nothing half-cut is left at the end.
  WCHECK(neutralMessage("CLAUDE and anthropic, ClAuDe") == "AI and AI, AI");
  WCHECK(neutralMessage("Anthropic Claude Claude says hi") == "AI says hi");
  {
    std::string big;
    for (int i = 0; i < 2340; ++i) big += "claude ";
    const std::string n = neutralMessage(big);
    WCHECK(n.size() <= kNeutralMaxInput && n.find("cl") == std::string::npos && n.find("AI AI") == std::string::npos);
    std::string mixed;
    for (int i = 0; i < 40; ++i) mixed += "anthropic is busy ";
    const std::string m = neutralMessage(mixed);
    WCHECK(m.size() <= kNeutralMaxInput && m.find("anth") == std::string::npos && m.find("AI is busy") == 0);
  }
  // R-13: numbers out of an integer's range (or NaN-like) never reach a double → int cast.
  {
    WorkerStatus s;
    std::string err;
    WCHECK(parseWorkerStatus(200,
                             "{\"linked\":true,\"plan\":\"trial\",\"can_solve\":true,\"solves_this_month\":1e300,"
                             "\"solves_limit\":-1e300,\"trial_ends_at\":1e30,\"period_end\":-5e19}",
                             s, err));
    WCHECK(s.solvesThisMonth == 0 && s.solvesLimit == 0 && s.trialEndsAt == 0 && s.periodEnd == 0);
    WCHECK(parseWorkerStatus(200, "{\"linked\":true,\"can_solve\":true,\"solves_this_month\":2147483647}", s, err));
    WCHECK(s.solvesThisMonth == 2147483647);
    FirmwareManifest m;
    const std::string sha(64, 'a');
    for (const char* size : {"1e300", "-1e300", "1e9999", "4294967296000"})
      WCHECK(!parseFirmwareManifest(200, std::string("{\"update\":true,\"version\":\"v15lcd-2\",\"size\":") + size +
                                             ",\"sha256\":\"" + sha + "\",\"path\":\"/v1/firmware/image\"}",
                                    "v15lcd-1", m, err) &&
             m.size == 0);
    WorkerRegistration r;
    WCHECK(parseRegisterReply(201,
                              "{\"token\":\"dt_" + std::string(43, 'A') + "\",\"device_id\":\"dev_" + std::string(16, 'a') +
                                  "\",\"expires_at\":1e308}",
                              r, err));
    WCHECK(r.expiresAt == 0);
  }
  WCHECK(workerErrorType(errBody("spend_cap", "x")) == "spend_cap");
  WCHECK(workerErrorType("not json").empty());
}

}  // namespace

int runWorkerApiTests() {
  g_fail = g_pass = 0;
  testIdentity();
  testRegister();
  testStatus();
  testStatusNewFields();
  testPairStart();
  testSolveRequest();
  testSolveReply();
  testSolveErrors();
  testFirmware();
  testHelpers();
  std::printf("worker api: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail;
}
