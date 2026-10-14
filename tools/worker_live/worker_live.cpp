// A pretend calculator for the Cloudflare Worker API, built from the firmware's own request
// and reply code (core/worker_api.*, core/claude_api.*): what the v15 firmware sends and how it
// reads the answers, minus the ESP32 HTTPS. Windows (WinHTTP); see build.bat.
//
//   worker_live register [base-url]   register like a first boot (hw_secret from env HW_SECRET if set;
//                                     the hw_id is kept between runs, env HW_ID overrides it)
//   worker_live status                what the Link screen / AI SOLVE would learn
//   worker_live poll                  status every 5 s until linked (max 15 min), as the Link screen does
//   worker_live code                  a fresh link code (POST /v1/pair/start)
//   worker_live solve [photo.jpg|png] AI SOLVE: send a photo, stream the reply, print what the screen shows
//   worker_live typed ["2x+3=11"]     Type it: the typed problem as a PNG, then as solve
//   worker_live firmware              the update check (?family=; FETCH_IMAGE=1 also downloads the image)
//
// base-url defaults to http://127.0.0.1:8787 (wrangler dev). The identity (hw_id, token) is kept in
// %TEMP%\aicalc-worker-live.txt. Against a local dev Worker it sends x-dev-client-ip (as
// web/dev/calculator.mjs does) so its 5-per-hour registration limit is per pretend network.
#include <windows.h>
#include <winhttp.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <string>

#include "../../core/claude_api.h"
#include "../../core/png.h"
#include "../../core/worker_api.h"

#pragma comment(lib, "winhttp.lib")

using namespace calc;

namespace {

const char* kFirmware = "v15lcd-2026.10.11";  // what the firmware sends as x-firmware

struct State {
  std::string url = "http://127.0.0.1:8787", hwId, token, deviceId;
};

std::string statePath() {
  char tmp[MAX_PATH];
  GetTempPathA(MAX_PATH, tmp);
  return std::string(tmp) + "aicalc-worker-live.txt";
}

State load() {
  State s;
  std::ifstream f(statePath());
  std::string k, v;
  while (f >> k >> v) {
    if (k == "url") s.url = v;
    else if (k == "hw_id") s.hwId = v;
    else if (k == "token") s.token = v;
    else if (k == "device_id") s.deviceId = v;
  }
  return s;
}

void save(const State& s) {
  std::ofstream f(statePath());
  f << "url " << s.url << "\nhw_id " << s.hwId << "\ntoken " << s.token << "\ndevice_id " << s.deviceId << "\n";
}

std::wstring wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }

struct Reply {
  int status = 0;
  std::string body;
};

// One request. `onChunk` (optional) sees the body as it arrives (the SSE stream).
Reply request(const State& st, const char* method, const std::string& path, const std::string& body, bool auth,
              const std::function<void(const std::string&)>& onChunk = nullptr) {
  Reply r;
  URL_COMPONENTS uc{};
  uc.dwStructSize = sizeof uc;
  wchar_t host[256], urlPath[512];
  uc.lpszHostName = host;
  uc.dwHostNameLength = 256;
  uc.lpszUrlPath = urlPath;
  uc.dwUrlPathLength = 512;
  const std::wstring wurl = wide(st.url);
  if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
    r.body = "bad url";
    return r;
  }
  const bool tls = uc.nScheme == INTERNET_SCHEME_HTTPS;
  HINTERNET ses = WinHttpOpen(L"aicalc-worker-live", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
  HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0);
  const std::wstring fullPath = std::wstring(urlPath) + wide(path);
  HINTERNET req = WinHttpOpenRequest(con, wide(method).c_str(), fullPath.c_str(), nullptr, nullptr, nullptr,
                                     tls ? WINHTTP_FLAG_SECURE : 0);
  WinHttpSetTimeouts(req, 15000, 15000, 30000, 130000);  // receive: the firmware's 120 s idle limit
  std::string h;
  if (auth) h += "authorization: Bearer " + st.token + "\r\n";
  h += "x-device-id: " + st.hwId + "\r\nx-firmware: " + kFirmware + "\r\ncontent-type: application/json\r\n";
  const bool local = st.url.find("127.0.0.1") != std::string::npos || st.url.find("localhost") != std::string::npos;
  if (local) h += "x-dev-client-ip: 10.77.0.15\r\n";
  const std::wstring wh = wide(h);
  if (!req || !WinHttpSendRequest(req, wh.c_str(), DWORD(-1L), (LPVOID)body.data(), DWORD(body.size()),
                                  DWORD(body.size()), 0) ||
      !WinHttpReceiveResponse(req, nullptr)) {
    r.body = "no connection (error " + std::to_string(GetLastError()) + ")";
  } else {
    DWORD code = 0, sz = sizeof code;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &code, &sz, nullptr);
    r.status = int(code);
    for (;;) {
      DWORD avail = 0;
      if (!WinHttpQueryDataAvailable(req, &avail) || avail == 0) break;
      std::string chunk(avail, '\0');
      DWORD got = 0;
      if (!WinHttpReadData(req, &chunk[0], avail, &got) || got == 0) break;
      chunk.resize(got);
      r.body += chunk;
      if (onChunk) onChunk(chunk);
    }
  }
  if (req) WinHttpCloseHandle(req);
  if (con) WinHttpCloseHandle(con);
  if (ses) WinHttpCloseHandle(ses);
  return r;
}

