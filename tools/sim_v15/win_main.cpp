// Proofline v15 simulator: the window. Left, a drawn calculator (navy shell, fx-115ES key
// layout) whose screen shows the REAL v15 frames (board.cpp runs the firmware's loop around
// core/ and firmware-v15-lcd/src/ui*.cpp); right, test controls (battery, USB, Wi-Fi, link,
// exam mode, time, the fake AI's reply, the fake camera's picture, screenshots).
// Plain Win32 + GDI; WIC for pictures. `--selftest [folder]` runs a scripted tour and
// saves its screens as PNGs (exit code 0 = every check passed).
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <windowsx.h>
#include <cmath>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#include "board.h"
#include "sim_host.h"
#include "wic_image.h"

using calc::DKey;
namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------- the keypad

enum Style : uint8_t { kFn, kNum, kOrange, kShift, kAlpha, kNav, kOn };
struct KeyDef {
  DKey k;
  const wchar_t* label;
  const wchar_t* shift;  // printed above the key, left (orange)
  const wchar_t* alpha;  // printed above the key, right (red)
  Style style;
  const wchar_t* base = nullptr;  // BASE-N's number mode (DEC HEX BIN OCT), above the key, right (teal)
};

// In DKey order (core/keys.h): kKeys[i].k == DKey(i), checked at start.
const KeyDef kKeys[] = {
    {DKey::Shift, L"SHIFT", nullptr, nullptr, kShift},
    {DKey::Alpha, L"ALPHA", nullptr, nullptr, kAlpha},
    {DKey::Up, L"▲", nullptr, nullptr, kNav},
    {DKey::Down, L"▼", nullptr, nullptr, kNav},
    {DKey::Left, L"◀", nullptr, nullptr, kNav},
    {DKey::Right, L"▶", nullptr, nullptr, kNav},
    {DKey::Mode, L"MODE", L"SETUP", nullptr, kFn},
    {DKey::On, L"ON", nullptr, nullptr, kOn},
    {DKey::Calc, L"CALC", L"SOLVE", nullptr, kFn},
    {DKey::Integral, L"∫dx", L"d/dx", nullptr, kFn},
    {DKey::Inv, L"x⁻¹", L"x!", nullptr, kFn},
    {DKey::LogAB, L"logₐb", nullptr, nullptr, kFn},
    {DKey::Frac, L"□/□", nullptr, nullptr, kFn},
    {DKey::Sqrt, L"√□", L"∛", nullptr, kFn},
    {DKey::Sq, L"x²", L"x³", nullptr, kFn, L"DEC"},
    {DKey::Pow, L"x□", L"□√", nullptr, kFn, L"HEX"},
    {DKey::Log, L"log", L"10□", nullptr, kFn, L"BIN"},
    {DKey::Ln, L"ln", L"e□", nullptr, kFn, L"OCT"},
    {DKey::Neg, L"(−)", L"∠", L"A", kFn},
    {DKey::Dms, L"° ' \"", nullptr, L"B", kFn},
    {DKey::Hyp, L"hyp", L"Abs", L"C", kFn},
    {DKey::Sin, L"sin", L"sin⁻¹", L"D", kFn},
    {DKey::Cos, L"cos", L"cos⁻¹", L"E", kFn},
    {DKey::Tan, L"tan", L"tan⁻¹", L"F", kFn},
    {DKey::Rcl, L"RCL", L"STO", nullptr, kFn},
    {DKey::Eng, L"ENG", L"←", nullptr, kFn},
    {DKey::Open, L"(", L"%", nullptr, kFn},
    {DKey::Close, L")", L",", L"X", kFn},
    {DKey::SD, L"S⇔D", nullptr, L"Y", kFn},
    {DKey::MPlus, L"M+", L"M−", L"M", kFn},
    {DKey::D7, L"7", nullptr, nullptr, kNum},
    {DKey::D8, L"8", nullptr, nullptr, kNum},
    {DKey::D9, L"9", L"CLR", nullptr, kNum},
    {DKey::Del, L"DEL", L"INS", nullptr, kOrange},
    {DKey::AC, L"AC", L"OFF", nullptr, kOrange},
    {DKey::D4, L"4", L"MATRIX", nullptr, kNum},
    {DKey::D5, L"5", L"VECTOR", nullptr, kNum},
    {DKey::D6, L"6", nullptr, nullptr, kNum},
    {DKey::Mul, L"×", L"nPr", nullptr, kNum},
    {DKey::Div, L"÷", L"nCr", nullptr, kNum},
    {DKey::D1, L"1", nullptr, nullptr, kNum},
    {DKey::D2, L"2", L"CMPLX", nullptr, kNum},
    {DKey::D3, L"3", L"BASE", nullptr, kNum},
    {DKey::Add, L"+", nullptr, nullptr, kNum},
    {DKey::Sub, L"−", nullptr, nullptr, kNum},
    {DKey::D0, L"0", L"Rnd", nullptr, kNum},
    {DKey::Dot, L".", L"Ran#", L"RanInt", kNum},
    {DKey::Exp10, L"×10ˣ", L"π", L"e", kNum},
    {DKey::Ans, L"Ans", nullptr, nullptr, kNum},
    {DKey::Eq, L"=", nullptr, nullptr, kNum},
};
constexpr int kKeyCount = int(sizeof kKeys / sizeof kKeys[0]);
static_assert(kKeyCount == int(DKey::Count), "one KeyDef per DKey");

COLORREF rgbOf(uint32_t hex) { return RGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF); }

// The product's colours (firmware ui.h color::).
const COLORREF kWindowBg = rgbOf(0x0b0e14);
const COLORREF kShell = rgbOf(0x10141c);
const COLORREF kShellEdge = rgbOf(0x2a3242);
const COLORREF kBezel = rgbOf(0x05070a);
const COLORREF kText = rgbOf(0xeef2f6);
const COLORREF kDim = rgbOf(0x93a1b3);
const COLORREF kAccent = rgbOf(0xe07a4f);
const COLORREF kShiftText = rgbOf(0xf5b74f);
const COLORREF kAlphaText = rgbOf(0xe8606a);
const COLORREF kBaseText = rgbOf(0x5cc8c0);  // BASE-N's DEC HEX BIN OCT

// ---------------------------------------------------------------- state

Board g_board;
HWND g_hwnd = nullptr;
HINSTANCE g_inst = nullptr;
int g_scale = 2;
bool g_help = false;
int g_pressed = -1;  // key held down with the mouse
int g_flash = -1;    // key typed on the PC keyboard, shown pressed briefly
DWORD g_flashUntil = 0;
uint32_t g_shownFlushes = ~0u;
int g_shownLevel = -1;
uint32_t g_shownLog = ~0u;
DWORD g_statusAt = 0;
std::wstring g_exeDir;
std::wstring g_photoName = L"sample_photo.png (3x + 7 = 22)";

// layout (pixels, client coordinates)
RECT g_keyRect[kKeyCount];
RECT g_bodyRect, g_screenRect, g_padRect;
int g_bodyW = 0, g_bodyH = 0, g_clientW = 0, g_clientH = 0, g_panelX = 0;
// Everything but the LCD picture is sized in "logical" pixels times g_ui (the display's
// scaling: 1.25 at 125 %), so the window is crisp (DPI-aware) and still a normal size.
double g_ui = 1.0;
int S(double v) { return int(v * g_ui + 0.5); }
int kPanelW = 320;
HFONT g_fKey = nullptr, g_fNum = nullptr, g_fLabel = nullptr, g_fHead = nullptr, g_fHelp = nullptr,
      g_fSmall = nullptr, g_fUi = nullptr, g_fUiBold = nullptr;

// ---------------------------------------------------------------- layout

HFONT font(int px, int weight, const wchar_t* face) {
  return CreateFontW(-px, 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, face);
}

