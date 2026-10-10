#include "preview.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_http_server.h>

#include <cstring>
#include <mutex>

#include "camera.h"
#include "json.h"
#include "log.h"

namespace {
httpd_handle_t g_server = nullptr;  // page, settings, single photos (port 80)
httpd_handle_t g_stream = nullptr;  // live view only (port 81): a stream occupies its server
char g_token[17] = {};  // 16 hex chars, new every start
uint32_t g_startedAt = 0;
constexpr uint32_t kLimitMs = 15 * 60 * 1000;
std::string g_apName, g_apPass;
volatile float g_liveFocus = 0;  // sharpness of the writing in the latest live frame

const char kPage[] = R"HTML(<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Camera preview</title><style>
:root{color-scheme:light dark;--bg:#f6f6f4;--fg:#1c1c1a;--mute:#666;--line:#ccc;--acc:#1f5fbf}
@media(prefers-color-scheme:dark){:root{--bg:#161615;--fg:#eee;--mute:#aaa;--line:#444;--acc:#7aa7ff}}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.4 system-ui,sans-serif}
main{max-width:720px;margin:auto;padding:12px 16px 40px}
img{width:100%;height:auto;background:#000;border-radius:6px;display:block}
h1{font-size:1.2rem;margin:.2rem 0 .6rem}
.row{display:grid;grid-template-columns:8.5rem 1fr 2.5rem;gap:.6rem;align-items:center;padding:.35rem 0;border-bottom:1px solid var(--line)}
input[type=range]{width:100%;min-height:32px}
output{text-align:right;font-variant-numeric:tabular-nums}
.btns{display:flex;flex-wrap:wrap;gap:.5rem;margin:.8rem 0}
button,select{font:inherit;padding:.55rem .9rem;border-radius:6px;border:1px solid var(--line);background:transparent;color:inherit;min-height:44px}
button.primary{background:var(--acc);color:#fff;border-color:var(--acc)}
#msg{min-height:1.4em;color:var(--mute)}
p.help{color:var(--mute);font-size:.9rem}
.focus{font-size:1.5rem;margin:.5rem 0 0;font-variant-numeric:tabular-nums}
.focus span{font-size:1rem;color:var(--mute)}
h2{font-size:1.05rem;margin:1rem 0 .3rem}
.ans{font-size:1.3rem;font-weight:600;white-space:pre-line;margin:.3rem 0}
ol{padding-left:1.4rem;margin:.3rem 0}
.warn{color:#b3261e}
@media(prefers-color-scheme:dark){.warn{color:#ff8a80}}
button:disabled{opacity:.5}
pre{white-space:pre-wrap;font:.85rem/1.4 ui-monospace,monospace}
</style></head><body><main>
<h1>Camera preview</h1>
<img id="view" alt="Live view from the calculator's camera">
<p class="focus"><span>Focus</span> <b id="focus">&ndash;</b> <span id="best"></span></p>
<p class="help">Move the camera closer and further: the distance with the highest number is where your writing is sharpest.</p>
<div class="btns">
 <button class="primary" id="send">Send to Claude</button>
</div>
<p id="msg" role="status" aria-live="polite"></p>
<section id="answer" hidden aria-labelledby="ansTitle">
 <h2 id="ansTitle">Claude's answer</h2>
 <p id="readAs" class="help"></p>
 <p id="ans" class="ans"></p>
 <ol id="steps"></ol>
 <p id="unclear" class="warn"></p>
</section>
<div class="btns">
 <button id="live" aria-pressed="true">Pause live view</button>
 <button id="last">Show last scan photo</button>
 <button id="detail">Show last enhanced image</button>
</div>
<div id="controls"></div>
<div class="btns">
 <button id="tune">Tune for this light now</button>
</div>
<pre id="tuneReport" class="help" hidden></pre>
<div class="btns">
 <button class="primary" id="save">Save for scans</button>
 <button id="reset">Reset to defaults</button>
</div>
<p class="help"><b>Send to Claude</b> scans what the camera sees right now. Changes apply at once. <b>Save for scans</b> keeps them after a restart.
The live view runs at a smaller size for speed; scans use the Photo size.
Blurry at every setting? The lens focus is fixed: try 20&ndash;30 cm and check the lens has no protective film.</p>
</main><script>
const t=new URLSearchParams(location.search).get('t')||'';
const q=(p,o={})=>p+'?t='+encodeURIComponent(t)+Object.entries(o).map(([k,v])=>'&'+k+'='+encodeURIComponent(v)).join('');
const msg=s=>document.getElementById('msg').textContent=s;
const view=document.getElementById('view');
const SIZES={8:'VGA 640x480',9:'SVGA 800x600',10:'XGA 1024x768',11:'HD 1280x720',12:'SXGA 1280x1024',13:'UXGA 1600x1200'};
const ROWS=[['framesize','Photo size',8,13],['quality','JPEG quality (lower = sharper)',6,30],['brightness','Brightness',-2,2],
['contrast','Contrast',-2,2],['sharpness','Sharpness',-2,2],['ae_level','Exposure',-2,2],['gainceiling','Max gain',0,6],
['denoise','Denoise',0,8],['settle','Hold-still wait (tenths of a second)',2,30],['careful','Careful scan: tune for the light first (about 5 s slower)',0,1],['grayscale','Grayscale',0,1],['hmirror','Mirror',0,1],['vflip','Flip',0,1]];
let live=true;
function startLive(){view.src=q(location.protocol+'//'+location.hostname+':81/stream',{n:Date.now()});}
async function set(name,val){
 try{const r=await fetch(q('/set',{var:name,val}));msg(r.ok?'Set '+name+' to '+val:'Not accepted: '+name);}
 catch(e){msg('Lost connection to the calculator.');}
}
function build(cur){
 const box=document.getElementById('controls');box.textContent='';
 for(const [k,label,min,max] of ROWS){
  const row=document.createElement('div');row.className='row';
  const id='c_'+k,lab=document.createElement('label');lab.htmlFor=id;lab.textContent=label;
  let input,out=document.createElement('output');out.htmlFor=id;
  if(k==='framesize'){
   input=document.createElement('select');
   for(const [v,n] of Object.entries(SIZES)){const o=document.createElement('option');o.value=v;o.textContent=n;input.append(o);}
   input.value=cur[k];out.textContent='';
  }else{
   input=document.createElement('input');input.type='range';input.min=min;input.max=max;input.step=1;input.value=cur[k];
   out.textContent=cur[k];input.oninput=()=>out.textContent=input.value;
  }
  input.id=id;input.onchange=()=>set(k,input.value);
  row.append(lab,input,out);box.append(row);
 }
}
async function load(){
 try{const r=await fetch(q('/tuning'));if(!r.ok)throw 0;build(await r.json());}
 catch(e){msg('Could not reach the calculator. Is the link from this session\'s Serial Monitor?');}
}
document.getElementById('live').onclick=e=>{
 live=!live;e.target.setAttribute('aria-pressed',live);e.target.textContent=live?'Pause live view':'Resume live view';
 if(live)startLive();else view.src=q('/frame',{n:Date.now()});
};
document.getElementById('last').onclick=()=>{
 live=false;const b=document.getElementById('live');b.setAttribute('aria-pressed','false');b.textContent='Resume live view';
 view.onerror=()=>msg('No scan photo yet. Press = on the calculator first.');view.src=q('/last.jpg',{n:Date.now()});
};
document.getElementById('detail').onclick=()=>{
 live=false;const b=document.getElementById('live');b.setAttribute('aria-pressed','false');b.textContent='Resume live view';
 view.onerror=()=>msg('No enhanced image yet: take a scan first.');
 view.src=q('/detail.jpg',{n:Date.now()});
};
let best=0;
setInterval(async()=>{
 if(!live)return;
 try{const r=await fetch(q('/focus'));if(!r.ok)return;const f=(await r.json()).focus;
  document.getElementById('focus').textContent=f||'\u2013';
  if(f>best){best=f;document.getElementById('best').textContent='(best '+best+')';}}catch(e){}
},1000);
// ---- Send to Claude ----
let lastN=0, waiting=false, waitStart=0;
async function getResult(){
 const c=new AbortController();const tm=setTimeout(()=>c.abort(),3000);
 try{const r=await fetch(q('/result',{n:Date.now()}),{signal:c.signal});return r.ok?await r.json():null;}
 catch(e){return null;}finally{clearTimeout(tm);}
}
function show(res){
 const box=document.getElementById('answer');box.hidden=false;
 const set=(id,t)=>document.getElementById(id).textContent=t||'';
 const steps=document.getElementById('steps');steps.textContent='';
 if(res.error){set('readAs','');set('ans','Not solved: '+res.error);set('unclear','');return;}
 const r=res.reply||{};
 if(r.readable===false){set('readAs','');set('ans','No problem found in the photo.');set('unclear','');return;}
 set('readAs',r.read_as?'Read as: '+r.read_as:'');
 set('ans',r.answer);
 for(const s of (r.steps||[])){const li=document.createElement('li');li.textContent=s;steps.append(li);}
 const u=(r.unclear||[]).join('; ');
 set('unclear',(typeof r.confidence==='number'&&r.confidence<0.8?'Unclear scan ('+Math.round(r.confidence*100)+'% sure). ':'')+(u?'Check: '+u:''));
 box.scrollIntoView({behavior:'smooth',block:'nearest'});
}
async function poll(){
 if(!waiting)return;
 const res=await getResult();
 if(res&&res.n>lastN){
  lastN=res.n;waiting=false;document.getElementById('send').disabled=false;
  msg(res.error?'The scan didn\'t work.':'Answer received.');show(res);
  if(live)startLive();load();return;
 }
 const s=Math.round((Date.now()-waitStart)/1000);
 msg(s<90?'Solving... the calculator stepped off this network to reach Claude ('+s+' s). The answer also shows on the calculator.'
  :'Still not back ('+s+' s). If your laptop switched to another Wi-Fi, rejoin the AI-Calc network; this page keeps checking.');
 setTimeout(poll,2000);
}
document.getElementById('send').onclick=async()=>{
 const b=document.getElementById('send');b.disabled=true;
 const cur=await getResult();if(cur)lastN=cur.n;
 let holdMs=2000;
 try{const r=await fetch(q('/send'));if(!r.ok)throw 0;holdMs=parseInt(await r.text(),10)||holdMs;}
 catch(e){b.disabled=false;msg('Could not reach the calculator.');return;}
 view.removeAttribute('src');  // free the camera and the connection
 const until=Date.now()+holdMs;
 await new Promise(done=>{
  const tickDown=()=>{const left=Math.ceil((until-Date.now())/1000);
   if(left<=0)return done();msg('Hold still: '+left+' s');setTimeout(tickDown,250);};
  tickDown();
 });
 msg('Got the photo. Sending to Claude...');
 waiting=true;waitStart=Date.now();setTimeout(poll,3000);
};
getResult().then(r=>{if(r)lastN=r.n;});
document.getElementById('tune').onclick=async()=>{
 const b=document.getElementById('tune');b.disabled=true;msg('Tuning: keep the page under the camera (about 5 s)...');
 try{const r=await fetch(q('/autotune'));const t=await r.text();
  const pre=document.getElementById('tuneReport');pre.hidden=false;pre.textContent=t;
  msg(r.ok?'Tuned. Save for scans to keep these settings.':'Tuning failed.');load();}
 catch(e){msg('Lost connection.');}
 b.disabled=false;
};
document.getElementById('save').onclick=async()=>{try{const r=await fetch(q('/save'));msg(r.ok?'Saved. Scans use these settings now and after restart.':'Save failed.');}catch(e){msg('Lost connection.');}};
document.getElementById('reset').onclick=async()=>{try{const r=await fetch(q('/reset'));if(r.ok){msg('Back to defaults.');load();}}catch(e){msg('Lost connection.');}};
load();startLive();
</script></body></html>)HTML";

// Every request must carry ?t=<token>: the hotspot is private, but this keeps
// anyone else who joins it from watching the camera.
bool authorised(httpd_req_t* req) {
  char query[160], t[24];
  if (httpd_req_get_url_query_str(req, query, sizeof query) == ESP_OK &&
      httpd_query_key_value(query, "t", t, sizeof t) == ESP_OK && std::strcmp(t, g_token) == 0)
    return true;
  httpd_resp_set_status(req, "403 Forbidden");
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, "Use the link printed in the Serial Monitor (it changes each time preview starts).");
  return false;
}

void noCache(httpd_req_t* req) { httpd_resp_set_hdr(req, "Cache-Control", "no-store"); }

esp_err_t pageHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  noCache(req);
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, kPage, sizeof kPage - 1);
}

esp_err_t sendJpeg(httpd_req_t* req, const std::string& jpeg) {
  noCache(req);
  httpd_resp_set_type(req, "image/jpeg");
  return httpd_resp_send(req, jpeg.data(), jpeg.size());
}

esp_err_t frameHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  std::string jpeg;
  if (!cameraFrame(jpeg)) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No frame");
  return sendJpeg(req, jpeg);
}

esp_err_t lastHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  std::string jpeg;
  if (!cameraLastPhoto(jpeg)) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No scan photo yet");
  return sendJpeg(req, jpeg);
}

// Motion-JPEG: the browser shows each part as it arrives. Ends when the
// browser disconnects or preview stops.
esp_err_t streamHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  static const char kBoundary[] = "\r\n--frame\r\n";
  noCache(req);
  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  std::string jpeg;
  unsigned frameNo = 0;
  while (g_stream) {
    if (!cameraFrame(jpeg)) {
      delay(50);
      continue;
    }
    if (++frameNo % 3 == 0) g_liveFocus = static_cast<float>(cameraFocusScore(jpeg));
    char head[80];
    int n = snprintf(head, sizeof head, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)jpeg.size());
    if (httpd_resp_send_chunk(req, kBoundary, sizeof kBoundary - 1) != ESP_OK ||
        httpd_resp_send_chunk(req, head, n) != ESP_OK ||
        httpd_resp_send_chunk(req, jpeg.data(), jpeg.size()) != ESP_OK)
      break;
    delay(5);  // leave the camera free for a scan between frames
  }
  httpd_resp_send_chunk(req, nullptr, 0);
  return ESP_OK;
}

esp_err_t detailHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  std::string jpeg;
  if (!cameraLastDetail(jpeg)) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No enhanced image yet");
  return sendJpeg(req, jpeg);
}

