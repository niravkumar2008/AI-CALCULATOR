#include "claude_client.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <atomic>

#include "claude_api.h"
#include "root_ca.h"
#include "worker_api.h"

using namespace calc;

namespace {
constexpr uint32_t kConnectMs = 15000;
constexpr uint32_t kIdleMs = 120000;  // no bytes for 2 minutes = timed out (as in the simulator)

// Behind the proxy the system prompt is the server's (server/proxy/prompts), so it need not be
// uploaded (~5 KB per solve). Default 1 = sent (what older proxies need); platformio.ini sets 0
// where the new proxy is assumed. Build with 1 for the proxy's PROMPT_SOURCE=device bench mode.
#ifndef CALC_SEND_SYSTEM_PROMPT
#define CALC_SEND_SYSTEM_PROMPT 1
#endif

// millis() when the last byte went out or came in on any proxy connection; the preview
// Send in main.cpp uses it as an idle timer (the proxy's ": ping" every 15 s counts).
std::atomic<uint32_t> g_lastActivityMs{0};
void noteActivity() { g_lastActivityMs.store(millis(), std::memory_order_relaxed); }

#ifndef CALC_ALLOW_HTTP
#define CALC_ALLOW_HTTP 0  // 1: a bench build may talk plain http to a Worker on the LAN (wrangler dev)
#endif

struct Url {
  std::string host, base;  // base: path prefix without trailing slash ("" or "/calc")
  uint16_t port = 443;
  bool tls = true;
};

bool parseUrl(const std::string& u, Url& out) {
  size_t skip = 8;
  if (u.rfind("https://", 0) != 0) {
    if (!CALC_ALLOW_HTTP || u.rfind("http://", 0) != 0) return false;
    skip = 7;
    out.tls = false;
    out.port = 80;
  }
  std::string rest = u.substr(skip);
  const size_t slash = rest.find('/');
  std::string hostPort = rest.substr(0, slash);
  out.base = slash == std::string::npos ? "" : rest.substr(slash);
  while (!out.base.empty() && out.base.back() == '/') out.base.pop_back();
  const size_t colon = hostPort.find(':');
  out.host = hostPort.substr(0, colon);
  if (colon != std::string::npos) out.port = uint16_t(atoi(hostPort.c_str() + colon + 1));
  return !out.host.empty() && out.port != 0;
}


// One connection: TLS normally, plain TCP only to an http:// Worker in a CALC_ALLOW_HTTP build.
// Reads and writes go through `c` (WiFiClientSecure overrides WiFiClient's virtuals).
struct Conn {
  WiFiClientSecure tls;
  WiFiClient plain;
  WiFiClient* c = &tls;
  void stop() { c->stop(); }
};

// Reads one header line (without CRLF). False on timeout or dropped connection.
bool readLine(WiFiClient& c, std::string& line, const std::function<bool()>& cancelled) {
  line.clear();
  uint32_t last = millis();
  while (true) {
    if (cancelled && cancelled()) return false;
    int ch = c.read();
    if (ch < 0) {
      if (!c.connected() || millis() - last > kIdleMs) return false;
      delay(5);
      continue;
    }
    last = millis();
    noteActivity();
    if (ch == '\n') {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      return true;
    }
    if (line.size() < 8192) line += static_cast<char>(ch);
  }
}

bool writeAll(WiFiClient& c, const char* p, size_t n) {
  while (n) {
    size_t w = c.write(reinterpret_cast<const uint8_t*>(p), n < 4096 ? n : 4096);
    if (w == 0) return false;
    noteActivity();
    p += w;
    n -= w;
  }
  return true;
}

std::string lower(std::string s) {
  for (char& ch : s) ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
  return s;
}

// Connects to the server in use (Settings::serverUrl) and sends the request head + body.
// `auth`: send the device token (every call except the Worker's /v1/device/register).
// Empty string on success, else why not.
std::string open(Conn& conn, const Settings& s, const char* method, const std::string& path, const std::string& body,
                 const char* accept, Failure& f, bool auth = true) {
  Url url;
  if (!parseUrl(s.serverUrl(), url)) {
    f = Failure::NoApiKey;
    return s.worker() ? "AI server address not set: type api worker https://... over USB."
                      : "Proxy address not set: type proxy over USB.";
  }
  if (auth && s.serverToken().empty()) {
    f = s.worker() ? Failure::Account : Failure::NoApiKey;
    return s.worker() ? "This calculator isn't registered yet. Try again in a moment."
                      : "Device token not set: type token over USB.";
  }
  bool connected;
  if (url.tls) {
    conn.c = &conn.tls;
    conn.tls.setCACert(kRootCAs);
    conn.tls.setHandshakeTimeout(kConnectMs / 1000);
    connected = conn.tls.connect(url.host.c_str(), url.port, kConnectMs);
  } else {
    conn.c = &conn.plain;
    connected = conn.plain.connect(url.host.c_str(), url.port, kConnectMs);
  }
  if (!connected) {
    char err[96] = "";
    const int code = url.tls ? conn.tls.lastError(err, sizeof(err)) : 0;
    // mbedTLS errors mean we reached a server but couldn't trust it.
    f = code != 0 ? Failure::ApiError : Failure::NoConnection;
    return code != 0 ? std::string("Secure connection failed: ") + err : "Can't reach the AI server.";
  }
  noteActivity();
  const bool defaultPort = url.port == (url.tls ? 443 : 80);
  std::string head = std::string(method) + " " + url.base + path + " HTTP/1.1\r\n" + "Host: " + url.host +
                     (defaultPort ? "" : ":" + std::to_string(url.port)) + "\r\n";
  if (auth) head += "authorization: Bearer " + s.serverToken() + "\r\n";
  head += "x-device-id: " + deviceId() + "\r\n" +
          "x-firmware: " + kFirmwareVersion + "\r\n" +
          "accept: " + accept + "\r\n" +
          "content-type: application/json\r\n" +
          "content-length: " + std::to_string(body.size()) + "\r\n" +
          "Connection: close\r\n\r\n";
  if (!writeAll(*conn.c, head.data(), head.size()) || !writeAll(*conn.c, body.data(), body.size())) {
    conn.stop();
    f = Failure::NoConnection;
    return "The connection dropped while sending.";
  }
  return "";
}

// Status line + headers. False if the connection dropped.
bool readHead(WiFiClient& c, int& status, bool& chunked, const std::function<bool()>& cancelled,
              size_t* contentLength = nullptr) {
  std::string line;
  status = 0;
  chunked = false;
  if (contentLength) *contentLength = 0;
  if (!readLine(c, line, cancelled)) return false;
  if (line.rfind("HTTP/", 0) == 0 && line.size() > 12) status = atoi(line.c_str() + 9);
  while (true) {
    if (!readLine(c, line, cancelled)) return false;
    if (line.empty()) return true;
    const std::string l = lower(line);
    if (l.rfind("transfer-encoding:", 0) == 0 && l.find("chunked") != std::string::npos) chunked = true;
    if (contentLength && l.rfind("content-length:", 0) == 0) *contentLength = strtoul(l.c_str() + 15, nullptr, 10);
  }
}

// Body bytes to `deliver` until the end, de-chunking if needed. False if it dropped mid-way.
bool readBody(WiFiClient& c, bool chunked, const std::function<bool()>& cancelled,
              const std::function<bool(const char*, size_t)>& deliver, bool& timedOut) {
  char buf[2048];
  size_t chunkLeft = 0;
  std::string line;
  uint32_t last = millis();
  timedOut = false;
  while (true) {
    if (cancelled && cancelled()) return true;
    if (chunked && chunkLeft == 0) {
      if (!readLine(c, line, cancelled)) return false;  // chunk size line (after the previous chunk's CRLF)
      if (line.empty()) continue;
      chunkLeft = strtoul(line.c_str(), nullptr, 16);
      if (chunkLeft == 0) return true;  // last chunk
    }
    const int avail = c.available();
    if (avail <= 0) {
      if (!c.connected()) return !chunked;  // Connection: close ends a plain body
      if (millis() - last > kIdleMs) {
        timedOut = true;
        return false;
      }
      delay(10);
      continue;
    }
    size_t want = sizeof(buf);
    if (chunked && want > chunkLeft) want = chunkLeft;
    const int got = c.read(reinterpret_cast<uint8_t*>(buf), want);
    if (got <= 0) continue;
    last = millis();
    noteActivity();
    if (chunked) {
      chunkLeft -= got;
      if (chunkLeft == 0) readLine(c, line, cancelled);  // the CRLF after each chunk
    }
    if (!deliver(buf, size_t(got))) return true;  // the reader has all it needs
  }
}
}  // namespace

