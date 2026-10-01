#include "claude_client.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "claude_api.h"
#include "root_ca.h"

using namespace calc;

namespace {
constexpr uint32_t kConnectMs = 15000;
constexpr uint32_t kIdleMs = 120000;  // no bytes for 2 minutes = timed out (as in the simulator)

// Reads one header line (without CRLF). False on timeout or dropped connection.
bool readLine(WiFiClientSecure& c, std::string& line, const SolveCallbacks& cb) {
  line.clear();
  uint32_t last = millis();
  while (true) {
    if (cb.cancelled()) return false;
    int ch = c.read();
    if (ch < 0) {
      if (!c.connected() || millis() - last > kIdleMs) return false;
      delay(5);
      continue;
    }
    last = millis();
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
    p += w;
    n -= w;
  }
  return true;
}

std::string lower(std::string s) {
  for (char& ch : s) ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
  return s;
}
}  // namespace

void claudeSolve(const std::string& apiKey, const std::string& jpeg, const SolveCallbacks& cb) {
  if (WiFi.status() != WL_CONNECTED) return cb.fail(Failure::NoConnection, "");

  const std::string body = buildSolveRequest(jpeg);
  WiFiClientSecure c;
  c.setCACert(kRootCAs);
  c.setHandshakeTimeout(kConnectMs / 1000);
  if (!c.connect(kApiHost, 443, kConnectMs)) {
    char err[96] = "";
    int code = c.lastError(err, sizeof(err));
    // mbedTLS errors mean we reached a server but couldn't trust it.
    if (code != 0) return cb.fail(Failure::ApiError, std::string("Secure connection failed: ") + err);
    return cb.fail(Failure::NoConnection, "Can't reach Claude.");
  }

  std::string head = std::string("POST ") + kApiPath + " HTTP/1.1\r\n" +
                     "Host: " + kApiHost + "\r\n" +
                     "content-type: application/json\r\n" +
                     "anthropic-version: " + kApiVersion + "\r\n" +
                     "x-api-key: " + apiKey + "\r\n" +
                     "content-length: " + std::to_string(body.size()) + "\r\n" +
                     "Connection: close\r\n\r\n";
  if (!writeAll(c, head.data(), head.size()) || !writeAll(c, body.data(), body.size())) {
    c.stop();
    return cb.fail(Failure::NoConnection, "The connection dropped while sending the photo.");
  }

  // Status line and headers
  std::string line;
  if (!readLine(c, line, cb)) {
    c.stop();
    if (cb.cancelled()) return;
    return cb.fail(Failure::Timeout, "");
  }
  int status = 0;
  if (line.rfind("HTTP/", 0) == 0 && line.size() > 12) status = atoi(line.c_str() + 9);
  bool chunked = false;
  while (true) {
    if (!readLine(c, line, cb)) {
      c.stop();
      if (cb.cancelled()) return;
      return cb.fail(Failure::NoConnection, "The reply was cut off.");
    }
    if (line.empty()) break;
    std::string l = lower(line);
    if (l.rfind("transfer-encoding:", 0) == 0 && l.find("chunked") != std::string::npos) chunked = true;
  }

  // Body: server-sent events, possibly in HTTP chunks
  StreamReader stream;
  std::string errorBody, shownAnswer;
  auto deliver = [&](const char* p, size_t n) {
    if (status != 200) {
      if (errorBody.size() < 16384) errorBody.append(p, n);
      return;
    }
    stream.feed(std::string(p, n));
    double conf;
    std::string answer;
    if (peekAnswer(stream.text(), conf, answer) && answer != shownAnswer) {
      shownAnswer = answer;
      cb.partial(conf, answer);
    }
  };

  char buf[2048];
  size_t chunkLeft = 0;
  bool dropped = false;
  uint32_t last = millis();
  while (!stream.finished()) {
    if (cb.cancelled()) {
      c.stop();
      return;
    }
    if (chunked && chunkLeft == 0) {
      if (!readLine(c, line, cb)) {  // chunk size line (after the previous chunk's CRLF)
        dropped = !cb.cancelled();
        break;
      }
      if (line.empty()) continue;
      chunkLeft = strtoul(line.c_str(), nullptr, 16);
      if (chunkLeft == 0) break;  // last chunk
    }
    int avail = c.available();
    if (avail <= 0) {
      if (!c.connected()) break;
      if (millis() - last > kIdleMs) {
        c.stop();
        return cb.fail(Failure::Timeout, "");
      }
      delay(10);
      continue;
    }
    size_t want = sizeof(buf);
    if (chunked && want > chunkLeft) want = chunkLeft;
    int got = c.read(reinterpret_cast<uint8_t*>(buf), want);
    if (got <= 0) continue;
    last = millis();
    if (chunked) {
      chunkLeft -= got;
      if (chunkLeft == 0) readLine(c, line, cb);  // the CRLF after each chunk
    }
    deliver(buf, got);
  }
  c.stop();
  if (cb.cancelled()) return;

  Failure f;
  std::string detail;
  if (dropped && status == 200 && !stream.finished()) return cb.fail(Failure::NoConnection, "The reply was cut off.");
  if (classifyFailure(status, errorBody, stream, f, detail)) return cb.fail(f, detail);
  cb.reply(stream.text());
}