esp_err_t focusHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  noCache(req);
  httpd_resp_set_type(req, "application/json");
  char out[32];
  snprintf(out, sizeof out, "{\"focus\":%d}", int(g_liveFocus));
  return httpd_resp_sendstr(req, out);
}

esp_err_t autotuneHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  const std::string report = cameraAutoTune();
  LOGF("preview", "%s", report.c_str());
  noCache(req);
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  return httpd_resp_sendstr(req, report.c_str());
}

esp_err_t setHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  char query[160], name[24], val[8];
  if (httpd_req_get_url_query_str(req, query, sizeof query) != ESP_OK ||
      httpd_query_key_value(query, "var", name, sizeof name) != ESP_OK ||
      httpd_query_key_value(query, "val", val, sizeof val) != ESP_OK)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Need var and val");
  char* end;
  long v = std::strtol(val, &end, 10);
  if (*end != '\0' || !cameraSet(name, static_cast<int>(v)))
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown setting or out of range");
  return httpd_resp_sendstr(req, "ok");
}

esp_err_t tuningHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  noCache(req);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, cameraTuningJson().c_str());
}

esp_err_t saveHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  cameraSaveTuning();
  LOGF("preview", "camera settings saved");
  return httpd_resp_sendstr(req, "ok");
}

esp_err_t resetHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  cameraResetTuning();
  LOGF("preview", "camera settings reset to defaults");
  return httpd_resp_sendstr(req, "ok");
}
// ---- Send: scan with Claude from the page ----
// The page asks; the app (main.cpp) takes the photo, leaves this network to
// reach Claude over the hotspot, and comes back with the result.
volatile bool g_sendWanted = false;
std::string g_result = "{\"n\":0}";  // last Send result as JSON for the page
int g_resultNo = 0;
std::mutex g_resultLock;
bool g_suspended = false;  // network down for a Send, coming back