uint32_t claudeLastActivityMs() { return g_lastActivityMs.load(std::memory_order_relaxed); }

void claudeSolve(const Settings& s, const std::string& jpeg, const std::string& closeUp, const char* effort,
                 bool tutor, const SolveCallbacks& cb) {
  if (WiFi.status() != WL_CONNECTED) return cb.fail(Failure::NoConnection, "");

  std::string body;
  if (s.worker()) {
    // The Worker's SolveRequest: only the images and tutor (it owns prompt, model and effort).
    body = buildWorkerSolveRequest(jpeg, closeUp, tutor);
  } else {
    // The same Messages API request the simulators send; the proxy checks it, picks the
    // model and adds the API key. "tutor" is read (and removed) by the proxy.
    body = buildSolveRequest(jpeg, closeUp, effort, CALC_SEND_SYSTEM_PROMPT != 0);
    if (tutor && !body.empty() && body.back() == '}') body.insert(body.size() - 1, ",\"tutor\":true");
  }
  Conn conn;
  Failure f = Failure::NoConnection;
  const std::string why = open(conn, s, "POST", "/v1/solve", body, "text/event-stream", f);
  std::string().swap(body);
  if (!why.empty()) return cb.fail(f, why);
  WiFiClient& c = *conn.c;

  int status;
  bool chunked;
  if (!readHead(c, status, chunked, cb.cancelled)) {
    conn.stop();
    if (cb.cancelled()) return;
    return cb.fail(Failure::Timeout, "");
  }

  // Body: Claude's server-sent events (passed through by the proxy / Worker), or a JSON error.
  StreamReader stream;
  std::string errorBody, shownAnswer;
  bool timedOut = false;
  const bool complete = readBody(c, chunked, cb.cancelled, [&](const char* p, size_t n) {
    if (status != 200) {
      if (errorBody.size() < 16384) errorBody.append(p, n);
      return true;
    }
    stream.feed(std::string(p, n));
    double conf;
    std::string answer;
    if (peekAnswer(stream.text(), conf, answer) && answer != shownAnswer) {
      shownAnswer = answer;
      cb.partial(conf, answer);
    }
    return !stream.finished();
  }, timedOut);
  conn.stop();
  if (cb.cancelled()) return;
  if (timedOut) return cb.fail(Failure::Timeout, "");

  if (!complete && status == 200 && !stream.finished()) return cb.fail(Failure::NoConnection, "The reply was cut off.");
  if (s.worker()) {
    WorkerFailure wf;
    if (classifyWorkerFailure(status, errorBody, stream, wf)) {
      if (wf.action != WorkerAction::None && cb.worker) cb.worker(wf);
      return cb.fail(wf.failure, wf.detail);
    }
    return cb.reply(stream.text());
  }
  std::string detail;
  if (status == 401 && errorBody.find("device_unknown") == std::string::npos)
    return cb.fail(Failure::Account, "The AI server didn't accept this calculator's token.");
  if (classifyFailure(status, errorBody, stream, f, detail)) return cb.fail(f, detail);
  cb.reply(stream.text());
}