void screen(const std::string& title, const std::string& text) {
  std::printf("  +---------------------------+\n  | %-25s |\n", title.c_str());
  std::string line;
  std::istringstream words(text);
  std::string w;
  while (words >> w) {
    if (!line.empty() && line.size() + 1 + w.size() > 25) {
      std::printf("  | %-25s |\n", line.c_str());
      line.clear();
    }
    line += (line.empty() ? "" : " ") + w;
  }
  if (!line.empty()) std::printf("  | %-25s |\n", line.c_str());
  std::printf("  +---------------------------+\n");
}

int cmdRegister(State st, const char* url) {
  if (url) st.url = url;
  while (!st.url.empty() && st.url.back() == '/') st.url.pop_back();
  if (st.hwId.empty()) {
    std::mt19937_64 rng(std::random_device{}());
    char id[24];
    std::snprintf(id, sizeof id, "calc-%012llx", (unsigned long long)(rng() & 0xffffffffffffULL));
    st.hwId = id;
  }
  const char* secret = std::getenv("HW_SECRET");
  const std::string body = buildRegisterBody(st.hwId, secret ? secret : "");
  const Reply r = request(st, "POST", "/v1/device/register", body, false);
  WorkerRegistration reg;
  std::string err;
  if (!parseRegisterReply(r.status, r.body, reg, err)) {
    std::printf("register: HTTP %d: %s\n", r.status, err.c_str());
    return 1;
  }
  st.token = reg.token;
  st.deviceId = reg.deviceId;
  save(st);
  std::printf("registered %s as %s (%s), token saved (not shown)\n", st.hwId.c_str(), reg.deviceId.c_str(),
              isWorkerHwSecret(secret ? secret : "") ? "with hw_secret" : "not enrolled");
  screen("LINK THIS CALCULATOR", "Go to " + reg.site + "/link and type this code: " + reg.pairCode);
  std::printf("link_url %s  (Device::setLinkInfo(\"%s\", \"%s\"))\n", reg.linkUrl.c_str(), reg.pairCode.c_str(),
              reg.site.c_str());
  return 0;
}

int cmdStatus(const State& st, bool quiet = false, bool* linked = nullptr) {
  const Reply r = request(st, "GET", "/v1/device/status", "", true);
  WorkerStatus s;
  std::string err;
  if (!parseWorkerStatus(r.status, r.body, s, err)) {
    std::printf("status: HTTP %d: %s (error type %s)\n", r.status, err.c_str(), workerErrorType(r.body).c_str());
    return 1;
  }
  if (linked) *linked = s.linked;
  if (quiet && !s.linked) return 0;
  std::printf("status: linked=%s plan=%s can_solve=%s account=%s solves=%d/%d trial_ends_at=%lld\n",
              s.linked ? "yes" : "no", s.plan.c_str(), s.canSolve ? "yes" : "no", s.accountHint.c_str(),
              s.solvesThisMonth, s.solvesLimit, s.trialEndsAt);
  if (!s.reason.empty()) std::printf("reason: %s\n", s.reason.c_str());
  // Fields added 2026-10-11 (empty = the server didn't send them, or plan_reason was unknown).
  std::printf("plan_reason=%s link_url=%s\n", s.planReason.empty() ? "(none)" : s.planReason.c_str(),
              s.linkUrl.empty() ? "(none)" : s.linkUrl.c_str());
  if (!s.linked) {
    std::printf("link screen: code %s at %s/link\n", s.pairCode.c_str(), s.site.c_str());
    if (!s.linkUrl.empty()) std::printf("QR: %s\n", pairQrUrl(s.linkUrl, s.pairCode).c_str());
  } else {
    std::printf("link screen plan line: \"%s\"\n", planStatusText(s.planReason, s.site, s.reason).c_str());
  }
  return 0;
}

int cmdPoll(const State& st) {
  for (int i = 0; i < 15 * 60 / 5; ++i) {
    bool linked = false;
    if (cmdStatus(st, i > 0, &linked)) return 1;
    if (linked) {
      std::printf("LINKED after %d s\n", i * 5);
      return cmdStatus(st);
    }
    Sleep(5000);
  }
  std::printf("not linked after 15 minutes\n");
  return 1;
}

int cmdCode(const State& st) {
  const Reply r = request(st, "POST", "/v1/pair/start", "", true);
  WorkerPairing p;
  std::string err;
  if (!parsePairStart(r.status, r.body, p, err)) {
    std::printf("code: HTTP %d: %s\n", r.status, err.c_str());
    return 1;
  }
  if (p.alreadyLinked) std::printf("already linked\n");
  else screen("LINK THIS CALCULATOR", "Go to " + p.site + "/link and type this code: " + p.code);
  return 0;
}