esp_err_t sendHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  g_sendWanted = true;
  // How long to hold still, so the page can count down.
  return httpd_resp_sendstr(req, std::to_string(cameraHoldEstimateMs()).c_str());
}

esp_err_t resultHandler(httpd_req_t* req) {
  if (!authorised(req)) return ESP_OK;
  noCache(req);
  httpd_resp_set_type(req, "application/json");
  std::string r;
  {
    std::lock_guard<std::mutex> lock(g_resultLock);
    r = g_result;
  }
  return httpd_resp_sendstr(req, r.c_str());
}

std::string jsonText(const std::string& s) {
  std::string o = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += char(c);
    else if (c < 0x20) o += ' ';
    else o += char(c);
  }
  return o + "\"";
}

bool startNetwork() {
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  return WiFi.softAP(g_apName.c_str(), g_apPass.c_str(), 6, 0, 2);
}

void stopServers() {
  httpd_handle_t s = g_server, st = g_stream;
  g_server = nullptr;
  g_stream = nullptr;  // ends any running stream loop
  delay(500);          // let a stream finish its current frame
  if (st) httpd_stop(st);
  if (s) httpd_stop(s);
}

// A GET route with every other field zeroed (the struct grows with IDF options).
httpd_uri_t getUri(const char* path, esp_err_t (*handler)(httpd_req_t*)) {
  httpd_uri_t u = {};
  u.uri = path;
  u.method = HTTP_GET;
  u.handler = handler;
  return u;
}