void layout() {
  const int s = g_scale;
  const int padX = S(24), bezel = S(10);
  const int scrW = simhost::kW * s, scrH = simhost::kH * s;
  g_bodyW = scrW + 2 * (padX + bezel);
  const int scrX = padX + bezel, scrY = S(40) + bezel;
  g_screenRect = {scrX, scrY, scrX + scrW, scrY + scrH};
  // Key rows: label strip + key. Shrunk (to half at most) if the window wouldn't fit.
  double h1 = S(46), h2 = S(50), h3 = S(60);
  const int fixed = scrY + scrH + bezel + S(16) + S(6) + S(30);  // above the keys + gaps + footer
  RECT work;
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
  const int avail = (work.bottom - work.top) - S(48) - fixed;  // window frame and title
  const double need = 2 * h1 + 3 * h2 + 4 * h3;
  if (need > avail) {
    const double k = std::max(0.5, avail / need);
    h1 *= k;
    h2 *= k;
    h3 *= k;
  }
  const int lbl = S(14), in = S(5);
  const int x0 = padX, iw = g_bodyW - 2 * padX;
  const double c6 = iw / 6.0, c5 = iw / 5.0;
  int y = scrY + scrH + bezel + S(16);
  auto cell = [&](int i, double cx, double cw, double top, double h) {
    g_keyRect[i] = {LONG(cx + in), LONG(top + lbl), LONG(cx + cw - in), LONG(top + h - in)};
  };
  // rows 0-1: SHIFT ALPHA [pad] MODE ON / CALC ∫dx [pad] x⁻¹ logₐb
  const int top0[] = {int(DKey::Shift), int(DKey::Alpha), int(DKey::Mode), int(DKey::On)};
  const int top1[] = {int(DKey::Calc), int(DKey::Integral), int(DKey::Inv), int(DKey::LogAB)};
  const int cols[] = {0, 1, 4, 5};
  for (int i = 0; i < 4; ++i) {
    cell(top0[i], x0 + cols[i] * c6, c6, y, h1);
    cell(top1[i], x0 + cols[i] * c6, c6, y + h1, h1);
  }
  // the round replay pad between them
  g_padRect = {LONG(x0 + 2 * c6 + 8), LONG(y + 6), LONG(x0 + 4 * c6 - 8), LONG(y + 2 * h1 - 6)};
  const int pw = g_padRect.right - g_padRect.left, ph = g_padRect.bottom - g_padRect.top;
  const int cx = (g_padRect.left + g_padRect.right) / 2, cy = (g_padRect.top + g_padRect.bottom) / 2;
  const int bw = pw * 30 / 100, bh = ph * 30 / 100;
  g_keyRect[int(DKey::Up)] = {cx - bw / 2, g_padRect.top + 4, cx + bw / 2, g_padRect.top + 4 + bh};
  g_keyRect[int(DKey::Down)] = {cx - bw / 2, g_padRect.bottom - 4 - bh, cx + bw / 2, g_padRect.bottom - 4};
  g_keyRect[int(DKey::Left)] = {g_padRect.left + 6, cy - bh / 2, g_padRect.left + 6 + bw, cy + bh / 2};
  g_keyRect[int(DKey::Right)] = {g_padRect.right - 6 - bw, cy - bh / 2, g_padRect.right - 6, cy + bh / 2};
  double yy = y + 2 * h1 + 6;
  // rows 2-4: six function keys each (Frac .. M+)
  for (int i = int(DKey::Frac); i <= int(DKey::MPlus); ++i) {
    const int n = i - int(DKey::Frac);
    cell(i, x0 + (n % 6) * c6, c6, yy + (n / 6) * h2, h2);
  }
  yy += 3 * h2 + 6;
  // rows 5-8: five big keys each (7 .. =)
  for (int i = int(DKey::D7); i <= int(DKey::Eq); ++i) {
    const int n = i - int(DKey::D7);
    cell(i, x0 + (n % 5) * c5, c5, yy + (n / 5) * h3, h3);
  }
  yy += 4 * h3;
  g_bodyH = int(yy) + S(30);
  g_bodyRect = {0, 0, g_bodyW, g_bodyH};
  kPanelW = S(320);
  g_panelX = g_bodyW + S(12);
  g_clientW = g_panelX + kPanelW + S(8);
  g_clientH = std::max(g_bodyH, S(720));

  for (HFONT* f : {&g_fKey, &g_fNum, &g_fLabel, &g_fHead, &g_fHelp, &g_fSmall})
    if (*f) DeleteObject(*f);
  const int keyH = g_keyRect[int(DKey::Sin)].bottom - g_keyRect[int(DKey::Sin)].top;
  const int numH = g_keyRect[int(DKey::D7)].bottom - g_keyRect[int(DKey::D7)].top;
  g_fKey = font(std::max(S(11), std::min(S(16), keyH * 62 / 100)), FW_SEMIBOLD, L"Segoe UI");
  g_fNum = font(std::max(S(13), std::min(S(24), numH * 66 / 100)), FW_SEMIBOLD, L"Segoe UI");
  g_fLabel = font(S(12), FW_BOLD, L"Segoe UI");
  g_fHead = font(S(17), FW_BOLD, L"Segoe UI");
  g_fHelp = font(S(15), FW_NORMAL, L"Segoe UI");
  g_fSmall = font(S(12), FW_NORMAL, L"Segoe UI");
}

// ---------------------------------------------------------------- drawing

void text(HDC dc, const wchar_t* s, RECT r, COLORREF c, HFONT f, UINT fmt) {
  SelectObject(dc, f);
  SetTextColor(dc, c);
  DrawTextW(dc, s, -1, &r, fmt | DT_NOPREFIX);
}

void roundRect(HDC dc, RECT r, int radius, COLORREF fill, COLORREF edge) {
  HBRUSH b = CreateSolidBrush(fill);
  HPEN p = CreatePen(PS_SOLID, 1, edge);
  HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
  RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
  SelectObject(dc, ob);
  SelectObject(dc, op);
  DeleteObject(b);
  DeleteObject(p);
}

COLORREF lighten(COLORREF c, int d) {
  auto f = [d](int v) { return std::min(255, std::max(0, v + d)); };
  return RGB(f(GetRValue(c)), f(GetGValue(c)), f(GetBValue(c)));
}

// The panel's picture as the window shows it: RGB565 -> 24-bit, dimmed by the backlight.
void drawScreen(HDC dc) {
  static uint32_t px[simhost::kW * simhost::kH];
  const int level = simhost::lightLevel();
  const int k = level <= 0 ? 0 : 300 + 7 * level;  // /1000: 10 % dimmed still readable
  for (int i = 0; i < simhost::kW * simhost::kH; ++i) {
    const uint16_t v = simhost::frame[i];
    int r = (v >> 11) & 0x1F, g = (v >> 5) & 0x3F, b = v & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    px[i] = uint32_t(((r * k / 1000) << 16) | ((g * k / 1000) << 8) | (b * k / 1000));
  }
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof bi.bmiHeader;
  bi.bmiHeader.biWidth = simhost::kW;
  bi.bmiHeader.biHeight = -simhost::kH;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  SetStretchBltMode(dc, COLORONCOLOR);
  StretchDIBits(dc, g_screenRect.left, g_screenRect.top, g_screenRect.right - g_screenRect.left,
                g_screenRect.bottom - g_screenRect.top, 0, 0, simhost::kW, simhost::kH, px, &bi, DIB_RGB_COLORS,
                SRCCOPY);
}

