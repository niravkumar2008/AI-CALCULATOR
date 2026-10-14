#include "worker_api.h"

#include <cctype>

#include "json.h"
#include "text.h"

namespace calc {

namespace {

bool isB64Url(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}
bool isLowerHex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }

// prefix + exactly n characters that pass `ok`.
bool shaped(const std::string& s, const char* prefix, size_t n, bool (*ok)(char)) {
  const std::string p = prefix;
  if (s.size() != p.size() + n || s.compare(0, p.size(), p) != 0) return false;
  for (size_t i = p.size(); i < s.size(); ++i)
    if (!ok(s[i])) return false;
  return true;
}

// Pair-code alphabet: A-Z without I and O, 2-9.
bool isCodeChar(char c) { return (c >= 'A' && c <= 'Z' && c != 'I' && c != 'O') || (c >= '2' && c <= '9'); }

std::string jsonStr(const std::string& s) {
  std::string o = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') {
      o += '\\';
      o += static_cast<char>(c);
    } else if (c < 0x20) {
      static const char* hex = "0123456789abcdef";
      o += "\\u00";
      o += hex[c >> 4];
      o += hex[c & 15];
    } else {
      o += static_cast<char>(c);
    }
  }
  return o + "\"";
}

bool isPng(const std::string& bytes) { return bytes.compare(0, 8, std::string("\x89PNG\r\n\x1a\n", 8)) == 0; }

std::string imagePart(const std::string& bytes) {
  return std::string("{\"media_type\":\"") + (isPng(bytes) ? "image/png" : "image/jpeg") + "\",\"data\":\"" +
         base64Encode(bytes) + "\"}";
}

// A JSON number as an integer in [lo, hi]: 0 when it is missing, not finite or outside (a
// double → integer cast of 1e300 or NaN is undefined behaviour).
long long intIn(const Json& j, double lo, double hi) {
  if (!j.isNumber()) return 0;
  const double v = j.asNumber();
  if (!(v >= lo && v <= hi)) return 0;
  return static_cast<long long>(v);
}
// Unix times and the like: anything a 64-bit integer holds exactly enough (±9E18).
long long intOf(const Json& j) { return intIn(j, -9.0e18, 9.0e18); }
int smallIntOf(const Json& j) { return static_cast<int>(intIn(j, -2147483647.0, 2147483647.0)); }

// The server's message for a non-2xx reply, else a fallback naming the status.
std::string errorText(int status, const std::string& body, const char* what) {
  const std::string m = workerErrorMessage(body);
  if (!m.empty()) return m;
  if (status == 0) return "No reply from the AI server.";
  return std::string(what) + " (HTTP " + std::to_string(status) + ").";
}

std::string lowerCopy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

bool isFamily(const std::string& f) {
  if (f.empty() || f.size() > 24) return false;
  for (char c : f)
    if (!std::isalnum(static_cast<unsigned char>(c))) return false;
  return true;
}

// A manifest path the firmware may GET: /v1/firmware/image, optionally ?family=<1-24 alnum>
// (the contract's pattern). Older proxies only ever sent the bare path.
bool isImagePath(const std::string& p) {
  const std::string base = "/v1/firmware/image";
  if (p.compare(0, base.size(), base) != 0) return false;
  if (p.size() == base.size()) return true;
  const std::string q = "?family=";
  return p.compare(base.size(), q.size(), q) == 0 && isFamily(p.substr(base.size() + q.size()));
}

// "go to <site>/link" in a not-linked message -> "<site>" ("" if absent).
std::string siteFromReason(const std::string& reason) {
  const size_t g = reason.find("go to ");
  if (g == std::string::npos) return "";
  size_t b = g + 6, e = b;
  while (e < reason.size() && reason[e] != ' ') ++e;
  return siteFromLinkUrl(reason.substr(b, e - b));
}

}  // namespace

std::string versionFamily(const std::string& v) { return v.substr(0, v.find('-')); }

std::string firmwareCheckPath(const std::string& running) {
  const std::string f = versionFamily(running);
  return isFamily(f) ? "/v1/firmware?family=" + f : "/v1/firmware";
}

std::string planStatusText(const std::string& planReason, const std::string& site, const std::string& reason) {
  std::string host = site;
  for (const char* scheme : {"https://", "http://"})
    if (host.rfind(scheme, 0) == 0) host.erase(0, std::string(scheme).size());
  while (!host.empty() && host.back() == '/') host.pop_back();
  const std::string at = host.empty() ? "on the website" : "at " + host;
  if (planReason == "not_linked") return "";
  if (planReason == "trial") return "Free month active";
  if (planReason == "active") return "AI tutor plan active";
  if (planReason == "past_due") return "Payment problem: check your account";
  if (planReason == "free_month_over") return "Free month over: subscribe " + at;
  if (planReason == "inactive") return "Plan ended: subscribe " + at;
  if (planReason == "no_trial") return "No free month for this calculator";
  return reason;
}