bool startServers(std::string& error) {
  httpd_config_t c = HTTPD_DEFAULT_CONFIG();
  c.stack_size = 8192;
  c.max_uri_handlers = 14;
  c.lru_purge_enable = true;  // a phone that left mid-stream doesn't hold a slot forever
  c.core_id = 0;
  if (httpd_start(&g_server, &c) != ESP_OK) {
    g_server = nullptr;
    error = "Couldn't start the preview page.";
    return false;
  }
  const httpd_uri_t uris[] = {
      getUri("/", pageHandler),
      getUri("/frame", frameHandler),
      getUri("/last.jpg", lastHandler),
      getUri("/set", setHandler),
      getUri("/tuning", tuningHandler),
      getUri("/save", saveHandler),
      getUri("/reset", resetHandler),
      getUri("/detail.jpg", detailHandler),
      getUri("/focus", focusHandler),
      getUri("/send", sendHandler),
      getUri("/result", resultHandler),
      getUri("/autotune", autotuneHandler),
  };
  for (const auto& u : uris) httpd_register_uri_handler(g_server, &u);
  // The live view gets its own server: esp_http_server answers one request
  // at a time, so a stream on port 80 would block every button on the page.
  httpd_config_t sc = HTTPD_DEFAULT_CONFIG();
  sc.server_port = 81;
  sc.ctrl_port = c.ctrl_port + 1;
  sc.stack_size = 8192;
  sc.max_open_sockets = 2;
  sc.lru_purge_enable = true;
  sc.core_id = 0;
  if (httpd_start(&g_stream, &sc) != ESP_OK) {
    g_stream = nullptr;
    httpd_stop(g_server);
    g_server = nullptr;
    error = "Couldn't start the live view.";
    return false;
  }
  const httpd_uri_t stream = getUri("/stream", streamHandler);
  httpd_register_uri_handler(g_stream, &stream);
  return true;
}