void drawKey(HDC dc, int i) {
  const KeyDef& kd = kKeys[i];
  const bool down = i == g_pressed || (i == g_flash && GetTickCount() < g_flashUntil);
  RECT r = g_keyRect[i];
  COLORREF fill, ink = kText;
  switch (kd.style) {
    case kNum: fill = rgbOf(0xcfd6e0); ink = rgbOf(0x121722); break;
    case kOrange: fill = kAccent; ink = RGB(255, 255, 255); break;
    case kShift: fill = rgbOf(0x263042); ink = kShiftText; break;
    case kAlpha: fill = rgbOf(0x263042); ink = kAlphaText; break;
    case kNav: fill = rgbOf(0x34405a); break;
    default: fill = rgbOf(0x263042); break;
  }
  // shadow, then the key (pressed: lower and lighter)
  RECT sh = r;
  OffsetRect(&sh, 0, 2);
  roundRect(dc, sh, 10, rgbOf(0x05070a), rgbOf(0x05070a));
  if (down) {
    OffsetRect(&r, 0, 2);
    fill = lighten(fill, kd.style == kNum ? -40 : 40);
  }
  roundRect(dc, r, 10, fill, lighten(fill, kd.style == kNum ? -50 : 30));
  text(dc, kd.label, r, ink, kd.style == kNum || kd.style == kOrange ? g_fNum : g_fKey,
       DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  if (kd.style == kNav) return;
  // SHIFT (orange, left) and ALPHA (red, right) functions printed above the key
  RECT lab = {g_keyRect[i].left - 2, g_keyRect[i].top - S(15), g_keyRect[i].right + 2, g_keyRect[i].top - 1};
  if (kd.shift && kd.base) {  // x² x^□ log ln: SHIFT left, BASE-N's number mode right
    text(dc, kd.shift, lab, kShiftText, g_fLabel, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);
    text(dc, kd.base, lab, kBaseText, g_fLabel, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE);
  } else if (kd.shift && kd.alpha) {
    text(dc, kd.shift, lab, kShiftText, g_fLabel, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);
    text(dc, kd.alpha, lab, kAlphaText, g_fLabel, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE);
  } else if (kd.shift) {
    text(dc, kd.shift, lab, kShiftText, g_fLabel, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
  } else if (kd.alpha) {
    text(dc, kd.alpha, lab, kAlphaText, g_fLabel, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
  }
}

// The F1 overlay: tab-separated columns (key, what it is, key, what it is); "#" = a heading.
const wchar_t* const kHelpLines[] = {
    L"#PC KEYBOARD → CALCULATOR KEY",
    L"0-9 . + - * / ^ ( )\tas printed\tEnter\t=",
    L"Backspace\tDEL\tEsc\tAC",
    L"arrow keys\t◀ ▶ ▲ ▼\t[   ]\tSHIFT, ALPHA",
    L"m\tMODE\to\tON",
    L"s   c   t\tsin  cos  tan\tl   n\tlog, ln",
    L"r   q   i\t√   x²   x⁻¹\tf   ~\tfraction, (−)",
    L"E   p\t×10ˣ,  π\th   b\thyp, Abs",
    L"k   w\tRCL, S⇔D\tM   a\tM+, Ans",
    L"!   %   ,\tx!,  %,  comma\tg\tlogₐb",
    L"Tab\tgo to SEND (Type it)\tletters, space\ttype on its keyboard",
    L"F2\tsave a screenshot\tF5   F6\trestart, power",
    L"",
    L"#QUICK TOUR",
    L"MODE 4 = Apps:  ◀ ▶ choose,  = open (or 1-7),  AC back.",
    L"AI SOLVE shows the camera's picture;  = takes the photo.",
    L"SHIFT AC = off,  ON = on,  SHIFT 7 ON while off = exam mode.",
    L"SHIFT MODE = SETUP,  SHIFT ▲ ▼ = brightness.",
    L"SHIFT and ALPHA apply to the next key, like the real calculator.",
    L"",
    L"#F1, Esc or a click closes this help.",
};

void drawHelp(HDC dc) {
  RECT r = {g_bodyRect.left + S(14), g_screenRect.top - S(4), g_bodyRect.right - S(14), g_bodyRect.bottom - S(40)};
  roundRect(dc, r, 16, rgbOf(0x1a2130), kAccent);
  SelectObject(dc, g_fHelp);
  const int x = r.left + S(18);
  int tabs[3] = {S(150), S(320), S(430)};
  int y = r.top + S(16);
  const int lineH = S(25);
  for (const wchar_t* line : kHelpLines) {
    const bool head = line[0] == L'#';
    if (head) ++line;
    SetTextColor(dc, head ? kAccent : kText);
    TabbedTextOutW(dc, x, y, line, lstrlenW(line), 3, tabs, x);
    y += lineH;
  }
}

void paintBody(HDC dc) {
  HBRUSH bg = CreateSolidBrush(kWindowBg);
  FillRect(dc, &g_bodyRect, bg);
  DeleteObject(bg);
  SetBkMode(dc, TRANSPARENT);
  RECT shell = {6, 6, g_bodyW - 6, g_bodyH - 6};
  roundRect(dc, shell, 36, kShell, kShellEdge);
  // brand line above the screen
  RECT head = {g_screenRect.left - 4, S(10), g_screenRect.right + 4, S(38)};
  text(dc, L"PROOFLINE", head, kAccent, g_fHead, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  text(dc, L"v15  ·  SIMULATOR", head, kDim, g_fSmall, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
  RECT bez = g_screenRect;
  InflateRect(&bez, 12, 12);
  roundRect(dc, bez, 18, kBezel, rgbOf(0x1f2633));
  drawScreen(dc);
  // the replay pad
  HBRUSH pb = CreateSolidBrush(rgbOf(0x1a2130));
  HPEN pp = CreatePen(PS_SOLID, 2, rgbOf(0x3a4558));
  HGDIOBJ ob = SelectObject(dc, pb), op = SelectObject(dc, pp);
  Ellipse(dc, g_padRect.left, g_padRect.top, g_padRect.right, g_padRect.bottom);
  SelectObject(dc, ob);
  SelectObject(dc, op);
  DeleteObject(pb);
  DeleteObject(pp);
  for (int i = 0; i < kKeyCount; ++i) drawKey(dc, i);
  RECT foot = {shell.left, g_bodyH - S(32), shell.right, g_bodyH - S(10)};
  text(dc, L"F1: key help   ·   click a key or type on the PC keyboard", foot, kDim, g_fSmall,
       DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  if (g_help) drawHelp(dc);
}

void paint(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC dc = BeginPaint(hwnd, &ps);
  HDC mem = CreateCompatibleDC(dc);
  HBITMAP bmp = CreateCompatibleBitmap(dc, g_clientW, g_clientH);
  HGDIOBJ old = SelectObject(mem, bmp);
  RECT all = {0, 0, g_clientW, g_clientH};
  FillRect(mem, &all, GetSysColorBrush(COLOR_BTNFACE));
  RECT left = {0, 0, g_bodyW + 6, g_clientH};
  HBRUSH bg = CreateSolidBrush(kWindowBg);
  FillRect(mem, &left, bg);
  DeleteObject(bg);
  paintBody(mem);
  BitBlt(dc, 0, 0, g_clientW, g_clientH, mem, 0, 0, SRCCOPY);
  SelectObject(mem, old);
  DeleteObject(bmp);
  DeleteDC(mem);
  EndPaint(hwnd, &ps);
}

// ---------------------------------------------------------------- pictures

std::wstring stamp() {
  std::time_t t = std::time(nullptr);
  std::tm tm{};
  localtime_s(&tm, &t);
  wchar_t b[32];
  wcsftime(b, 32, L"%Y%m%d_%H%M%S", &tm);
  return b;
}

// The panel's frame (undimmed) at `scale`, as a PNG.
bool saveFrame(const std::wstring& path, int scale) {
  const int w = simhost::kW * scale, h = simhost::kH * scale;
  std::vector<uint32_t> px(size_t(w) * h);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      const uint16_t v = simhost::frame[(y / scale) * simhost::kW + x / scale];
      int r = (v >> 11) & 0x1F, g = (v >> 5) & 0x3F, b = v & 0x1F;
      r = (r << 3) | (r >> 2);
      g = (g << 2) | (g >> 4);
      b = (b << 3) | (b >> 2);
      px[size_t(y) * w + x] = uint32_t((r << 16) | (g << 8) | b);
    }
  return wicSavePng(path, px.data(), w, h);
}

// The drawn calculator (no panel) as a PNG.
bool saveBody(const std::wstring& path) {
  HDC screen = GetDC(nullptr);
  HDC mem = CreateCompatibleDC(screen);
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof bi.bmiHeader;
  bi.bmiHeader.biWidth = g_bodyW;
  bi.bmiHeader.biHeight = -g_bodyH;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  void* bits = nullptr;
  HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  HGDIOBJ old = SelectObject(mem, dib);
  paintBody(mem);
  GdiFlush();
  const bool ok = bits && wicSavePng(path, static_cast<const uint32_t*>(bits), g_bodyW, g_bodyH);
  SelectObject(mem, old);
  DeleteObject(dib);
  DeleteDC(mem);
  ReleaseDC(nullptr, screen);
  return ok;
}

std::string narrow(const std::wstring& w) {
  if (w.empty()) return "";
  const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), int(w.size()), nullptr, 0, nullptr, nullptr);
  std::string s(size_t(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), int(w.size()), &s[0], n, nullptr, nullptr);
  return s;
}

std::wstring widen(const std::string& s) {
  if (s.empty()) return L"";
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), nullptr, 0);
  std::wstring w(size_t(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), &w[0], n);
  return w;
}

void takeScreenshot() {
  const fs::path dir = fs::path(g_exeDir) / L"tools" / L"sim_v15" / L"screenshots";
  std::error_code ec;
  fs::create_directories(dir, ec);
  const fs::path file = dir / (L"v15_" + stamp() + L".png");
  if (saveFrame(file.wstring(), g_scale)) g_board.note("[sim] screenshot saved: " + narrow(file.wstring()));
  else g_board.note("[sim] couldn't save the screenshot");
}

// The sample photo, or (if it's missing) a made-up notebook page with "3x+7=22"-like strokes.
std::vector<uint8_t> defaultPhoto(bool& isSample) {
  std::vector<uint8_t> rgb;
  isSample = wicLoadCover((fs::path(g_exeDir) / L"tools" / L"sim_v15" / L"sample_photo.png").wstring(), 320, 240, rgb);
  if (isSample) return rgb;
  rgb.assign(320 * 240 * 3, 0);
  for (int y = 0; y < 240; ++y)
    for (int x = 0; x < 320; ++x) {
      uint8_t* p = &rgb[(size_t(y) * 320 + x) * 3];
      const bool rule = y % 18 == 0, margin = x == 38;
      p[0] = uint8_t(rule ? 150 : margin ? 222 : 236);
      p[1] = uint8_t(rule ? 182 : margin ? 128 : 232);
      p[2] = uint8_t(rule ? 214 : margin ? 128 : 220);
    }
  auto ink = [&](int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; ++y)
      for (int x = x0; x < x0 + w; ++x) {
        uint8_t* p = &rgb[(size_t(y) * 320 + x) * 3];
        p[0] = 28, p[1] = 40, p[2] = 92;
      }
  };
  for (int c = 0; c < 9; ++c) {
    const int x = 70 + c * 22, y = 96;
    ink(x, y, 4, 30);
    ink(x, y + (c % 3) * 12, 14, 4);
    if (c % 2) ink(x + 12, y, 4, 30);
  }
  return rgb;
}