int proxyRequest(const Settings& s, const char* method, const std::string& path, const std::string& body,
                 std::string& reply, bool auth) {
  reply.clear();
  if (WiFi.status() != WL_CONNECTED) {
    reply = "Not connected to the hotspot.";
    return 0;
  }
  Conn conn;
  Failure f;
  const std::string why = open(conn, s, method, path, body, "application/json", f, auth);
  if (!why.empty()) {
    reply = why;
    return 0;
  }
  int status;
  bool chunked, timedOut;
  if (!readHead(*conn.c, status, chunked, nullptr)) {
    conn.stop();
    reply = "No reply from the AI server.";
    return 0;
  }
  readBody(*conn.c, chunked, nullptr, [&](const char* p, size_t n) {
    if (reply.size() < 8192) reply.append(p, n);
    return true;
  }, timedOut);
  conn.stop();
  return status;
}

int proxyDownload(const Settings& s, const std::string& path, const std::function<bool(const char*, size_t)>& sink,
                  size_t& contentLength, std::string& error) {
  contentLength = 0;
  error.clear();
  if (WiFi.status() != WL_CONNECTED) {
    error = "Not connected to the hotspot.";
    return 0;
  }
  Conn conn;
  Failure f;
  error = open(conn, s, "GET", path, "", "application/octet-stream", f);
  if (!error.empty()) return 0;
  int status;
  bool chunked, timedOut;
  if (!readHead(*conn.c, status, chunked, nullptr, &contentLength)) {
    conn.stop();
    error = "No reply from the AI server.";
    return 0;
  }
  const bool complete = readBody(*conn.c, chunked, nullptr, sink, timedOut);
  conn.stop();
  if (status == 200 && !complete) {
    error = timedOut ? "The download stalled." : "The download was cut off.";
    return 0;
  }
  return status;
}