std::string pairQrUrl(const std::string& linkUrl, const std::string& code) {
  if (linkUrl.empty() || code.empty()) return "";
  return linkUrl + (linkUrl.find('?') == std::string::npos ? "?code=" : "&code=") + code;
}

bool isWorkerHwId(const std::string& s) { return shaped(s, "calc-", 12, isLowerHex); }
bool isWorkerHwSecret(const std::string& s) { return shaped(s, "hs_", 43, isB64Url); }
bool isWorkerDeviceToken(const std::string& s) { return shaped(s, "dt_", 43, isB64Url); }
bool isWorkerDeviceId(const std::string& s) { return shaped(s, "dev_", 16, isLowerHex); }

std::string buildRegisterBody(const std::string& hwId, const std::string& hwSecret) {
  std::string b = "{\"hw_id\":" + jsonStr(hwId);
  if (isWorkerHwSecret(hwSecret)) b += ",\"hw_secret\":" + jsonStr(hwSecret);
  return b + "}";
}

std::string siteFromLinkUrl(const std::string& linkUrl) {
  std::string u = linkUrl;
  const size_t scheme = u.find("://");
  const size_t from = scheme == std::string::npos ? 0 : scheme + 3;
  const size_t link = u.find("/link", from);
  if (link != std::string::npos &&
      (link + 5 == u.size() || u[link + 5] == '?' || u[link + 5] == '/' || u[link + 5] == '#'))
    u.resize(link);
  while (!u.empty() && u.back() == '/') u.pop_back();
  return u;
}

std::string findPairCode(const std::string& text) {
  auto at = [&](size_t i) -> std::string {  // a code starting at i, or ""
    auto edge = [&](size_t k) { return k >= text.size() || !std::isalnum(static_cast<unsigned char>(text[k])); };
    if (i > 0 && !edge(i - 1)) return "";
    if (i + 8 > text.size()) return "";
    for (size_t k = 0; k < 4; ++k)
      if (!isCodeChar(text[i + k])) return "";
    size_t j = i + 4;
    if (j < text.size() && text[j] == '-') ++j;
    if (j + 4 > text.size()) return "";
    for (size_t k = 0; k < 4; ++k)
      if (!isCodeChar(text[j + k])) return "";
    if (!edge(j + 4)) return "";
    return text.substr(i, 4) + "-" + text.substr(j, 4);
  };
  const size_t c = text.find("code ");
  if (c != std::string::npos) {
    const std::string s = at(c + 5);
    if (!s.empty()) return s;
  }
  for (size_t i = 0; i < text.size(); ++i) {
    const std::string s = at(i);
    if (!s.empty()) return s;
  }
  return "";
}

std::string neutralMessage(const std::string& msg) {
  // Every caller shows at most 110 bytes: look at no more than twice that (a 16 KB error body
  // of "claude claude ..." took O(n²) with a lower-cased copy per replacement).
  std::string in = utf8Prefix(msg, kNeutralMaxInput);
  if (in.size() < msg.size()) {  // cut at a word's end, so no half "claude" is left
    const size_t sp = in.find_last_of(' ');
    if (sp != std::string::npos && sp > kNeutralMaxInput / 2) in.resize(sp);
  }
  auto startsWithCI = [&](size_t p, const char* word) {
    for (size_t k = 0; word[k]; ++k)
      if (p + k >= in.size() || std::tolower(static_cast<unsigned char>(in[p + k])) != word[k]) return false;
    return true;
  };
  std::string out;
  out.reserve(in.size());
  for (size_t p = 0; p < in.size();) {
    if (startsWithCI(p, "anthropic")) {
      out += "AI";
      p += 9;
    } else if (startsWithCI(p, "claude")) {
      out += "AI";
      p += 6;
    } else {
      out += in[p++];
    }
  }
  // "AI AI" (e.g. from "Anthropic Claude") reads badly: one pass, each "AI " before "AI" goes.
  std::string tidy;
  tidy.reserve(out.size());
  for (size_t p = 0; p < out.size();) {
    if (out.compare(p, 5, "AI AI") == 0) {
      p += 3;
      continue;
    }
    tidy += out[p++];
  }
  return tidy;
}

std::string workerErrorType(const std::string& body) {
  Json j;
  std::string err;
  if (!Json::parse(body, j, err) || !j["error"]["type"].isString()) return "";
  return j["error"]["type"].asString();
}

std::string workerErrorMessage(const std::string& body) {
  Json j;
  std::string err;
  if (!Json::parse(body, j, err) || !j["error"]["message"].isString()) return "";
  return utf8Prefix(neutralMessage(j["error"]["message"].asString()), 110);
}

