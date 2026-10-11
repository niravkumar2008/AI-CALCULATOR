#include "setup_portal.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <esp_http_server.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

#include "log.h"

namespace {
constexpr uint32_t kLimitMs = 10 * 60 * 1000;
constexpr int kApChannel = 1;
constexpr int kMaxClients = 2;
constexpr int kMaxNetworks = 12;
constexpr size_t kMaxFormBytes = 1024;  // ssid 32 + password 63 + url 200 + token 100, URL-encoded

httpd_handle_t g_server = nullptr;
DNSServer g_dns;
bool g_dnsOn = false;
uint32_t g_startedAt = 0;
std::string g_apName, g_apPass;
std::vector<std::string> g_networks;  // seen in the scan, strongest first
Settings g_current;                   // what the form starts from (never shows the token)
std::mutex g_lock;
Settings g_saved;
bool g_haveSaved = false;
uint32_t g_savedAt = 0;

// ---- HTML
const char kHead[] = R"HTML(<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>AI Calculator setup</title><style>
:root{color-scheme:light dark;--bg:#f6f6f4;--fg:#1c1c1a;--mute:#666;--line:#ccc;--acc:#1f5fbf}
@media(prefers-color-scheme:dark){:root{--bg:#161615;--fg:#eee;--mute:#aaa;--line:#444;--acc:#7aa7ff}}
body{margin:0;background:var(--bg);color:var(--fg);font:17px/1.45 system-ui,sans-serif}
main{max-width:480px;margin:auto;padding:16px 16px 48px}h1{font-size:1.3rem;margin:.2rem 0 .8rem}
label{display:block;margin:.9rem 0 .25rem;font-weight:600}input,select{width:100%;box-sizing:border-box;font:inherit;padding:.6rem;border:1px solid var(--line);border-radius:8px;background:transparent;color:inherit;min-height:44px}
button{font:inherit;padding:.7rem 1rem;border-radius:8px;border:0;background:var(--acc);color:#fff;min-height:48px;width:100%;margin-top:1.2rem}
p.help{color:var(--mute);font-size:.92rem;margin:.2rem 0 0}.ok{color:#1a7f37}.bad{color:#b3261e}code{font-family:ui-monospace,monospace}
</style></head><body><main>)HTML";
const char kTail[] = "</main></body></html>";

std::string escape(const std::string& s) {
  std::string o;
  for (char c : s) {
    switch (c) {
      case '&': o += "&amp;"; break;
      case '<': o += "&lt;"; break;
      case '>': o += "&gt;"; break;
      case '"': o += "&quot;"; break;
      default: o += c;
    }
  }
  return o;
}

std::string formPage(const std::string& notice, bool bad) {
  std::string p = kHead;
  p += "<h1>AI Calculator setup</h1>";
  if (!notice.empty()) p += "<p class=\"" + std::string(bad ? "bad" : "ok") + "\">" + escape(notice) + "</p>";
  p += "<p class=\"help\">Calculator <code>" + escape(deviceId()) + "</code>. Fields left empty keep their current value.</p>";
  p += "<form method=\"post\" action=\"/\" autocomplete=\"off\">";
  // Hotspot: a list of what the calculator saw, plus a free text box for a hidden one.
  p += "<label for=\"ssid\">Phone hotspot (2.4 GHz)</label><select id=\"ssidPick\" onchange=\"document.getElementById('ssid').value=this.value\">";
  p += "<option value=\"\">" + std::string(g_networks.empty() ? "No networks seen: type the name below" : "Pick one, or type it below") + "</option>";
  for (const auto& n : g_networks) p += "<option value=\"" + escape(n) + "\">" + escape(n) + "</option>";
  p += "</select><input id=\"ssid\" name=\"ssid\" maxlength=\"32\" placeholder=\"" +
       escape(g_current.ssid.empty() ? "exactly as the phone shows it" : "now: " + g_current.ssid) + "\" style=\"margin-top:.4rem\">";
  p += "<p class=\"help\">On iPhone turn on Maximize Compatibility; the calculator only sees 2.4 GHz.</p>";
  p += "<label for=\"pass\">Hotspot password</label><input id=\"pass\" name=\"pass\" type=\"password\" maxlength=\"63\" placeholder=\"8-63 characters\">";
  p += "<label for=\"proxy\">AI server address</label><input id=\"proxy\" name=\"proxy\" inputmode=\"url\" maxlength=\"200\" value=\"" +
       escape(g_current.proxyUrl) + "\" placeholder=\"https://...\">";
  p += "<label for=\"token\">Device token</label><input id=\"token\" name=\"token\" type=\"password\" maxlength=\"100\" placeholder=\"" +
       escape(g_current.deviceToken.empty() ? "dt_... from the AI server" : "now: " + g_current.tokenHint()) + "\">";
  p += "<p class=\"help\">The token identifies this calculator to the AI server. It is never a Claude API key.</p>";
  p += "<button type=\"submit\">Save to the calculator</button></form>";
  p += "<p class=\"help\" style=\"margin-top:1.4rem\">After saving, the calculator leaves this network: rejoin your usual Wi-Fi. "
       "To link the calculator to your account, open AI SOLVE (MODE 4) and press =: it shows the code for the link page.</p>";
  p += kTail;
  return p;
}

std::string savedPage(const Settings& s) {
  std::string p = kHead;
  p += "<h1>Saved</h1><p class=\"ok\">The calculator keeps these settings.</p><ul>";
  p += "<li>Hotspot: <b>" + escape(s.ssid.empty() ? "not set" : s.ssid) + "</b></li>";
  p += "<li>AI server: <b>" + escape(s.proxyUrl.empty() ? "not set" : s.proxyUrl) + "</b></li>";
  p += "<li>Device token: <b>" + escape(s.tokenHint()) + "</b></li></ul>";
  p += "<p>This network closes in a few seconds. Rejoin your usual Wi-Fi, turn the phone's hotspot on, "
       "then MODE 4 on the calculator and = to solve (the first = shows a code to link your account).</p>";
  p += kTail;
  return p;
}

// ---- form decoding
int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

std::string urlDecode(const std::string& in) {
  std::string o;
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '+') {
      o += ' ';
    } else if (in[i] == '%' && i + 2 < in.size() && hexVal(in[i + 1]) >= 0 && hexVal(in[i + 2]) >= 0) {
      o += char(hexVal(in[i + 1]) * 16 + hexVal(in[i + 2]));
      i += 2;
    } else {
      o += in[i];
    }
  }
  return o;
}

std::string field(const std::string& body, const char* name) {
  size_t p = 0;
  while (p < body.size()) {
    size_t amp = body.find('&', p);
    if (amp == std::string::npos) amp = body.size();
    const std::string pair = body.substr(p, amp - p);
    const size_t eq = pair.find('=');
    if (eq != std::string::npos && pair.substr(0, eq) == name) return urlDecode(pair.substr(eq + 1));
    p = amp + 1;
  }
  return "";
}

// ---- handlers
void noCache(httpd_req_t* req) { httpd_resp_set_hdr(req, "Cache-Control", "no-store"); }

esp_err_t sendHtml(httpd_req_t* req, const std::string& page) {
  noCache(req);
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, page.data(), page.size());
}

esp_err_t formHandler(httpd_req_t* req) { return sendHtml(req, formPage("", false)); }

esp_err_t saveHandler(httpd_req_t* req) {
  if (req->content_len > kMaxFormBytes) return sendHtml(req, formPage("That was too long to be a setup form.", true));
  std::string body(req->content_len, '\0');
  size_t got = 0;
  while (got < body.size()) {
    const int n = httpd_req_recv(req, &body[got], body.size() - got);
    if (n <= 0) return sendHtml(req, formPage("The form didn't arrive in one piece: try again.", true));
    got += size_t(n);
  }
  Settings s = g_current;
  std::string ssid = field(body, "ssid"), pass = field(body, "pass"), proxy = field(body, "proxy"), token = field(body, "token");
  std::string why;
  if (!ssid.empty() || !pass.empty()) {
    if (ssid.empty()) ssid = s.ssid;  // new password for the saved hotspot
    why = settingsCheckWifi(ssid, pass);
    if (!why.empty()) return sendHtml(req, formPage(why, true));
    s.ssid = ssid;
    s.password = pass;
  }
  if (!proxy.empty()) {
    why = settingsCheckProxy(proxy);
    if (!why.empty()) return sendHtml(req, formPage(why, true));
    s.proxyUrl = proxy;
  }
  if (!token.empty()) {
    why = settingsCheckToken(token);
    if (!why.empty()) return sendHtml(req, formPage(why, true));
    s.deviceToken = token;
  }
  settingsSave(s);
  {
    std::lock_guard<std::mutex> lock(g_lock);
    g_saved = s;
    g_current = s;
    g_haveSaved = true;
    g_savedAt = millis() | 1;
  }
  LOGF("setup", "saved from the phone: hotspot \"%s\", proxy %s, token %s", s.ssid.c_str(),
       s.proxyUrl.empty() ? "not set" : s.proxyUrl.c_str(), s.tokenHint().c_str());
  return sendHtml(req, savedPage(s));
}

// Every other address (the phone's "is there internet?" probes included) goes to the form:
// that is what makes the phone pop the sign-in sheet up.
esp_err_t redirectHandler(httpd_req_t* req) {
  noCache(req);
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
  return httpd_resp_send(req, nullptr, 0);
}

httpd_uri_t route(const char* path, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*)) {
  httpd_uri_t u = {};
  u.uri = path;
  u.method = method;
  u.handler = handler;
  return u;
}

void stopAll() {
  if (g_dnsOn) {
    g_dns.stop();
    g_dnsOn = false;
  }
  if (g_server) {
    httpd_stop(g_server);
    g_server = nullptr;
  }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  g_apName.clear();
  g_apPass.clear();
}

void scanNetworks() {
  g_networks.clear();
  WiFi.mode(WIFI_STA);
  const int n = WiFi.scanNetworks();
  // Strongest first, each name once, 2.4 GHz only (the chip sees nothing else anyway).
  std::vector<std::pair<int, std::string>> seen;
  for (int i = 0; i < n; ++i) {
    const std::string name = WiFi.SSID(i).c_str();
    if (name.empty()) continue;
    bool dup = false;
    for (auto& s : seen) dup |= s.second == name;
    if (!dup) seen.push_back({WiFi.RSSI(i), name});
  }
  WiFi.scanDelete();
  std::sort(seen.begin(), seen.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
  for (size_t i = 0; i < seen.size() && int(i) < kMaxNetworks; ++i) g_networks.push_back(seen[i].second);
}
}  // namespace

bool setupStart(const Settings& current, SetupInfo& info, std::string& error) {
  if (g_server) {
    info = {g_apName, g_apPass, "http://192.168.4.1/"};
    return true;
  }
  {
    std::lock_guard<std::mutex> lock(g_lock);
    g_current = current;
    g_haveSaved = false;
    g_savedAt = 0;
  }
  scanNetworks();
  // Short password a person can type from the e-paper: 8 characters, no 0/O, 1/l/I.
  static const char kAlpha[] = "abcdefghjkmnpqrstuvwxyz23456789";
  g_apPass.clear();
  for (int i = 0; i < 8; ++i) g_apPass += kAlpha[esp_random() % (sizeof kAlpha - 1)];
  char name[16];
  snprintf(name, sizeof name, "AI-Calc-%04X", unsigned(esp_random() & 0xFFFF));
  g_apName = name;
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(g_apName.c_str(), g_apPass.c_str(), kApChannel, 0, kMaxClients)) {
    error = "Couldn't start the setup network.";
    stopAll();
    return false;
  }
  httpd_config_t c = HTTPD_DEFAULT_CONFIG();
  c.stack_size = 8192;
  c.max_uri_handlers = 4;
  c.lru_purge_enable = true;
  c.uri_match_fn = httpd_uri_match_wildcard;
  c.core_id = 0;
  if (httpd_start(&g_server, &c) != ESP_OK) {
    g_server = nullptr;
    error = "Couldn't start the setup page.";
    stopAll();
    return false;
  }
  const httpd_uri_t uris[] = {
      route("/", HTTP_GET, formHandler),
      route("/", HTTP_POST, saveHandler),
      route("/*", HTTP_GET, redirectHandler),
      route("/*", HTTP_POST, redirectHandler),
  };
  for (const auto& u : uris) httpd_register_uri_handler(g_server, &u);
  g_dnsOn = g_dns.start(53, "*", WiFi.softAPIP());
  g_startedAt = millis();
  info = {g_apName, g_apPass, "http://192.168.4.1/"};
  LOGF("setup", "network %s, password %s, page %s (%d networks seen)", g_apName.c_str(), g_apPass.c_str(), info.url.c_str(),
       int(g_networks.size()));
  return true;
}

void setupStop() {
  if (!g_server) return;
  stopAll();
  LOGF("setup", "setup mode ended");
}

bool setupActive() { return g_server != nullptr; }

void setupService() {
  if (!g_server) return;
  if (g_dnsOn) g_dns.processNextRequest();
  if (millis() - g_startedAt > kLimitMs) {
    LOGF("setup", "10 minutes are up");
    setupStop();
  }
}

uint32_t setupSecondsLeft() {
  if (!g_server) return 0;
  const uint32_t used = millis() - g_startedAt;
  return used >= kLimitMs ? 0 : (kLimitMs - used) / 1000;
}

bool setupTakeSaved(Settings& out) {
  std::lock_guard<std::mutex> lock(g_lock);
  if (!g_haveSaved) return false;
  g_haveSaved = false;
  out = g_saved;
  return true;
}

uint32_t setupSavedAgoMs() {
  std::lock_guard<std::mutex> lock(g_lock);
  return g_savedAt ? millis() - g_savedAt : 0;
}
