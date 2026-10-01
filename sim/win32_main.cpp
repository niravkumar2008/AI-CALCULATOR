// Windows simulator for the modified Casio (Stage 4: the full calculator).
// Uses only what Windows ships with: Win32 + GDI for the window, a file dialog
// as the camera, WIC to shrink the photo, and WinHTTP to call Claude.
//
// Type on the keyboard like the calculator's keys (see keysForChar in
// core/device.h). In AI SOLVE (m then 4), '=' takes a picture: with
// api_key.txt next to the exe it asks for a photo and sends it to Claude;
// without it, it plays back the sample replies in samples/.
#include <windows.h>
#include <commdlg.h>
#include <wincodec.h>
#include <winhttp.h>

#include <algorithm>
#include <thread>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../core/device.h"
#include "../core/claude_api.h"

using namespace calc;

namespace {

constexpr int kZoom = 5;
constexpr int kPad = 20;
constexpr int kLcdW = Framebuffer::kWidth * kZoom;
constexpr int kLcdH = Framebuffer::kHeight * kZoom;
constexpr int kIconH = 0;  // the status bar is part of the screen now
constexpr COLORREF kBg = RGB(40, 44, 52);
constexpr UINT_PTR kTimer = 1;

constexpr UINT WM_APP_EVENT = WM_APP + 1;

Device g_dev;
HWND g_hwnd = nullptr;
std::string g_apiKey;                 // empty: sample-reply mode
std::wstring g_lastPhoto;
bool g_online = true, g_hasKey = true;
std::wstring g_dir;                   // folder containing the exe
std::vector<std::wstring> g_samples;  // sample reply files, sorted
size_t g_next = 0;
int g_reqId = 0;
DWORD g_reqStart = 0;
bool g_captured = false;

std::string readFile(const std::wstring& path) {
  std::ifstream f(path.c_str(), std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

void loadSettings() {
  wchar_t buf[MAX_PATH];
  GetModuleFileNameW(nullptr, buf, MAX_PATH);
  g_dir = buf;
  g_dir = g_dir.substr(0, g_dir.find_last_of(L"\\/") + 1);
  // API key: first line of api_key.txt, whitespace trimmed.
  std::string key = readFile(g_dir + L"api_key.txt");
  key = key.substr(0, key.find_first_of("\r\n"));
  const char* ws = " \t\xEF\xBB\xBF";  // spaces, tabs, UTF-8 BOM bytes
  size_t b = key.find_first_not_of(ws), e = key.find_last_not_of(ws);
  g_apiKey = b == std::string::npos ? "" : key.substr(b, e - b + 1);
  g_samples.clear();
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW((g_dir + L"samples\\*.json").c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return;
  do g_samples.push_back(g_dir + L"samples\\" + fd.cFileName);
  while (FindNextFileW(h, &fd));
  FindClose(h);
  std::sort(g_samples.begin(), g_samples.end());
}


// ---- Live mode: photo -> WIC -> Claude, on a worker thread ----

struct Event {  // posted from the worker thread to the window
  enum Kind { Captured, Partial, Reply, Fail } kind;
  int id;
  std::string text;
  double confidence = 0;
  Failure failure = Failure::ApiError;
};

void post(Event* e) { PostMessageW(g_hwnd, WM_APP_EVENT, 0, reinterpret_cast<LPARAM>(e)); }

template <class T>
struct Com {  // releases a COM pointer when it goes out of scope
  T* p = nullptr;
  ~Com() { if (p) p->Release(); }
  T** operator&() { return &p; }
  T* operator->() { return p; }
};

// EXIF orientation (1-8) of a phone photo, 1 if absent.
UINT exifOrientation(IWICBitmapFrameDecode* frame) {
  Com<IWICMetadataQueryReader> q;
  if (FAILED(frame->GetMetadataQueryReader(&q))) return 1;
  PROPVARIANT v;
  PropVariantInit(&v);
  UINT o = 1;
  if (SUCCEEDED(q->GetMetadataByName(L"/app1/ifd/{ushort=274}", &v)) && v.vt == VT_UI2) o = v.uiVal;
  PropVariantClear(&v);
  return o;
}

// Loads any image Windows can decode, applies the EXIF rotation, shrinks it
// so the long edge is at most 1600 px, and re-encodes it as grayscale JPEG.
bool photoToJpeg(const std::wstring& path, std::string& jpeg) {
  Com<IWICImagingFactory> f;
  if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                              IID_IWICImagingFactory, reinterpret_cast<void**>(&f))))
    return false;
  Com<IWICBitmapDecoder> dec;
  Com<IWICBitmapFrameDecode> frame;
  if (FAILED(f->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                          WICDecodeMetadataCacheOnDemand, &dec)) ||
      FAILED(dec->GetFrame(0, &frame)))
    return false;