// Forgets the network and secret; the app rejoins the hotspot.
void endPreview() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  g_apName.clear();
  g_apPass.clear();
  std::memset(g_token, 0, sizeof g_token);
  g_suspended = false;
  g_sendWanted = false;
  previewEnded();
}
}  // namespace

bool previewStart(std::string& message) {
  if (!g_server && !g_suspended) {
    // Own network: random name suffix and password, new every start.
    static const char kAlpha[] = "abcdefghjkmnpqrstuvwxyz23456789";  // no 0/o, 1/l/i
    g_apPass.clear();
    for (int i = 0; i < 10; ++i) g_apPass += kAlpha[esp_random() % (sizeof kAlpha - 1)];
    char name[16];
    snprintf(name, sizeof name, "AI-Calc-%04X", (unsigned)(esp_random() & 0xFFFF));
    g_apName = name;
    snprintf(g_token, sizeof g_token, "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
    // The chip has one radio. Keeping the hotspot connection at the same time
    // made laptops fail to join, so the preview has the radio to itself; the
    // hotspot comes back when it stops (and briefly for each Send).
    if (!startNetwork()) {
      message = "Couldn't start the preview network.";
      endPreview();
      return false;
    }
    if (!startServers(message)) {
      endPreview();
      return false;
    }
  }
  g_startedAt = millis();
  message = "Preview on for 15 minutes.\n"
            "1. On your laptop, join the Wi-Fi network  " + g_apName + "\n"
            "   password  " + g_apPass + "\n"
            "   (it has no internet; that's expected. Stay on it anyway.\n"
            "   Use the laptop, not the iPhone: an iPhone joining it would stop its own hotspot.)\n"
            "2. Open  http://192.168.4.1/?t=" + std::string(g_token) + "\n"
            "Send on the page scans with Claude: the calculator steps off this network for a few\n"
            "seconds to use the hotspot, then comes back. Type preview off when done.";
  return true;
}

void previewStop() {
  if (!g_server && !g_suspended) return;
  stopServers();
  endPreview();
  LOGF("preview", "stopped");
}

bool previewRunning() { return g_server != nullptr; }
bool previewActive() { return g_server != nullptr || g_suspended; }

void previewService() {
  if (!g_server) return;  // suspended for a Send: the app brings it back
  if (millis() - g_startedAt > kLimitMs) previewStop();
}

bool previewTakeSend() {
  if (!g_sendWanted) return false;
  g_sendWanted = false;
  return g_server != nullptr;
}

void previewSuspend() {
  if (!g_server) return;
  stopServers();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  g_suspended = true;
}

bool previewResume() {
  if (!g_suspended) return false;
  std::string err;
  if (!startNetwork() || !startServers(err)) {
    LOGF("preview", "couldn't bring the preview back; type preview to start it again");
    stopServers();
    endPreview();
    return false;
  }
  g_suspended = false;
  g_startedAt = millis();  // a fresh 15 minutes after each Send
  return true;
}

void previewSetResult(const std::string& replyJson, const std::string& error) {
  calc::Json parsed;
  std::string parseError;
  const bool replyOk = error.empty() && calc::Json::parse(replyJson, parsed, parseError);
  std::lock_guard<std::mutex> lock(g_resultLock);
  ++g_resultNo;
  g_result = "{\"n\":" + std::to_string(g_resultNo) +
             (replyOk ? ",\"reply\":" + replyJson
                      : ",\"error\":" + jsonText(error.empty() ? "Claude's reply couldn't be read." : error)) +
             "}";
}