bool parseRegisterReply(int status, const std::string& body, WorkerRegistration& out, std::string& error) {
  out = WorkerRegistration{};
  error.clear();
  if (status != 201) {
    error = errorText(status, body, "Registration failed");
    return false;
  }
  Json j;
  std::string err;
  if (!Json::parse(body, j, err)) {
    error = "The registration reply couldn't be read.";
    return false;
  }
  if (!j["token"].isString() || !isWorkerDeviceToken(j["token"].asString()) || !j["device_id"].isString() ||
      !isWorkerDeviceId(j["device_id"].asString())) {
    error = "The registration reply has no valid token.";
    return false;
  }
  out.token = j["token"].asString();
  out.deviceId = j["device_id"].asString();
  if (j["pair_code"].isString()) out.pairCode = findPairCode(j["pair_code"].asString());
  if (j["link_url"].isString()) {
    out.linkUrl = j["link_url"].asString();
    out.site = siteFromLinkUrl(out.linkUrl);
  }
  out.expiresAt = intOf(j["expires_at"]);
  return true;
}

bool parseWorkerStatus(int status, const std::string& body, WorkerStatus& out, std::string& error) {
  out = WorkerStatus{};
  error.clear();
  if (status != 200) {
    error = errorText(status, body, "Status check failed");
    return false;
  }
  Json j;
  std::string err;
  if (!Json::parse(body, j, err) || !j["linked"].isBool() || !j["can_solve"].isBool()) {
    error = "The status reply couldn't be read.";
    return false;
  }
  out.linked = j["linked"].asBool();
  out.canSolve = j["can_solve"].asBool();
  if (j["plan"].isString()) out.plan = j["plan"].asString();
  if (j["reason"].isString()) out.reason = utf8Prefix(neutralMessage(j["reason"].asString()), 110);
  if (j["account_hint"].isString()) out.accountHint = utf8Prefix(j["account_hint"].asString(), 40);
  out.trialEndsAt = intOf(j["trial_ends_at"]);
  out.periodEnd = intOf(j["period_end"]);
  if (j["solves_this_month"].isNumber()) out.solvesThisMonth = smallIntOf(j["solves_this_month"]);
  if (j["solves_limit"].isNumber()) out.solvesLimit = smallIntOf(j["solves_limit"]);
  // Added 2026-10-11 (older servers omit them): prefer them, else read `reason`.
  if (j["link_url"].isString()) {
    out.linkUrl = j["link_url"].asString();
    out.site = siteFromLinkUrl(out.linkUrl);
  }
  if (j["plan_reason"].isString()) {
    const std::string r = j["plan_reason"].asString();
    for (const char* known : {"not_linked", "trial", "active", "past_due", "free_month_over", "inactive", "no_trial"})
      if (r == known) out.planReason = r;
  }
  if (!out.linked) {
    if (j["pair_code"].isString()) out.pairCode = findPairCode(j["pair_code"].asString());  // null when linked
    if (out.pairCode.empty()) out.pairCode = findPairCode(out.reason);
    if (out.site.empty()) out.site = siteFromReason(out.reason);
  }
  return true;
}

bool parsePairStart(int status, const std::string& body, WorkerPairing& out, std::string& error) {
  out = WorkerPairing{};
  error.clear();
  if (status != 200 && status != 201) {
    error = errorText(status, body, "No link code");
    return false;
  }
  Json j;
  std::string err;
  if (!Json::parse(body, j, err) || !j["already_linked"].isBool()) {
    error = "The link code reply couldn't be read.";
    return false;
  }
  out.alreadyLinked = j["already_linked"].asBool();
  if (out.alreadyLinked) return true;
  if (j["code"].isString()) out.code = findPairCode(j["code"].asString());
  if (out.code.empty()) {
    error = "The link code reply has no code.";
    return false;
  }
  if (j["link_url"].isString()) {
    out.linkUrl = j["link_url"].asString();
    out.site = siteFromLinkUrl(out.linkUrl);
  }
  out.expiresAt = intOf(j["expires_at"]);
  return true;
}

std::string buildWorkerSolveRequest(const std::string& image, const std::string& detail, bool tutor) {
  std::string b = "{\"images\":[" + imagePart(image);
  if (!detail.empty()) b += "," + imagePart(detail);
  b += "]";
  if (tutor) b += ",\"tutor\":true";
  return b + "}";
}