std::string demoPhoto() {  // the same tiny JPEG-shaped picture as web/dev/calculator.mjs
  std::string b("\xff\xd8\xff\xe0\x00\x10\x4a\x46\x49\x46\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00", 20);
  b += "LOCAL DEMO PHOTO";
  b += std::string(600, ' ');
  b += std::string("\xff\xd9", 2);
  return b;
}

int cmdSolve(const State& st, const char* file, bool tutor) {
  std::string photo = demoPhoto();
  if (file) {
    std::ifstream f(file, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    photo = ss.str();
  }
  const std::string body = buildWorkerSolveRequest(photo, "", tutor);
  StreamReader stream;
  std::string shown;
  const DWORD t0 = GetTickCount();
  int status = 0;
  const Reply r = request(st, "POST", "/v1/solve", body, true, [&](const std::string& chunk) {
    stream.feed(chunk);
    double conf;
    std::string answer;
    if (peekAnswer(stream.text(), conf, answer) && answer != shown) {
      shown = answer;
      std::printf("partial answer after %lu ms: %s (confidence %.2f)\n", GetTickCount() - t0, answer.c_str(), conf);
    }
  });
  status = r.status;
  if (status != 200) stream = StreamReader{};
  WorkerFailure wf;
  if (classifyWorkerFailure(status, status == 200 ? "" : r.body, stream, wf)) {
    static const char* names[] = {"NoConnection", "NoApiKey", "Timeout", "ApiBusy", "ApiError",
                                  "BadReply",     "Camera",   "Account", "LowBattery"};
    static const char* actions[] = {"None", "Reregister", "Revoked", "Unlinked"};
    std::printf("solve: HTTP %d -> Failure::%s, action %s, code %s\n", status, names[int(wf.failure)],
                actions[int(wf.action)], wf.pairCode.c_str());
    screen(wf.failure == Failure::Account ? "AI ACCOUNT" : wf.failure == Failure::ApiBusy ? "AI BUSY" : "AI ERROR",
           wf.detail);
    return 2;
  }
  std::printf("reply after %lu ms (%zu bytes): %s\n", GetTickCount() - t0, stream.text().size(), stream.text().c_str());
  return 0;
}

int cmdFirmware(const State& st) {
  const std::string path = firmwareCheckPath(kFirmware);  // as ota.cpp: "/v1/firmware?family=v15lcd"
  const Reply r = request(st, "GET", path, "", true);
  FirmwareManifest m;
  std::string err;
  std::printf("GET %s -> HTTP %d\n", path.c_str(), r.status);
  if (!parseFirmwareManifest(r.status, r.body, kFirmware, m, err)) {
    std::printf("firmware: HTTP %d: %s\n", r.status, err.c_str());
    return 1;
  }
  const std::string note = m.note.empty() ? "" : "  (" + m.note + ")";
  std::printf("firmware: %s%s%s\n", m.available ? "update " : "no update", m.available ? m.version.c_str() : "",
              note.c_str());
  if (!m.path.empty())
    std::printf("manifest: version %s family %s rollback %s size %zu path %s\n", m.version.c_str(),
                m.family.empty() ? "(none)" : m.family.c_str(), m.rollback ? "true" : "false", m.size, m.path.c_str());
  if (m.available && std::getenv("FETCH_IMAGE")) {  // what otaInstall downloads: the path as given
    const Reply img = request(st, "GET", m.path, "", true);
    std::printf("image: HTTP %d, %zu bytes (manifest says %zu)\n", img.status, img.body.size(), m.size);
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string cmd = argc > 1 ? argv[1] : "status";
  State st = load();
  if (cmd == "register") {  // a new token; the same board (hw_id) unless HW_ID says otherwise, as a real board
    State fresh;
    fresh.url = st.url;
    fresh.hwId = std::getenv("HW_ID") ? std::getenv("HW_ID") : st.hwId;
    return cmdRegister(fresh, argc > 2 ? argv[2] : nullptr);
  }
  if (st.token.empty()) {
    std::printf("No calculator yet: worker_live register [http://127.0.0.1:8787]\n");
    return 1;
  }
  if (cmd == "status") return cmdStatus(st);
  if (cmd == "poll") return cmdPoll(st);
  if (cmd == "code") return cmdCode(st);
  if (cmd == "solve") return cmdSolve(st, argc > 2 ? argv[2] : nullptr, std::getenv("TUTOR") != nullptr);
  if (cmd == "typed") {  // Apps > Type it: the typed problem as a PNG (core/png.h), sent as image/png
    const std::string text = argc > 2 ? argv[2] : "2x+3=11";
    const std::string png = renderTextPng(text);
    char tmp[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp);
    const std::string path = std::string(tmp) + "aicalc-worker-live-typed.png";
    std::ofstream(path, std::ios::binary) << png;
    std::printf("typed \"%s\" -> %zu-byte PNG\n", text.c_str(), png.size());
    return cmdSolve(st, path.c_str(), std::getenv("TUTOR") != nullptr);
  }
  if (cmd == "firmware") return cmdFirmware(st);
  std::printf("Commands: register [url], status, poll, code, solve [photo], typed [text], firmware\n");
  return 1;
}