  IWICBitmapSource* src = frame.p;
  Com<IWICBitmapFlipRotator> rot;
  static const WICBitmapTransformOptions kRotate[9] = {
      WICBitmapTransformRotate0, WICBitmapTransformRotate0, WICBitmapTransformFlipHorizontal,
      WICBitmapTransformRotate180, WICBitmapTransformFlipVertical,
      static_cast<WICBitmapTransformOptions>(WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal),
      WICBitmapTransformRotate90,
      static_cast<WICBitmapTransformOptions>(WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal),
      WICBitmapTransformRotate270};
  UINT o = exifOrientation(frame.p);
  if (o >= 2 && o <= 8 && SUCCEEDED(f->CreateBitmapFlipRotator(&rot)) &&
      SUCCEEDED(rot->Initialize(src, kRotate[o])))
    src = rot.p;

  UINT w, h;
  src->GetSize(&w, &h);
  const double scale = std::min(1.0, 1600.0 / std::max(w, h));
  const UINT nw = std::max(1u, static_cast<UINT>(w * scale + 0.5));
  const UINT nh = std::max(1u, static_cast<UINT>(h * scale + 0.5));
  Com<IWICBitmapScaler> sc;
  Com<IWICFormatConverter> gray;
  if (FAILED(f->CreateBitmapScaler(&sc)) ||
      FAILED(sc->Initialize(src, nw, nh, WICBitmapInterpolationModeFant)) ||
      FAILED(f->CreateFormatConverter(&gray)) ||
      FAILED(gray->Initialize(sc.p, GUID_WICPixelFormat8bppGray, WICBitmapDitherTypeNone,
                              nullptr, 0, WICBitmapPaletteTypeCustom)))
    return false;

  Com<IStream> st;
  Com<IWICBitmapEncoder> enc;
  Com<IWICBitmapFrameEncode> fe;
  Com<IPropertyBag2> props;
  if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &st)) ||
      FAILED(f->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &enc)) ||
      FAILED(enc->Initialize(st.p, WICBitmapEncoderNoCache)) ||
      FAILED(enc->CreateNewFrame(&fe, &props)))
    return false;
  PROPBAG2 opt = {};
  opt.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
  VARIANT q;
  VariantInit(&q);
  q.vt = VT_R4;
  q.fltVal = 0.85f;
  props->Write(1, &opt, &q);
  WICPixelFormatGUID fmt = GUID_WICPixelFormat8bppGray;
  if (FAILED(fe->Initialize(props.p)) || FAILED(fe->SetSize(nw, nh)) ||
      FAILED(fe->SetPixelFormat(&fmt)) || FAILED(fe->WriteSource(gray.p, nullptr)) ||
      FAILED(fe->Commit()) || FAILED(enc->Commit()))
    return false;

  STATSTG stat;
  if (FAILED(st->Stat(&stat, STATFLAG_NONAME))) return false;
  jpeg.resize(static_cast<size_t>(stat.cbSize.QuadPart));
  LARGE_INTEGER zero = {};
  ULONG got = 0;
  st->Seek(zero, STREAM_SEEK_SET, nullptr);
  return SUCCEEDED(st->Read(&jpeg[0], static_cast<ULONG>(jpeg.size()), &got)) && got == jpeg.size();
}