bool classifyWorkerFailure(int status, const std::string& body, const StreamReader& stream, WorkerFailure& out) {
  out = WorkerFailure{};
  if (status == 200) {
    if (!stream.errorType().empty()) {  // an error event in the stream (the Worker's own, or passed through)
      const std::string& t = stream.errorType();
      if (t == "overloaded_error" || t == "rate_limit_error" || t == "spend_cap" || t == "unavailable") {
        out.failure = Failure::ApiBusy;
      } else {
        out.failure = Failure::ApiError;
        out.detail = t == "upstream_error" && !stream.errorMessage().empty()
                         ? utf8Prefix(neutralMessage(stream.errorMessage()), 70)
                         : "The AI reply was cut off. Please try again.";
      }
      return true;
    }
    std::string detail;
    Failure f;
    if (!classifyFailure(200, "", stream, f, detail)) return false;  // refusal, max_tokens, cut off
    out.failure = f;
    out.detail = detail;
    return true;
  }

  const std::string type = workerErrorType(body);
  const std::string msg = workerErrorMessage(body);
  auto account = [&](const char* fallback, WorkerAction a) {
    out.failure = Failure::Account;
    out.detail = msg.empty() ? fallback : msg;
    out.action = a;
    return true;
  };
  if (type == "unauthorized")
    return account("This calculator isn't registered. It will register again on its own.", WorkerAction::Reregister);
  if (type == "device_revoked")
    return account("This calculator was removed from its account. Link it again in Apps > Settings.",
                   WorkerAction::Revoked);
  if (type == "device_not_linked") {
    account("Link this calculator first: Apps > Settings > Link calculator.", WorkerAction::Unlinked);
    out.pairCode = findPairCode(msg);
    out.site = siteFromReason(msg);
    return true;
  }
  if (type == "subscription_inactive") return account("AI needs a subscription for this calculator.", WorkerAction::None);
  if (type == "quota_exceeded") return account("Monthly AI limit reached.", WorkerAction::None);
  if (type == "rate_limited") return account("Too many solves. Please wait a moment.", WorkerAction::None);
  if (type == "spend_cap" || type == "unavailable" || type == "upstream_error") {
    out.failure = Failure::ApiBusy;
    out.detail = msg;
    return true;
  }
  if (type == "payload_too_large" || type == "bad_request") {
    out.failure = Failure::ApiError;
    out.detail = msg.empty() ? "The photo couldn't be sent. Please take it again." : utf8Prefix(msg, 70);
    return true;
  }
  // No (known) error body: a proxy in front, an HTML error page, an unexpected status.
  if (status == 401 || status == 403) {
    out.failure = Failure::Account;
    out.detail = "The AI server didn't accept this calculator.";
    return true;
  }
  if (status == 0) {
    out.failure = Failure::NoConnection;
    return true;
  }
  if (status == 429 || status >= 500) {
    out.failure = Failure::ApiBusy;
    return true;
  }
  out.failure = Failure::ApiError;
  out.detail = msg.empty() ? "The AI server refused the request (HTTP " + std::to_string(status) + ")." : utf8Prefix(msg, 70);
  return true;
}

bool parseFirmwareManifest(int status, const std::string& body, const std::string& running, FirmwareManifest& out,
                           std::string& error) {
  out = FirmwareManifest{};
  error.clear();
  Json j;
  std::string err;
  if (!Json::parse(body, j, err)) {
    error = "The update manifest couldn't be read (HTTP " + std::to_string(status) + ").";
    return false;
  }
  if (status != 200) {
    error = errorText(status, body, "The AI server refused the update check");
    return false;
  }
  out.available = j["update"].isBool() && j["update"].asBool();
  if (j["version"].isString()) out.version = j["version"].asString();
  if (!out.available) return true;
  if (!j["sha256"].isString() || !j["size"].isNumber() || !j["path"].isString()) {
    error = "The update manifest is incomplete.";
    out.available = false;
    return false;
  }
  out.sha256 = lowerCopy(j["sha256"].asString());
  // An image is at most a 1.9 MB app slot; past 1E9 (or NaN) it is 0, which fails the check below.
  out.size = static_cast<size_t>(intIn(j["size"], 0, 1e9));
  out.path = j["path"].asString();
  out.rollback = j["rollback"].isBool() && j["rollback"].asBool();
  if (j["family"].isString()) out.family = j["family"].asString();  // null: the legacy channel
  bool hex = out.sha256.size() == 64;
  for (char c : out.sha256) hex = hex && isLowerHex(c);
  if (!hex || out.size < 100000 || !isImagePath(out.path)) {
    error = "The update manifest looks wrong.";
    out.available = false;
    return false;
  }
  if (out.version == running) {  // the Worker compares strings; say "up to date" ourselves too
    out.available = false;
    return true;
  }
  // Our own family check even though the Worker filters by ?family=: the legacy channel (an
  // older server, or no family sent) announces any board's image.
  const std::string mine = versionFamily(running);
  if (versionFamily(out.version) != mine || (!out.family.empty() && out.family != mine)) {
    out.note = "The published firmware " + out.version + " is for another board: skipped.";
    out.available = false;
  }
  return true;
}

}  // namespace calc