// ---------------------------------------------------------------- the test-controls panel

enum : int {
  ID_BATT = 100, ID_BATT_LBL, ID_USB, ID_WIFI, ID_LINKED, ID_CODE, ID_EXAM, ID_UNLOCK, ID_T1, ID_T10, ID_T12,
  ID_POWER, ID_COLD, ID_FACTORY, ID_REPLY, ID_PHOTO, ID_SHAKY, ID_PHOTO_LBL, ID_SHOT, ID_SCALE, ID_HELP, ID_FOLDER,
  ID_STATUS, ID_LOG, ID_TITLE
};

struct Ctl {
  HWND h;
  int x, y, w, hgt;
};
std::vector<Ctl> g_ctls;

HWND addCtl(const wchar_t* cls, const wchar_t* label, DWORD style, int x, int y, int w, int h, int id,
            DWORD exStyle = 0) {
  x = S(x);
  y = S(y);
  w = S(w);
  h = S(h);
  HWND c = CreateWindowExW(exStyle, cls, label, WS_CHILD | WS_VISIBLE | style, g_panelX + x, y, w, h, g_hwnd,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_inst, nullptr);
  SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(id == ID_TITLE ? g_fUiBold : g_fUi), TRUE);
  g_ctls.push_back({c, x, y, w, h});
  return c;
}

void placeControls() {
  for (const Ctl& c : g_ctls) {
    int h = c.hgt;
    if (GetDlgCtrlID(c.h) == ID_LOG) h = std::max(80, g_clientH - c.y - 10);
    MoveWindow(c.h, g_panelX + c.x, c.y, c.w, h, FALSE);
  }
}

void setBatteryLabel() {
  wchar_t b[64];
  swprintf(b, 64, L"Battery: %d %%%s", g_board.batteryPct, g_board.usb ? L" (charging)" : L"");
  SetDlgItemTextW(g_hwnd, ID_BATT_LBL, b);
}

