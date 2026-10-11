#include "claude_client.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <atomic>

#include "claude_api.h"
#include "root_ca.h"

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

struct Url {
  std::string host, base;  // base: path prefix without trailing slash ("" or "/calc")
  uint16_t port = 443;
};

bool parseUrl(const std::string& u, Url& out) {
  if (u.rfind("https://", 0) != 0) return false;
  std::string rest = u.substr(8);
  const size_t slash = rest.find('/');
  std::string hostPort = rest.substr(0, slash);
  out.base = slash == std::string::npos ? "" : rest.substr(slash);
  while (!out.base.empty() && out.base.back() == '/') out.base.pop_back();
  const size_t colon = hostPort.find(':');
  out.host = hostPort.substr(0, colon);
  if (colon != std::string::npos) out.port = uint16_t(atoi(hostPort.c_str() + colon + 1));
  return !out.host.empty() && out.port != 0;
}

// Reads one header line (without CRLF). False on timeout or dropped connection.
bool readLine(WiFiClientSecure& c, std::string& line, const std::function<bool()>& cancelled) {
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

bool writeAll(WiFiClientSecure& c, const char* p, size_t n) {
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

// Connects and sends the request head + body. Empty string on success, else why not.
std::string open(WiFiClientSecure& c, const Settings& s, const char* method, const std::string& path,
                 const std::string& body, const char* accept, Failure& f) {
  Url url;
  if (!parseUrl(s.proxyUrl, url)) {
    f = Failure::NoApiKey;
    return "Proxy address not set: type proxy over USB.";
  }
  if (s.deviceToken.empty()) {
    f = Failure::NoApiKey;
    return "Device token not set: type token over USB.";
  }
  c.setCACert(kRootCAs);
  c.setHandshakeTimeout(kConnectMs / 1000);
  if (!c.connect(url.host.c_str(), url.port, kConnectMs)) {
    char err[96] = "";
    const int code = c.lastError(err, sizeof(err));
    // mbedTLS errors mean we reached a server but couldn't trust it.
    f = code != 0 ? Failure::ApiError : Failure::NoConnection;
    return code != 0 ? std::string("Secure connection failed: ") + err : "Can't reach the AI server.";
  }
  noteActivity();
  const std::string head = std::string(method) + " " + url.base + path + " HTTP/1.1\r\n" +
                           "Host: " + url.host + "\r\n" +
                           "authorization: Bearer " + s.deviceToken + "\r\n" +
                           "x-device-id: " + deviceId() + "\r\n" +
                           "x-firmware: " + kFirmwareVersion + "\r\n" +
                           "accept: " + accept + "\r\n" +
                           "content-type: application/json\r\n" +
                           "content-length: " + std::to_string(body.size()) + "\r\n" +
                           "Connection: close\r\n\r\n";
  if (!writeAll(c, head.data(), head.size()) || !writeAll(c, body.data(), body.size())) {
    c.stop();
    f = Failure::NoConnection;
    return "The connection dropped while sending.";
  }
  return "";
}

// Status line + headers. False if the connection dropped.
bool readHead(WiFiClientSecure& c, int& status, bool& chunked, const std::function<bool()>& cancelled,
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
bool readBody(WiFiClientSecure& c, bool chunked, const std::function<bool()>& cancelled,
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

  // The same Messages API request the simulators send; the proxy checks it, picks the
  // model and adds the API key. "tutor" is read (and removed) by the proxy.
  std::string body = buildSolveRequest(jpeg, closeUp, effort, CALC_SEND_SYSTEM_PROMPT != 0);
  if (tutor && !body.empty() && body.back() == '}') body.insert(body.size() - 1, ",\"tutor\":true");
  WiFiClientSecure c;
  Failure f = Failure::NoConnection;
  const std::string why = open(c, s, "POST", "/v1/solve", body, "text/event-stream", f);
  std::string().swap(body);
  if (!why.empty()) return cb.fail(f, why);

  int status;
  bool chunked;
  if (!readHead(c, status, chunked, cb.cancelled)) {
    c.stop();
    if (cb.cancelled()) return;
    return cb.fail(Failure::Timeout, "");
  }

  // Body: Claude's server-sent events (passed through by the proxy), or a JSON error.
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
  c.stop();
  if (cb.cancelled()) return;
  if (timedOut) return cb.fail(Failure::Timeout, "");

  std::string detail;
  if (!complete && status == 200 && !stream.finished()) return cb.fail(Failure::NoConnection, "The reply was cut off.");
  if (status == 401 && errorBody.find("device_unknown") == std::string::npos)
    return cb.fail(Failure::Account, "The AI server didn't accept this calculator's token.");
  if (classifyFailure(status, errorBody, stream, f, detail)) return cb.fail(f, detail);
  cb.reply(stream.text());
}

int proxyRequest(const Settings& s, const char* method, const std::string& path, const std::string& body,
                 std::string& reply) {
  reply.clear();
  if (WiFi.status() != WL_CONNECTED) {
    reply = "Not connected to the hotspot.";
    return 0;
  }
  WiFiClientSecure c;
  Failure f;
  const std::string why = open(c, s, method, path, body, "application/json", f);
  if (!why.empty()) {
    reply = why;
    return 0;
  }
  int status;
  bool chunked, timedOut;
  if (!readHead(c, status, chunked, nullptr)) {
    c.stop();
    reply = "No reply from the AI server.";
    return 0;
  }
  readBody(c, chunked, nullptr, [&](const char* p, size_t n) {
    if (reply.size() < 8192) reply.append(p, n);
    return true;
  }, timedOut);
  c.stop();
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
  WiFiClientSecure c;
  Failure f;
  error = open(c, s, "GET", path, "", "application/octet-stream", f);
  if (!error.empty()) return 0;
  int status;
  bool chunked, timedOut;
  if (!readHead(c, status, chunked, nullptr, &contentLength)) {
    c.stop();
    error = "No reply from the AI server.";
    return 0;
  }
  const bool complete = readBody(c, chunked, nullptr, sink, timedOut);
  c.stop();
  if (status == 200 && !complete) {
    error = timedOut ? "The download stalled." : "The download was cut off.";
    return 0;
  }
  return status;
}