Failure failureForWinHttp(DWORD err, std::string& detail) {
  switch (err) {
    case ERROR_WINHTTP_TIMEOUT: return Failure::Timeout;
    case ERROR_WINHTTP_SECURE_FAILURE:
      detail = "Secure connection to Claude failed.";
      return Failure::ApiError;
    default:  // name not resolved, cannot connect, connection reset...
      detail = "Can't reach Claude (error " + std::to_string(err) + ").";
      return Failure::NoConnection;
  }
}

// Sends the photo and streams the reply back to the window.
void solve(int id, const std::string& jpeg) {
  const std::string body = buildSolveRequest(jpeg, "", g_dev.ai().effortParam());
  const std::wstring headers = L"content-type: application/json\r\nanthropic-version: " +
                               std::wstring(kApiVersion, kApiVersion + strlen(kApiVersion)) +
                               L"\r\nx-api-key: " + std::wstring(g_apiKey.begin(), g_apiKey.end()) + L"\r\n";
  HINTERNET s = WinHttpOpen(L"AI-Calc-Simulator/2", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!s)  // Windows older than 8.1
    s = WinHttpOpen(L"AI-Calc-Simulator/2", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  HINTERNET c = s ? WinHttpConnect(s, L"api.anthropic.com", INTERNET_DEFAULT_HTTPS_PORT, 0) : nullptr;
  HINTERNET r = c ? WinHttpOpenRequest(c, L"POST", L"/v1/messages", nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
                  : nullptr;
  auto close = [&] {
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    if (s) WinHttpCloseHandle(s);
  };
  auto fail = [&](Failure f, const std::string& detail) {
    post(new Event{Event::Fail, id, detail, 0, f});
    close();
  };
  if (r) WinHttpSetTimeouts(r, 10000, 15000, 30000, 120000);  // resolve, connect, send, receive
  if (!r || !WinHttpSendRequest(r, headers.c_str(), static_cast<DWORD>(-1L), const_cast<char*>(body.data()),
                                static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) ||
      !WinHttpReceiveResponse(r, nullptr)) {
    std::string detail;
    Failure f = failureForWinHttp(GetLastError(), detail);
    return fail(f, detail);
  }
  DWORD status = 0, len = sizeof(status);
  WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX, &status, &len, WINHTTP_NO_HEADER_INDEX);

  StreamReader stream;
  std::string errorBody, shownAnswer;
  char buf[4096];
  DWORD got = 0;
  while (true) {
    if (!WinHttpReadData(r, buf, sizeof(buf), &got)) {
      std::string detail;
      Failure f = failureForWinHttp(GetLastError(), detail);
      return fail(f, detail);
    }
    if (got == 0) break;
    if (status != 200) {
      errorBody.append(buf, got);
      continue;
    }
    stream.feed(std::string(buf, got));
    double conf;
    std::string answer;
    if (peekAnswer(stream.text(), conf, answer) && answer != shownAnswer) {
      shownAnswer = answer;
      post(new Event{Event::Partial, id, answer, conf});
    }
  }
  Failure f;
  std::string detail;
  if (classifyFailure(static_cast<int>(status), errorBody, stream, f, detail)) return fail(f, detail);
  post(new Event{Event::Reply, id, stream.text()});
  close();
}

void startLive(int id) {
  wchar_t file[MAX_PATH] = L"";
  OPENFILENAMEW ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = g_hwnd;
  ofn.lpstrFilter = L"Photos\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tif;*.tiff;*.heic;*.webp\0All files\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = L"Take a picture: choose a photo of the problem";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
  if (!GetOpenFileNameW(&ofn)) {
    g_dev.onKey(DKey::AC);  // dialog cancelled: back to AI SOLVE
    return;
  }
  g_lastPhoto = file;
  std::wstring path = file;
  std::thread([id, path] {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    std::string jpeg;
    bool ok = photoToJpeg(path, jpeg);
    CoUninitialize();
    if (!ok) return post(new Event{Event::Fail, id, "Couldn't open that image.", 0, Failure::Camera});
    post(new Event{Event::Captured, id, ""});
    solve(id, jpeg);
  }).detach();
}

void handleEvent(Event* e) {
  switch (e->kind) {
    case Event::Captured: g_dev.onCaptured(e->id); break;
    case Event::Partial: g_dev.onPartialAnswer(e->id, e->confidence, e->text); break;
    case Event::Reply: g_dev.onReply(e->id, e->text); break;
    case Event::Fail: g_dev.onFailure(e->id, e->failure, e->text); break;
  }
  delete e;
}

// Sample mode: plays the role of camera + network for the current request.
void advanceRequest() {
  if (g_reqId == 0) return;
  DWORD t = GetTickCount() - g_reqStart;
  if (!g_captured && t > 600) {
    g_dev.onCaptured(g_reqId);
    g_captured = true;
  }
  if (t > 2000) {
    if (g_samples.empty())
      g_dev.onFailure(g_reqId, Failure::Camera, "No sample replies found next to the exe.");
    else
      g_dev.onReply(g_reqId, readFile(g_samples[g_next++ % g_samples.size()]));
    g_reqId = 0;
  }
}

void press(DKey k) {
  g_dev.tick(GetTickCount());
  g_dev.onKey(k);
  if (g_reqId && g_dev.activeRequest() != g_reqId) g_reqId = 0;  // cancelled (AC, MODE, OFF)
  int id = g_dev.takeRequest();
  if (!id) return;
  loadSettings();  // re-read api_key.txt so a fixed key works on retry
  if (!g_apiKey.empty()) return startLive(id);
  g_reqId = id;
  g_reqStart = GetTickCount();
  g_captured = false;
}

void paint(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC dc = BeginPaint(hwnd, &ps);
  RECT rc;
  GetClientRect(hwnd, &rc);

  // Draw off-screen, then copy, to avoid flicker.
  HDC mem = CreateCompatibleDC(dc);
  HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
  HGDIOBJ old = SelectObject(mem, bmp);
  HBRUSH bg = CreateSolidBrush(kBg);
  FillRect(mem, &rc, bg);
  DeleteObject(bg);

  Framebuffer fb;
  g_dev.render(fb);

  // LCD pixels: scale up with a 1px gap so the dot matrix is visible.
  static uint32_t pixels[kLcdH][kLcdW];
  const bool off = g_dev.isOff(), dots = g_dev.dotMatrix();
  const uint32_t lcd = off ? 0xA8AE98 : 0xC4CCB2, dot = 0x1E241E;
  for (int y = 0; y < kLcdH; ++y)
    for (int x = 0; x < kLcdW; ++x) {
      bool gap = dots && ((x % kZoom == kZoom - 1) || (y % kZoom == kZoom - 1));
      pixels[y][x] = (!gap && fb.get(x / kZoom, y / kZoom)) ? dot : lcd;
    }
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = kLcdW;
  bi.bmiHeader.biHeight = -kLcdH;  // top-down
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  const int lcdTop = kPad + kIconH;
  RECT glass = {kPad - 6, kPad - 6, kPad + kLcdW + 6, lcdTop + kLcdH + 6};
  HBRUSH gb = CreateSolidBrush(RGB(0xB2, 0xBA, 0xA0));
  FillRect(mem, &glass, gb);
  DeleteObject(gb);
  SetDIBitsToDevice(mem, kPad, lcdTop, kLcdW, kLcdH, 0, 0, 0, kLcdH, pixels, &bi, DIB_RGB_COLORS);

  // Fixed icons row and help text.
  SetBkMode(mem, TRANSPARENT);
  HFONT font = CreateFontW(18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  HGDIOBJ oldFont = SelectObject(mem, font);
  SetTextColor(mem, RGB(0x1E, 0x24, 0x1E));
  SetTextColor(mem, RGB(0xDD, 0xDD, 0xDD));
  const int helpTop = lcdTop + kLcdH + 20;
  const wchar_t* help[] = {
      L"0-9 . + - * / ^ ( )   Enter =   Backspace DEL   Esc AC   arrows   [ SHIFT   ] ALPHA   m MODE",
      L"s c t sin cos tan   l log   n ln   r \u221A   q x\u00B2   p \u03C0   ! x!   ~ (\u2212)   E \u00D710\u02E3   a Ans   w S\u21D4D",
      L"F6  SCAN A PHOTO (picks a file)   F2 hotspot   F3 key on/off   F4 power   F5 USB unlock",
  };
  for (int i = 0; i < 3; ++i) TextOutW(mem, kPad, helpTop + i * 24, help[i], lstrlenW(help[i]));
  std::wstring status =
      g_apiKey.empty()
          ? std::wstring(L"No api_key.txt next to the exe: F6 plays sample replies, not your photo.")
          : L"Live mode: Claude.    Last photo: " +
                (g_lastPhoto.empty() ? L"none" : g_lastPhoto.substr(g_lastPhoto.find_last_of(L"\\/") + 1));
  SetTextColor(mem, RGB(0x99, 0xA3, 0xB3));
  TextOutW(mem, kPad, helpTop + 3 * 24 + 8, status.c_str(), static_cast<int>(status.size()));
  SelectObject(mem, oldFont);
  DeleteObject(font);

  BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
  SelectObject(mem, old);
  DeleteObject(bmp);
  DeleteDC(mem);
  EndPaint(hwnd, &ps);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_APP_EVENT:
      handleEvent(reinterpret_cast<Event*>(lp));
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    case WM_TIMER:
      g_dev.tick(GetTickCount());
      if (g_reqId && g_dev.activeRequest() != g_reqId) g_reqId = 0;
      advanceRequest();
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    case WM_KEYDOWN:
      switch (wp) {
        case VK_RETURN: press(DKey::Eq); break;
        case VK_ESCAPE: case VK_DELETE: press(DKey::AC); break;
        case VK_BACK: press(DKey::Del); break;
        case VK_UP: press(DKey::Up); break;
        case VK_DOWN: press(DKey::Down); break;
        case VK_LEFT: press(DKey::Left); break;
        case VK_RIGHT: press(DKey::Right); break;
        case VK_F2: g_online = !g_online; g_dev.setOnline(g_online); break;
        case VK_F3: g_hasKey = !g_hasKey; g_dev.setHasApiKey(g_hasKey); break;
        case VK_F4:
          if (g_dev.isOff()) press(DKey::On);
          else { press(DKey::Shift); press(DKey::AC); }
          break;
        case VK_F5: g_dev.usbUnlock(); break;
        case VK_F6:  // scan a photo: MODE 4 (AI SOLVE) then =, which opens the photo picker
          if (g_dev.isOff()) press(DKey::On);
          if (g_dev.mode() != Mode::Ai) { press(DKey::AC); press(DKey::Mode); press(DKey::D4); }
          press(DKey::Eq);
          break;
      }
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    case WM_CHAR: {
      DKey ks[2];
      int n = 0;
      const wchar_t c = static_cast<wchar_t>(wp);
      if (c < 128 && c != '\r' && c != 27 && c != 8 && keysForChar(static_cast<char>(c), ks, n))
        for (int i = 0; i < n; ++i) press(ks[i]);
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT:
      paint(hwnd);
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int show) {
  loadSettings();
  g_dev.setNoKeyHelp("Put your key in api_key.txt next to the exe.");
  g_dev.setOnline(true);
  g_dev.setBlinkingCursor(true);
  g_dev.setNetInfo("PC", g_apiKey.empty() ? "sample mode" : "api_key.txt", "Edit api_key.txt");
  g_dev.tick(GetTickCount());
  WNDCLASSW wc = {};
  wc.lpfnWndProc = wndProc;
  wc.hInstance = inst;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = L"AiCalcSim";
  RegisterClassW(&wc);
  RECT r = {0, 0, kLcdW + 2 * kPad, kPad + kIconH + kLcdH + 20 + 4 * 24 + 8 + kPad};
  const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  AdjustWindowRect(&r, style, FALSE);
  HWND hwnd = CreateWindowW(L"AiCalcSim", L"AI Calculator simulator — Stage 4",
                            style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                            r.bottom - r.top, nullptr, nullptr, inst, nullptr);
  g_hwnd = hwnd;
  ShowWindow(hwnd, show);
  SetTimer(hwnd, kTimer, 100, nullptr);
  MSG m;
  while (GetMessageW(&m, nullptr, 0, 0)) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  return 0;
}