void createControls() {
  const DWORD btn = BS_PUSHBUTTON | WS_TABSTOP, chk = BS_AUTOCHECKBOX | WS_TABSTOP;
  addCtl(L"STATIC", L"Test controls", 0, 12, 12, 296, 20, ID_TITLE);
  addCtl(L"STATIC", L"", 0, 12, 38, 296, 18, ID_BATT_LBL);
  HWND tb = addCtl(TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_AUTOTICKS | WS_TABSTOP, 6, 56, 308, 30, ID_BATT);
  SendMessageW(tb, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
  SendMessageW(tb, TBM_SETTICFREQ, 10, 0);
  SendMessageW(tb, TBM_SETPOS, TRUE, g_board.batteryPct);
  addCtl(L"BUTTON", L"USB cable plugged in", chk, 12, 90, 296, 20, ID_USB);
  addCtl(L"BUTTON", L"Wi-Fi hotspot reachable", chk, 12, 112, 296, 20, ID_WIFI);
  addCtl(L"BUTTON", L"Linked to an account", chk, 12, 134, 296, 20, ID_LINKED);
  addCtl(L"STATIC", L"Link code:", 0, 30, 162, 70, 20, 0);
  addCtl(L"EDIT", widen(g_board.linkCode).c_str(), ES_UPPERCASE | ES_AUTOHSCROLL | WS_TABSTOP, 104, 158, 110, 24,
         ID_CODE, WS_EX_CLIENTEDGE);
  addCtl(L"BUTTON", L"Start exam mode", btn, 12, 192, 144, 28, ID_EXAM);
  addCtl(L"BUTTON", L"Teacher USB unlock", btn, 164, 192, 144, 28, ID_UNLOCK);
  addCtl(L"STATIC", L"Advance time:", 0, 12, 232, 92, 20, 0);
  addCtl(L"BUTTON", L"+1 min", btn, 106, 226, 60, 28, ID_T1);
  addCtl(L"BUTTON", L"+10 min", btn, 170, 226, 66, 28, ID_T10);
  addCtl(L"BUTTON", L"+12 h", btn, 240, 226, 68, 28, ID_T12);
  addCtl(L"BUTTON", L"Power off / on", btn, 12, 262, 144, 28, ID_POWER);
  addCtl(L"BUTTON", L"Reset (cold boot)", btn, 164, 262, 144, 28, ID_COLD);
  addCtl(L"BUTTON", L"Factory reset (erase saved data)", btn, 12, 296, 296, 28, ID_FACTORY);
  addCtl(L"STATIC", L"AI reply:", 0, 12, 340, 70, 20, 0);
  HWND cb = addCtl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 86, 336, 222, 200, ID_REPLY);
  for (const wchar_t* s : {L"Normal answer", L"Unclear (low confidence)", L"Unreadable / not a problem",
                           L"Timeout", L"Server error", L"AI busy"})
    SendMessageW(cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
  SendMessageW(cb, CB_SETCURSEL, 0, 0);
  addCtl(L"STATIC", L"Camera:", 0, 12, 374, 70, 20, 0);
  addCtl(L"BUTTON", L"Choose photo...", btn, 86, 368, 122, 28, ID_PHOTO);
  addCtl(L"BUTTON", L"Shaky hand", chk, 216, 372, 96, 20, ID_SHAKY);
  addCtl(L"STATIC", g_photoName.c_str(), SS_ENDELLIPSIS, 12, 402, 296, 18, ID_PHOTO_LBL);
  addCtl(L"BUTTON", L"Save screenshot", btn, 12, 428, 144, 28, ID_SHOT);
  addCtl(L"BUTTON", L"Screen size: 2x", btn, 164, 428, 144, 28, ID_SCALE);
  addCtl(L"BUTTON", L"Key help (F1)", btn, 12, 462, 144, 28, ID_HELP);
  addCtl(L"BUTTON", L"Open screenshots", btn, 164, 462, 144, 28, ID_FOLDER);
  addCtl(L"STATIC", L"", 0, 12, 500, 300, 66, ID_STATUS);
  addCtl(L"EDIT", L"", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 12, 572, 300, 140, ID_LOG,
         WS_EX_CLIENTEDGE);
  CheckDlgButton(g_hwnd, ID_USB, g_board.usb ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(g_hwnd, ID_WIFI, g_board.hotspot ? BST_CHECKED : BST_UNCHECKED);
  CheckDlgButton(g_hwnd, ID_LINKED, g_board.linked ? BST_CHECKED : BST_UNCHECKED);
  setBatteryLabel();
  placeControls();
}

void refreshPanel() {
  SetDlgItemTextW(g_hwnd, ID_STATUS, widen(g_board.statusText()).c_str());
  if (g_board.logRevision() == g_shownLog) return;
  g_shownLog = g_board.logRevision();
  std::string all;
  const auto& lines = g_board.log();
  const size_t from = lines.size() > 120 ? lines.size() - 120 : 0;
  for (size_t i = from; i < lines.size(); ++i) all += lines[i] + "\r\n";
  HWND log = GetDlgItem(g_hwnd, ID_LOG);
  SetWindowTextW(log, widen(all).c_str());
  const int len = GetWindowTextLengthW(log);
  SendMessageW(log, EM_SETSEL, len, len);
  SendMessageW(log, EM_SCROLLCARET, 0, 0);
}

void resizeWindow() {
  layout();
  RECT r = {0, 0, g_clientW, g_clientH};
  const DWORD style = DWORD(GetWindowLongW(g_hwnd, GWL_STYLE));
  AdjustWindowRect(&r, style, FALSE);
  SetWindowPos(g_hwnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER);
  placeControls();
  SetDlgItemTextW(g_hwnd, ID_SCALE, g_scale == 2 ? L"Screen size: 2x" : L"Screen size: 3x");
  InvalidateRect(g_hwnd, nullptr, TRUE);
}

void choosePhoto() {
  wchar_t file[MAX_PATH] = L"";
  OPENFILENAMEW ofn = {};
  ofn.lStructSize = sizeof ofn;
  ofn.hwndOwner = g_hwnd;
  ofn.lpstrFilter = L"Pictures\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tif;*.tiff\0All files\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = L"Fake camera: what should the calculator's camera see?";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
  if (!GetOpenFileNameW(&ofn)) return;
  std::vector<uint8_t> rgb;
  if (!wicLoadCover(file, 320, 240, rgb)) {
    MessageBoxW(g_hwnd, L"Windows couldn't read that picture.", L"Choose photo", MB_ICONWARNING);
    return;
  }
  g_board.setPhoto(rgb, false);
  g_photoName = fs::path(file).filename().wstring() + L" (demo answers rotate)";
  SetDlgItemTextW(g_hwnd, ID_PHOTO_LBL, g_photoName.c_str());
  g_board.note("[cam] fake camera now sees " + narrow(fs::path(file).filename().wstring()));
}

void flashKey(DKey k) {
  g_flash = int(k);
  g_flashUntil = GetTickCount() + 130;
}

// A PC character: the calculator key(s) for it, shown pressed.
void typeChar(wchar_t c) {
  if (c >= 128 || c < 32) return;
  DKey ks[2];
  int n = 0;
  if (calc::keysForChar(char(c), ks, n)) flashKey(ks[n - 1]);
  g_board.typeChar(char(c));
}

void pressKey(DKey k) {
  flashKey(k);
  g_board.press(k);
}

void onCommand(int id, int code) {
  switch (id) {
    case ID_USB:
      g_board.usb = IsDlgButtonChecked(g_hwnd, ID_USB) == BST_CHECKED;
      g_board.note(g_board.usb ? "[power] USB cable in" : "[power] USB cable out");
      g_board.batteryChanged();
      setBatteryLabel();
      break;
    case ID_WIFI:
      g_board.hotspot = IsDlgButtonChecked(g_hwnd, ID_WIFI) == BST_CHECKED;
      g_board.note(g_board.hotspot ? "[sim] hotspot reachable" : "[sim] hotspot out of reach");
      break;
    case ID_LINKED:
      g_board.linked = IsDlgButtonChecked(g_hwnd, ID_LINKED) == BST_CHECKED;
      g_board.linkChanged();
      break;
    case ID_CODE:
      if (code == EN_CHANGE) {
        wchar_t b[32] = L"";
        GetDlgItemTextW(g_hwnd, ID_CODE, b, 32);
        const std::string s = narrow(b);
        if (!s.empty() && s != g_board.linkCode) {
          g_board.linkCode = s;
          g_board.linkChanged();
        }
      }
      return;  // keep typing in the box
    case ID_EXAM: g_board.startExam(); break;
    case ID_UNLOCK: g_board.teacherUnlock(); break;
    case ID_T1: g_board.advance(60u * 1000); break;
    case ID_T10: g_board.advance(10u * 60 * 1000); break;
    case ID_T12: g_board.advance(12u * 60 * 60 * 1000); break;
    case ID_POWER: g_board.powerButton(); break;
    case ID_COLD: g_board.coldBoot(); break;
    case ID_FACTORY:
      if (MessageBoxW(g_hwnd, L"Erase the simulator's saved history, notes, memories and settings?", L"Factory reset",
                      MB_YESNO | MB_ICONQUESTION) == IDYES) {
        g_board.factoryReset();
        CheckDlgButton(g_hwnd, ID_LINKED, BST_UNCHECKED);
        SetDlgItemTextW(g_hwnd, ID_CODE, widen(g_board.linkCode).c_str());
      }
      break;
    case ID_REPLY:
      if (code == CBN_SELCHANGE) {
        g_board.reply = ReplyMode(SendDlgItemMessageW(g_hwnd, ID_REPLY, CB_GETCURSEL, 0, 0));
        g_board.note("[sim] AI reply mode changed");
        break;
      }
      return;
    case ID_PHOTO: choosePhoto(); break;
    case ID_SHAKY: g_board.shaky = IsDlgButtonChecked(g_hwnd, ID_SHAKY) == BST_CHECKED; break;
    case ID_SHOT: takeScreenshot(); break;
    case ID_SCALE:
      g_scale = g_scale == 2 ? 3 : 2;
      resizeWindow();
      break;
    case ID_HELP: g_help = !g_help; break;
    case ID_FOLDER: {
      const fs::path dir = fs::path(g_exeDir) / L"tools" / L"sim_v15" / L"screenshots";
      std::error_code ec;
      fs::create_directories(dir, ec);
      ShellExecuteW(g_hwnd, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
      break;
    }
    default: return;
  }
  SetFocus(g_hwnd);  // the keyboard goes back to the calculator
  InvalidateRect(g_hwnd, &g_bodyRect, FALSE);
}

int keyAt(int x, int y) {
  POINT p = {x, y};
  for (int i = 0; i < kKeyCount; ++i)
    if (PtInRect(&g_keyRect[i], p)) return i;
  return -1;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_TIMER: {
      g_board.step();
      const bool flashOver = g_flash >= 0 && GetTickCount() >= g_flashUntil;
      if (flashOver) g_flash = -1;
      if (simhost::flushes != g_shownFlushes || simhost::lightLevel() != g_shownLevel || flashOver) {
        g_shownFlushes = simhost::flushes;
        g_shownLevel = simhost::lightLevel();
        InvalidateRect(hwnd, &g_bodyRect, FALSE);
      }
      if (GetTickCount() - g_statusAt > 250) {
        g_statusAt = GetTickCount();
        refreshPanel();
      }
      return 0;
    }
    case WM_KEYDOWN:
      switch (wp) {
        case VK_RETURN: pressKey(DKey::Eq); break;
        case VK_BACK: pressKey(DKey::Del); break;
        case VK_ESCAPE:
          if (g_help) g_help = false;
          else pressKey(DKey::AC);
          break;
        case VK_UP: pressKey(DKey::Up); break;
        case VK_DOWN: pressKey(DKey::Down); break;
        case VK_LEFT: pressKey(DKey::Left); break;
        case VK_RIGHT: pressKey(DKey::Right); break;
        case VK_TAB: g_board.cursorToSend(); break;
        case VK_F1: g_help = !g_help; break;
        case VK_F2: takeScreenshot(); break;
        case VK_F5: g_board.coldBoot(); break;
        case VK_F6: g_board.powerButton(); break;
        default: return 0;
      }
      InvalidateRect(hwnd, &g_bodyRect, FALSE);
      return 0;
    case WM_CHAR:
      if (wp == '\r' || wp == '\b' || wp == 27 || wp == '\t') return 0;
      typeChar(wchar_t(wp));
      InvalidateRect(hwnd, &g_bodyRect, FALSE);
      return 0;
    case WM_LBUTTONDOWN: {
      SetFocus(hwnd);
      if (g_help) {
        g_help = false;
        InvalidateRect(hwnd, &g_bodyRect, FALSE);
        return 0;
      }
      const int k = keyAt(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
      if (k >= 0) {
        g_pressed = k;
        SetCapture(hwnd);
        g_board.press(kKeys[k].k);
        InvalidateRect(hwnd, &g_bodyRect, FALSE);
      }
      return 0;
    }
    case WM_LBUTTONUP:
      if (g_pressed >= 0) {
        g_pressed = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, &g_bodyRect, FALSE);
      }
      return 0;
    case WM_COMMAND:
      onCommand(LOWORD(wp), HIWORD(wp));
      return 0;
    case WM_HSCROLL:
      if (reinterpret_cast<HWND>(lp) == GetDlgItem(hwnd, ID_BATT)) {
        g_board.batteryPct = int(SendDlgItemMessageW(hwnd, ID_BATT, TBM_GETPOS, 0, 0));
        g_board.batteryChanged();
        setBatteryLabel();
        if (LOWORD(wp) == TB_ENDTRACK) SetFocus(hwnd);
      }
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT:
      paint(hwnd);
      return 0;
    case WM_CLOSE:
      g_board.saveNow();
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------- --selftest

struct SelfTest {
  fs::path dir;
  FILE* log = nullptr;
  int failures = 0, shots = 0;

  void say(const std::string& s) {
    std::printf("%s\n", s.c_str());
    if (log) std::fprintf(log, "%s\n", s.c_str());
  }
  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 15) {
      simhost::advance(15);
      g_board.step();
    }
  }
  void key(DKey k) {
    g_board.press(k);
    run(150);
  }
  void type(const char* s) {
    for (; *s; ++s) {
      g_board.typeChar(*s);
      run(150);
    }
  }
  void shot(const std::string& name) {
    run(1100);  // a whole second on: the blinking cursor shows
    const fs::path f = dir / (name + ".png");
    if (saveFrame(f.wstring(), 2)) {
      ++shots;
      say("  screenshot " + narrow(f.wstring()));
    } else {
      check(false, "save " + name);
    }
  }
  void check(bool ok, const std::string& what) {
    say(std::string(ok ? "  PASS " : "  FAIL ") + what);
    if (!ok) ++failures;
  }
  calc::Device& d() { return g_board.dev(); }
  void toApp(int i) {  // MODE 4, then the carousel to app i
    if (d().view() != calc::View::Apps) {
      key(DKey::AC);
      key(DKey::AC);
      key(DKey::Mode);
      key(DKey::D4);
    }
    for (int g = 0; g < 10 && d().appsSelected() != i; ++g) key(DKey::Right);
  }
};

int runSelfTest(const fs::path& out) {
  SelfTest t;
  t.dir = out;
  std::error_code ec;
  fs::create_directories(out, ec);
  t.log = _wfopen((out / L"selftest_log.txt").wstring().c_str(), L"w");
  simhost::useFakeClock(true);
  // Its own saved-data folder (wiped first), so the user's history and notes are untouched.
  g_board.dataDir = out / L"data";
  fs::remove_all(g_board.dataDir, ec);
  layout();
  bool sample = false;
  const std::vector<uint8_t> photo = defaultPhoto(sample);
  g_board.setPhoto(photo, sample);
  t.say(std::string("Proofline v15 simulator self-test (sample photo ") + (sample ? "found" : "MISSING: synthetic") + ")");
  g_board.boot();
  using calc::View;

  t.say("1. boot");
  t.shot("01_boot");
  t.check(!t.d().isOff() && t.d().view() == View::Calc, "boots into the calculator");

  t.say("2. 2+3=");
  t.type("2+3=");
  t.shot("02_calc_2_plus_3");
  t.check(t.d().resultText() == "5", "2+3= shows 5 (got \"" + t.d().resultText() + "\")");

  t.say("3. MODE, MODE 4");
  t.key(DKey::AC);
  t.key(DKey::Mode);
  t.shot("03_mode_menu");
  t.key(DKey::D4);
  t.shot("04_apps_menu");
  t.check(t.d().view() == View::Apps, "MODE 4 opens the Apps menu");

  t.say("4. every app");
  for (int i = 0; i < calc::Device::kAppCount; ++i) {
    t.toApp(i);
    t.key(DKey::Eq);
    t.run(i == 0 ? 2000 : 300);  // AI Solve: the viewfinder's first frames
    std::string name = calc::Device::appName(i);
    for (char& c : name) c = c == ' ' ? '_' : char(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    t.shot("1" + std::to_string(i) + "_app_" + name);
    t.check(t.d().view() == calc::Device::appView(i), std::string("app opens: ") + calc::Device::appName(i));
    t.key(DKey::AC);
  }

  t.say("5. Type it: 2+2, SEND, the fake tutor's answer");
  t.toApp(1);
  t.key(DKey::Eq);
  t.type("2+2");
  t.shot("20_type_it_2_plus_2");
  g_board.cursorToSend();
  t.shot("21_type_it_on_send");
  g_board.press(DKey::Eq);
  t.run(400);
  // Wi-Fi joins while Type it is open (main.cpp serviceWifi()), so the first SEND solves:
  // it used to find the calculator offline ("NO CONNECTION", = to retry).
  t.check(t.d().ai().screen() != calc::Screen::Message, "the first SEND is not refused (no NO CONNECTION)");
  t.check(t.d().activeRequest() != 0, "SEND starts an AI request");
  t.shot("22_type_it_solving");
  t.run(3500);
  t.shot("23_type_it_answer");
  t.check(t.d().ai().screen() == calc::Screen::Result && t.d().activeRequest() == 0, "the fake reply arrives");
  t.check(!t.d().savedAnswers().empty() && t.d().savedAnswers().front().answer.find('4') != std::string::npos,
          "History has the answer 4 (got \"" +
              (t.d().savedAnswers().empty() ? std::string() : t.d().savedAnswers().front().answer) + "\")");

  t.say("5b. Type it: an equation, worked out by the fake tutor");
  t.key(DKey::AC);
  t.toApp(1);
  t.key(DKey::Eq);
  for (int i = 0; i < 40 && !t.d().keyboardText().empty(); ++i) t.key(DKey::Del);  // the kept draft
  t.type("Solve 3x+7=22 for x");
  g_board.cursorToSend();
  g_board.press(DKey::Eq);
  t.run(400);
  t.check(t.d().activeRequest() != 0, "SEND on the equation starts an AI request at once");
  t.run(3500);
  t.shot("24_type_it_equation_answer");
  t.check(!t.d().savedAnswers().empty() && t.d().savedAnswers().front().answer == "x = 5",
          "SOLVE 3x+7=22 FOR x gives x = 5 (got \"" +
              (t.d().savedAnswers().empty() ? std::string() : t.d().savedAnswers().front().answer) + "\")");
  t.key(DKey::AC);

  t.say("6. AI Solve by photo: viewfinder, =, answer");
  t.toApp(0);
  t.key(DKey::Eq);
  t.run(2500);
  t.shot("30_ai_solve_viewfinder");
  g_board.press(DKey::Eq);
  t.run(300);
  t.shot("31_ai_solve_hold_still");
  t.run(5000);
  t.shot("32_ai_solve_answer");
  t.check(t.d().ai().screen() == calc::Screen::Result, "the photo's answer shows");
  t.check(t.d().savedAnswers().size() >= 2, "History has both answers");

  t.say("7. a failure: server busy");
  g_board.reply = ReplyMode::Busy;
  t.key(DKey::AC);
  t.toApp(1);
  t.key(DKey::Eq);
  for (int i = 0; i < 40 && !t.d().keyboardText().empty(); ++i) t.key(DKey::Del);
  t.type("1+1");
  g_board.cursorToSend();
  g_board.press(DKey::Eq);
  t.run(400);
  if (t.d().activeRequest() == 0) g_board.press(DKey::Eq);  // = retry after NO CONNECTION (see step 5)
  t.run(5000);
  t.shot("33_ai_failure_busy");
  t.check(t.d().ai().screen() == calc::Screen::Message, "busy shows the failure message");
  g_board.reply = ReplyMode::Normal;
  t.key(DKey::AC);
  t.key(DKey::AC);
  t.key(DKey::AC);

  t.say("7b. STAT: MODE 2, 1-VAR, 2 4 4 4 5 5 7 9, SHIFT 1 4 2 = (mean)");
  t.key(DKey::On);
  t.key(DKey::Mode);
  t.key(DKey::D2);
  t.check(t.d().view() == View::Stat, "MODE 2 opens STAT");
  t.key(DKey::D1);
  t.type("2=4=4=4=5=5=7=9=");
  t.shot("34_stat_editor");
  t.check(t.d().stat().rows() == 8, "STAT has 8 rows (got " + std::to_string(t.d().stat().rows()) + ")");
  t.key(DKey::AC);
  t.type("[142=");
  t.shot("35_stat_mean");
  t.check(t.d().stat().resultText() == "5", "the mean is 5 (got \"" + t.d().stat().resultText() + "\")");

  t.say("7c. SOLVE: MODE 1, X^2-2=0, SHIFT CALC, X? 1 =");
  t.key(DKey::Mode);
  t.key(DKey::D1);
  t.type("Xq-2Q0S");
  t.check(t.d().view() == View::SolveRun, "SHIFT CALC opens SOLVE");
  t.type("1=");
  t.shot("36_solve_result");
  t.check(t.d().solveRunner().showingResult() && std::fabs(t.d().solveRunner().root() - std::sqrt(2.0)) < 1e-9,
          "SOLVE gives X = 1.414213562");
  t.key(DKey::AC);
  t.key(DKey::AC);

  t.say("7d. EQN: MODE 5, 3 (aX^2+bX+c=0), 1 (-)3 2, = (X1=2, X2=1), MODE 1");
  t.key(DKey::Mode);
  t.key(DKey::D5);
  t.check(t.d().view() == View::Eqn, "MODE 5 opens EQN");
  t.key(DKey::D3);
  t.type("1=~3=2=");
  t.key(DKey::Eq);
  t.shot("37_eqn_x1");
  t.check(t.d().eqn().screen() == calc::EqnApp::Screen::Result && t.d().eqn().resultText(0) == "2" &&
              t.d().eqn().resultText(1) == "1",
          "EQN solves X^2-3X+2=0: X1=2, X2=1 (got \"" + t.d().eqn().shownText() + "\")");
  t.key(DKey::Mode);
  t.key(DKey::D1);
  t.check(t.d().view() == View::Calc, "MODE 1 leaves EQN");

  t.say("7e. CMPLX: MODE 6, (3+4i)(1-2i) = 11-2i (ENG is i), 3+4i SHIFT 2 3 (r<theta) =, MODE 1");
  t.key(DKey::Mode);
  t.key(DKey::D6);
  t.check(t.d().mode() == calc::Mode::Cmplx && t.d().view() == View::Calc, "MODE 6 opens CMPLX");
  t.type("(3+4j)(1-2j)=");
  t.shot("38_cmplx_result");
  t.check(t.d().resultText() == "11\xE2\x88\x92" "2i", "CMPLX: (3+4i)(1-2i) = 11-2i (got \"" + t.d().resultText() + "\")");
  t.type("3+4j[23=");
  t.shot("39_cmplx_polar");
  t.check(t.d().resultText() == "5\xE2\x88\xA0" "53.13010235",
          "CMPLX: 3+4i in r<theta is 5<53.13010235 (got \"" + t.d().resultText() + "\")");
  t.key(DKey::Mode);
  t.key(DKey::D1);
  t.check(t.d().mode() == calc::Mode::Comp && !t.d().imagVars().any(), "MODE 1 leaves CMPLX (real parts only)");

  t.say("7f. BASE-N: MODE 7, 15x37 = 555, x^ (HEX) 0000022B, log (BIN), 1010 SHIFT 3 1 (and) 1100 =, MODE 1");
  t.key(DKey::Mode);
  t.key(DKey::D7);
  t.check(t.d().mode() == calc::Mode::BaseN && t.d().view() == View::Calc, "MODE 7 opens BASE-N");
  t.type("15*37=");
  t.check(t.d().resultText() == "555", "BASE-N: 15x37 = 555 (got \"" + t.d().resultText() + "\")");
  t.key(DKey::Pow);
  t.shot("39b_basen_hex");
  t.check(t.d().resultText() == "0000022B", "BASE-N: HEX shows 0000022B (got \"" + t.d().resultText() + "\")");
  t.key(DKey::Log);
  t.type("1010[311100=");
  t.shot("39c_basen_bin_and");
  t.check(t.d().resultText() == "0000000000001000",
          "BASE-N: 1010 and 1100 = 0000000000001000 (got \"" + t.d().resultText() + "\")");
  t.key(DKey::Mode);
  t.key(DKey::D1);
  t.check(t.d().mode() == calc::Mode::Comp, "MODE 1 leaves BASE-N");

  t.say("7g. MATRIX: MODE 8, Dim MatA 2x2 = [[2,1],[1,1]], MatA x MatA (MatAns), det(MatA) = 1, MODE 9 VctA.VctB, MODE 1");
  t.key(DKey::Mode);
  t.key(DKey::D8);
  t.check(t.d().mode() == calc::Mode::Matrix && t.d().view() == View::MatVec, "MODE 8 opens MATRIX (Dim's list)");
  t.type("152=1=1=1=$");  // MatA, 2x2, the elements
  t.type("[43*[43=");
  t.shot("39d_matrix_matans");
  t.check(t.d().view() == View::MatVec && t.d().matVec().editing() == 3 && t.d().matVec().var(3).at(0, 0).v == 5,
          "MATRIX: MatA x MatA = [[5,3],[3,2]] on the MatAns screen");
  t.type("$[47[43=");
  t.check(t.d().resultText() == "1", "MATRIX: det(MatA) = 1 (got \"" + t.d().resultText() + "\")");
  t.key(DKey::Mode);
  t.key(DKey::D9);
  t.type("121=2=$[51223=4=$[53[57[54=");  // VctA (1,2), VctB (3,4), VctA.VctB
  t.shot("39e_vector_dot");
  t.check(t.d().mode() == calc::Mode::Vector && t.d().resultText() == "11",
          "VECTOR: (1,2).(3,4) = 11 (got \"" + t.d().resultText() + "\")");
  t.key(DKey::Mode);
  t.key(DKey::D1);
  t.check(t.d().mode() == calc::Mode::Comp, "MODE 1 leaves VECTOR");

  t.say("8. exam mode, then the apps are off");
  g_board.startExam();
  t.run(500);
  t.shot("40_exam_mode_on");
  t.check(t.d().examActive(), "SHIFT 7 ON while off starts exam mode");
  t.key(DKey::AC);
  t.key(DKey::Mode);
  t.shot("41_exam_mode_menu");
  t.key(DKey::D4);
  t.shot("42_exam_apps_off");
  t.check(t.d().view() != View::Apps, "MODE 4 opens nothing in exam mode");
  t.key(DKey::AC);
  t.key(DKey::AC);

  t.say("9. +12 h: exam mode ends itself");
  g_board.advance(12u * 60 * 60 * 1000);
  t.run(300);
  t.key(DKey::On);
  t.shot("43_exam_after_12h");
  t.check(!t.d().examActive(), "exam mode over after 12 h");

  t.say("10. cold boot keeps History");
  const size_t before = t.d().savedAnswers().size();
  g_board.coldBoot();
  t.run(500);
  t.check(t.d().savedAnswers().size() == before,
          "History survives a restart (" + std::to_string(t.d().savedAnswers().size()) + " of " +
              std::to_string(before) + ")");
  t.shot("50_after_cold_boot");

  t.say("11. the window's calculator");
  const fs::path body = out / L"60_window_calculator.png";
  t.check(saveBody(body.wstring()), "draws the calculator body");
  t.say("  screenshot " + narrow(body.wstring()));

  t.say(std::to_string(t.shots) + " screenshots, " + std::to_string(t.failures) + " failure(s)");
  t.say("Board log:");
  for (const std::string& l : g_board.log()) t.say("  " + l);
  if (t.log) std::fclose(t.log);
  return t.failures == 0 ? 0 : 1;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
  g_inst = inst;
  // Crisp on scaled displays: real pixels, with the chrome scaled by the display's DPI.
  SetProcessDPIAware();
  HDC sdc = GetDC(nullptr);
  g_ui = std::max(1.0, GetDeviceCaps(sdc, LOGPIXELSY) / 96.0);
  ReleaseDC(nullptr, sdc);
  for (int i = 0; i < kKeyCount; ++i)
    if (kKeys[i].k != DKey(i)) return 3;  // the key table must follow core/keys.h
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  wchar_t exe[MAX_PATH];
  GetModuleFileNameW(nullptr, exe, MAX_PATH);
  g_exeDir = fs::path(exe).parent_path().wstring();

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argc >= 2 && lstrcmpiW(argv[1], L"--selftest") == 0) {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
      FILE* f = nullptr;
      freopen_s(&f, "CONOUT$", "w", stdout);
    }
    const fs::path out = argc >= 3 ? fs::path(argv[2]) : fs::path(g_exeDir) / L"tools" / L"sim_v15" / L"selftest_out";
    const int rc = runSelfTest(out);
    LocalFree(argv);
    CoUninitialize();
    return rc;
  }
  // --window-shot <file.png>: opens the window (with a throw-away data folder), lets it run
  // for 2 s, saves a picture of the whole window and closes (for checking the layout).
  std::wstring windowShot;
  std::string windowKeys;  // optional: keys typed first ("?" = the help overlay)
  if (argc >= 3 && lstrcmpiW(argv[1], L"--window-shot") == 0) windowShot = argv[2];
  if (argc >= 4 && !windowShot.empty()) windowKeys = narrow(argv[3]);
  LocalFree(argv);

  INITCOMMONCONTROLSEX icc = {sizeof icc, ICC_BAR_CLASSES | ICC_STANDARD_CLASSES};
  InitCommonControlsEx(&icc);
  const wchar_t* appdata = _wgetenv(L"APPDATA");
  g_board.dataDir = fs::path(appdata ? appdata : g_exeDir.c_str()) / L"ProoflineSim";
  if (!windowShot.empty()) {
    g_board.dataDir = fs::path(windowShot).parent_path() / L"data_window_shot";
    std::error_code ec;
    fs::remove_all(g_board.dataDir, ec);
  }
  bool sample = false;
  const std::vector<uint8_t> photo = defaultPhoto(sample);
  g_board.setPhoto(photo, sample);
  if (!sample) g_photoName = L"(sample_photo.png missing: a drawn page)";
  g_board.boot();
  g_fUi = font(S(13), FW_NORMAL, L"Segoe UI");
  g_fUiBold = font(S(16), FW_BOLD, L"Segoe UI");
  layout();

  WNDCLASSW wc = {};
  wc.lpfnWndProc = wndProc;
  wc.hInstance = inst;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wc.lpszClassName = L"ProoflineSimV15";
  RegisterClassW(&wc);
  const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
  RECT r = {0, 0, g_clientW, g_clientH};
  AdjustWindowRect(&r, style, FALSE);
  g_hwnd = CreateWindowW(L"ProoflineSimV15", L"Proofline v15 simulator", style, CW_USEDEFAULT, CW_USEDEFAULT,
                         r.right - r.left, r.bottom - r.top, nullptr, nullptr, inst, nullptr);
  createControls();
  ShowWindow(g_hwnd, show);
  SetFocus(g_hwnd);
  SetTimer(g_hwnd, 1, 15, nullptr);
  MSG m;
  if (!windowShot.empty()) {
    for (char c : windowKeys) {
      if (c == '?') g_help = !g_help;
      else g_board.typeChar(c);
      for (int i = 0; i < 10; ++i) {
        Sleep(15);
        g_board.step();
      }
    }
    for (const DWORD t0 = GetTickCount(); GetTickCount() - t0 < 2500;) {
      while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
      }
      Sleep(10);
    }
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    HDC wdc = GetDC(g_hwnd);
    HDC mem = CreateCompatibleDC(wdc);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = rc.right;
    bi.bmiHeader.biHeight = -rc.bottom;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(wdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ old = SelectObject(mem, dib);
    PrintWindow(g_hwnd, mem, PW_CLIENTONLY | 0x2 /* PW_RENDERFULLCONTENT */);
    GdiFlush();
    const bool ok = bits && wicSavePng(windowShot, static_cast<const uint32_t*>(bits), rc.right, rc.bottom);
    SelectObject(mem, old);
    DeleteObject(dib);
    DeleteDC(mem);
    ReleaseDC(g_hwnd, wdc);
    DestroyWindow(g_hwnd);
    CoUninitialize();
    return ok ? 0 : 1;
  }
  while (GetMessageW(&m, nullptr, 0, 0)) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  CoUninitialize();
  return 0;
}
