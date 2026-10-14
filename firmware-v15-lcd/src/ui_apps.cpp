// The Apps menu and the small apps (Type it, History, Notes, Settings, Link, Confirm) in
// the colour look. Everything is drawn from the Device's public state; the core's own
// mono render of the same screen is only used for the hash (ui.cpp) and a title.
#include <algorithm>
#include <cmath>
#include <cstring>

#include "text.h"
#include "ui_draw.h"

using calc::Device;
using calc::KeyCell;
using calc::View;

namespace ui {

std::string geoTitle(const Device& dev, std::string& right);       // ui_geo.cpp
std::string periodicTitle(const Device& dev, std::string& right);  // ui_periodic.cpp

namespace {

constexpr uint32_t kBlinkMs = 500;

LGFX_Sprite& S() { return lcd::canvas(); }

std::string upper(std::string s) {
  for (char& c : s)
    if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
  return s;
}

// "x/y" for the status bar.
std::string ofText(int a, int b) { return std::to_string(a) + "/" + std::to_string(b); }

std::string stripScheme(std::string url) {
  for (const char* scheme : {"https://", "http://"})
    if (url.rfind(scheme, 0) == 0) url.erase(0, std::strlen(scheme));
  while (!url.empty() && url.back() == '/') url.pop_back();
  return url;
}

// Centred lines, wrapped at `cols`, from y; returns the y after them.
int centredWrapped(const std::string& text, int cols, int y, int scale, uint16_t color, int pitch) {
  for (const std::string& l : calc::wrapText(text, cols)) {
    gd::textCentered(S(), kW / 2, y, l, scale, color);
    y += pitch;
  }
  return y;
}

}  // namespace

// ---------------------------------------------------------------- shared list

void drawList(const std::vector<ListRow>& rows, int selected, int top, int visible) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const int n = int(rows.size());
  const bool bar = n > visible;
  const int x0 = 6, w = kW - 12 - (bar ? 6 : 0);
  for (int r = 0; r < visible && top + r < n; ++r) {
    const int i = top + r;
    const ListRow& row = rows[size_t(i)];
    const int y = kListY + r * kListPitch;
    const bool sel = i == selected;
    s.fillRoundRect(x0, y, w, kListPitch - 2, 5, sel ? p.accent : p.panel);
    int x = x0 + 8;
    if (row.swatch >= 0) {
      s.fillRoundRect(x, y + 5, 10, 12, 2, uint16_t(row.swatch));
      if (sel) s.drawRoundRect(x, y + 5, 10, 12, 2, p.onAccent);
      x += 16;
    }
    const uint16_t lc = sel ? p.onAccent : row.leftColor ? row.leftColor : p.text;
    const uint16_t rc = sel ? p.onAccent : row.rightColor ? row.rightColor : p.dim;
    int rightW = 0;
    if (!row.right.empty()) {
      rightW = gd::width(row.right, kTextScale, gd::tight(kTextScale));
      gd::text(s, x0 + w - 8 - rightW, y + 4, row.right, kTextScale, rc, gd::tight(kTextScale));
      rightW += 10;
    }
    gd::textFit(s, x, y + 4, row.left, kTextScale, lc, x0 + w - 8 - rightW - x);
  }
  if (bar) scrollBar(kListY, visible * kListPitch - 2, top, visible, n);
}

// ---------------------------------------------------------------- titles

std::string appTitle(const Device& dev, std::string& right) {
  right.clear();
  switch (dev.view()) {
    case View::Apps: right = ofText(dev.appsSelected() + 1, Device::kAppCount); return "APPS";
    case View::Keyboard: {
      right = ofText(calc::textLength(dev.keyboardText()), Device::kMaxText);
      if (!dev.keyboardForNote()) return "TYPE IT";
      // NEW NOTE / EDIT NOTE: the core's title row says which.
      const std::string t = coreRowText(1);
      return t.rfind("EDIT", 0) == 0 ? "EDIT NOTE" : "NEW NOTE";
    }
    case View::History:
      if (!dev.savedAnswers().empty()) right = ofText(int(dev.savedAnswers().size()), Device::kMaxHistory);
      return "HISTORY";
    case View::Notes: right = ofText(int(dev.notes().size()), Device::kMaxNotes); return "NOTES";
    case View::NoteView: return "NOTE " + std::to_string(dev.noteOpen() + 1);
    case View::Settings: return "SETTINGS";
    case View::Link: return "LINK CALCULATOR";
    case View::Confirm: {
      const std::string q = dev.confirmQuestion();
      if (q.find("history") != std::string::npos) return "HISTORY";
      if (q.find("note") != std::string::npos || q.find("changes") != std::string::npos) return "NOTES";
      return "TYPE IT";
    }
    case View::Geo: return geoTitle(dev, right);
    case View::Periodic: return periodicTitle(dev, right);
    default: return "";
  }
}

// ---------------------------------------------------------------- icons

void appIcon(int app, int cx, int cy, int size) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const uint16_t ink = color::text, acc = p.accent, hole = p.panel;
  const float u = size / 48.0f;  // the icons are designed in a 48 x 48 box
  auto X = [&](float v) { return cx + int(std::lround(v * u)); };
  auto Y = [&](float v) { return cy + int(std::lround(v * u)); };
  auto L = [&](float v) { return int(std::lround(v * u)); };
  switch (app) {
    case 0: {  // AI Solve: a camera with an orange lens
      s.fillRoundRect(X(-9), Y(-17), L(18), L(9), L(3), ink);
      s.fillRoundRect(X(-21), Y(-12), L(42), L(29), L(6), ink);
      s.fillCircle(X(0), Y(2), L(11), hole);
      s.fillCircle(X(0), Y(2), L(8), acc);
      s.fillCircle(X(-3), Y(-1), L(2.5f), ink);
      s.fillRoundRect(X(12), Y(-8), L(5), L(3), 1, acc);
      break;
    }
    case 1: {  // Type it: a keyboard
      s.fillRoundRect(X(-23), Y(-14), L(46), L(28), L(4), ink);
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 6; ++c) {
          if (r == 2 && c >= 1 && c <= 4) continue;  // the space bar
          s.fillRoundRect(X(-19 + c * 6.6f), Y(float(-10 + r * 7)), L(5), L(5), 1, hole);
        }
      s.fillRoundRect(X(-12.4f), Y(4), L(25), L(5), 1, acc);
      break;
    }
    case 2: {  // History: a clock with an orange hand
      s.fillCircle(X(0), Y(0), L(21), ink);
      s.fillCircle(X(0), Y(0), L(17), hole);
      for (int k = 0; k < 12; ++k) {
        const float a = k * 3.14159265f / 6;
        const float r0 = k % 3 == 0 ? 12.5f : 14.5f;
        s.drawWideLine(X(std::sin(a) * r0), Y(-std::cos(a) * r0), X(std::sin(a) * 15.5f), Y(-std::cos(a) * 15.5f),
                       k % 3 == 0 ? 1.3f * u : 0.7f * u, ink);
      }
      s.drawWideLine(X(0), Y(0), X(0), Y(-10), 1.6f * u, ink);
      s.drawWideLine(X(0), Y(0), X(8), Y(4), 1.6f * u, acc);
      s.fillCircle(X(0), Y(0), L(2.5f), acc);
      break;
    }
    case 3: {  // Notes: two pages, the front one lined
      s.fillRoundRect(X(-10), Y(-21), L(30), L(38), L(3), p.dim);
      s.fillRoundRect(X(-19), Y(-16), L(30), L(38), L(3), ink);
      s.fillTriangle(X(11), Y(22), X(11), Y(12), X(1), Y(22), hole);  // the folded corner
      s.fillTriangle(X(11), Y(12), X(1), Y(12), X(1), Y(22), p.dim);
      s.fillRoundRect(X(-14), Y(-10), L(20), L(3), 1, acc);
      for (int i = 0; i < 3; ++i) s.fillRoundRect(X(-14), Y(float(-3 + i * 6)), L(i == 2 ? 12.0f : 20.0f), L(2), 1, hole);
      break;
    }
    case 4: {  // Geography: a globe
      const uint16_t sea = rgb565(0x2f, 0x6f, 0xb3), land = rgb565(0x7f, 0xb7, 0x7e);
      s.fillCircle(X(0), Y(0), L(21), sea);
      s.fillEllipse(X(-8), Y(-7), L(8), L(6), land);
      s.fillEllipse(X(-4), Y(7), L(5), L(8), land);
      s.fillEllipse(X(10), Y(-3), L(7), L(9), land);
      s.drawEllipse(X(0), Y(0), L(9), L(21), ink);
      s.drawFastHLine(X(-21), Y(0), L(42), ink);
      s.drawFastHLine(X(-18), Y(-10), L(36), ink);
      s.drawFastHLine(X(-18), Y(10), L(36), ink);
      s.drawCircle(X(0), Y(0), L(21), ink);
      s.drawCircle(X(0), Y(0), L(21) - 1, ink);
      s.fillCircle(X(10), Y(-3), L(2.5f), acc);  // a capital
      break;
    }
    case 5: {  // Periodic Table: an element tile
      s.fillRoundRect(X(-21), Y(-21), L(42), L(42), L(6), acc);
      gd::text(s, X(-17), Y(-17), "26", 1, p.onAccent);
      gd::textCentered(s, X(0) + 1, Y(-8), "Fe", size >= 40 ? 3 : 2, p.onAccent, gd::advance(3));
      s.fillRoundRect(X(-14), Y(13), L(28), L(3), 1, p.onAccent);
      break;
    }
    default: {  // Settings: a gear
      for (int k = 0; k < 8; ++k) {
        const float a = k * 3.14159265f / 4;
        s.drawWideLine(X(std::cos(a) * 12), Y(std::sin(a) * 12), X(std::cos(a) * 19), Y(std::sin(a) * 19), 3.6f * u,
                       ink);
      }
      s.fillCircle(X(0), Y(0), L(15), ink);
      s.fillCircle(X(0), Y(0), L(7), hole);
      s.drawCircle(X(0), Y(0), L(7), acc);
      s.drawCircle(X(0), Y(0), L(7) + 1, acc);
      break;
    }
  }
}

// ---------------------------------------------------------------- Apps (carousel)

void drawAppsMenu(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const int sel = f.dev.appsSelected();
  // The icon on its tile.
  const int tw = 76, th = 60, tx = (kW - tw) / 2, ty = kContentY + 4;
  s.fillRoundRect(tx, ty, tw, th, 12, p.panel);
  s.drawRoundRect(tx, ty, tw, th, 12, p.panel2);
  appIcon(sel, kW / 2, ty + th / 2, 46);
  // ◀ ▶ either side: the carousel wraps.
  gd::glyph(s, 14, ty + th / 2 - 7, calc::findGlyph(0x25C0), kTextScale, p.dim);
  gd::glyph(s, kW - 24, ty + th / 2 - 7, calc::findGlyph(0x25B6), kTextScale, p.dim);
  // Name (3x when it fits), description.
  const std::string name = Device::appName(sel);
  const int nameY = ty + th + 6;
  if (gd::width(name, 3) <= kW - 20) gd::textCentered(s, kW / 2, nameY, name, 3, p.text);
  else gd::textCentered(s, kW / 2, nameY + 4, name, kTextScale, p.text);
  const std::string desc = Device::appDescription(sel);
  const int descY = nameY + 26;
  if (gd::width(desc, kTextScale) <= kW - 16) gd::textCentered(s, kW / 2, descY, desc, kTextScale, p.dim);
  else gd::textCentered(s, kW / 2, descY, desc, kTextScale, p.dim, gd::tight(kTextScale));
  // Page dots: the current app a short orange bar.
  const int dot = 5, gap = 6, wide = 14;
  const int total = (Device::kAppCount - 1) * (dot + gap) + wide;
  int x = (kW - total) / 2;
  const int y = kContentBottom - 8;
  for (int i = 0; i < Device::kAppCount; ++i) {
    if (i == sel) {
      s.fillRoundRect(x, y, wide, dot, 2, p.accent);
      x += wide + gap;
    } else {
      s.fillRoundRect(x, y, dot, dot, 2, p.panel2);
      x += dot + gap;
    }
  }
  softKeys({{"\xE2\x97\x80\xE2\x96\xB6", "Move"}, {"=", "Open"}, {"AC", "Back"}});
}

// ---------------------------------------------------------------- Type it / notes keyboard

void drawKeyboard(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const Device& dev = f.dev;
  // The text box: the last three lines, hard-wrapped, the cursor after the last character.
  constexpr int kCols = 26, kLines = 3;
  const int bx = 6, by = kContentY + 2, bw = kW - 12, bh = 3 * 16 + 6;
  s.fillRoundRect(bx, by, bw, bh, 6, p.panel);
  const std::vector<uint32_t> cps = calc::decodeUtf8(dev.keyboardText());
  const int n = int(cps.size());
  const int cursorLine = n / kCols;
  const int first = cursorLine >= kLines ? cursorLine - (kLines - 1) : 0;
  const int adv = gd::advance(kTextScale);
  const int tx = bx + 8;
  for (int l = first; l < first + kLines; ++l) {
    const int a = l * kCols;
    if (a >= n) break;
    const int b = a + kCols < n ? a + kCols : n;
    gd::text(s, tx, by + 4 + (l - first) * 16, calc::encodeUtf8(std::vector<uint32_t>(cps.begin() + a, cps.begin() + b)),
             kTextScale, p.text);
  }
  if (n == 0)
    gd::text(s, tx + 6, by + 4, dev.keyboardForNote() ? "Write a note..." : "Type a problem...", kTextScale, p.dim);
  if ((f.nowMs / kBlinkMs) % 2 == 0) {
    const int cx = tx + (n % kCols) * adv - 2, cy = by + 3 + (cursorLine - first) * 16;
    s.fillRect(cx < bx + 2 ? bx + 2 : cx, cy, 2, 16, p.accent);
  }
  if (first > 0) gd::glyph(s, bx + bw - 14, by + 3, calc::findGlyph(0x25B2), 1, p.dim);  // more above
  // The keys: 25 columns of 12 px, three rows.
  constexpr int kPitchX = 12, kPitchY = 20, kCellH = 18;
  const int gx = (kW - 25 * kPitchX) / 2 + 1, gy = by + bh + 5;
  const uint16_t delFill = rgb565(0x45, 0x26, 0x2c), delText = rgb565(0xff, 0xa3, 0xa3);
  const uint16_t sendFill = rgb565(0x4d, 0x2c, 0x1f);
  for (int i = 0; i < Device::keyboardCellCount(); ++i) {
    const KeyCell& c = Device::keyboardCellAt(i);
    const bool sel = i == dev.keyboardCell();
    const int x = gx + c.col * kPitchX, y = gy + c.row * kPitchY, w = c.width * kPitchX - 1;
    uint16_t fill = p.panel2, ink = p.text;
    if (c.kind == KeyCell::Erase) fill = delFill, ink = delText;
    if (c.kind == KeyCell::Send) fill = sendFill, ink = p.accent;
    if (sel) fill = p.accent, ink = p.onAccent;
    s.fillRoundRect(x, y, w, kCellH, 3, fill);
    if (c.kind == KeyCell::Send && !sel) s.drawRoundRect(x, y, w, kCellH, 3, p.accent);
    const std::string label = c.kind == KeyCell::Send && dev.keyboardForNote() ? "SAVE" : c.label;
    const int lw = gd::width(label, kTextScale, gd::tight(kTextScale)) - 2;  // ink width (no trailing gap)
    gd::text(s, x + (w - lw) / 2, y + 2, label, kTextScale, ink, gd::tight(kTextScale));
  }
  const KeyCell& cur = Device::keyboardCellAt(dev.keyboardCell());
  const char* what = cur.kind == KeyCell::Send ? (dev.keyboardForNote() ? "Save" : "Send")
                     : cur.kind == KeyCell::Erase ? "Erase" : "Type";
  softKeys({{"=", what}, {"DEL", "Erase"}, {"AC", "Back"}});
}

// ---------------------------------------------------------------- History

void drawHistory(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const auto& saved = f.dev.savedAnswers();
  if (saved.empty()) {
    appIcon(2, kW / 2, kContentY + 34, 40);
    gd::textCentered(s, kW / 2, kContentY + 66, "No answers yet.", kTextScale, p.text);
    // The explanation sits where the toast goes: left out while one shows ("History
    // cleared"), so the two never overlap.
    if (!f.dev.toastVisible())
      centredWrapped("Answers from AI Solve and Type it are kept here.", 26, kContentY + 88, kTextScale, p.dim, 18);
    softKeys({{"AC", "Back"}});
    return;
  }
  std::vector<ListRow> rows;
  for (size_t i = 0; i < saved.size(); ++i) {
    ListRow r;
    r.left = saved[i].answer;
    r.right = std::to_string(i + 1);
    rows.push_back(r);
  }
  ListRow clear;
  clear.left = "Clear history";
  clear.leftColor = rgb565(0xff, 0x8a, 0x8a);
  rows.push_back(clear);
  drawList(rows, f.dev.historySelected(), f.dev.listTop());
  const bool onClear = f.dev.historySelected() == int(saved.size());
  softKeys({{"\xE2\x96\xB2\xE2\x96\xBC", "Move"}, {"=", onClear ? "Clear" : "Open"}, {"AC", "Back"}});
}

// ---------------------------------------------------------------- Notes

void drawNotes(const Frame& f) {
  const Palette& p = pal();
  std::vector<ListRow> rows;
  ListRow add;
  const bool full = int(f.dev.notes().size()) >= Device::kMaxNotes;
  add.left = full ? "Notes full (10)" : "+ New note";
  add.leftColor = full ? p.dim : p.accent;
  rows.push_back(add);
  for (const std::string& n : f.dev.notes()) {
    ListRow r;
    r.left = calc::wrapText(n, 200)[0];  // the first line (a typed note has no line breaks)
    r.right = std::to_string(calc::textLength(n));
    rows.push_back(r);
  }
  drawList(rows, f.dev.notesSelected(), f.dev.listTop());
  softKeys({{"\xE2\x96\xB2\xE2\x96\xBC", "Move"}, {"=", f.dev.notesSelected() == 0 ? "New" : "Open"}, {"AC", "Back"}});
}

void drawNoteView(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const int i = f.dev.noteOpen();
  if (i < 0 || i >= int(f.dev.notes().size())) {
    softKeys({{"AC", "Back"}});
    return;
  }
  // The core's wrapping (25 columns, its scroll counts these lines); 6 lines show.
  const std::vector<std::string> lines = calc::wrapText(f.dev.notes()[size_t(i)], 25);
  const int top = f.dev.noteScroll(), visible = 6;
  const int bx = 6, by = kContentY + 2, bw = kW - 12, bh = kContentBottom - by - 1;
  s.fillRoundRect(bx, by, bw, bh, 6, p.panel);
  s.fillRect(bx + 8, by, 3, bh, p.accent);  // a margin rule, like a notebook
  for (int r = 0; r < visible && top + r < int(lines.size()); ++r)
    gd::text(s, bx + 18, by + 5 + r * 20, lines[size_t(top + r)], kTextScale, p.text, gd::tight(kTextScale) + 1);
  scrollBar(by + 4, bh - 8, top, visible > int(lines.size()) - top ? int(lines.size()) - top : visible,
            int(lines.size()));
  softKeys({{"1", "Edit"}, {"2", "Delete"}, {"AC", "Back"}});
}

// ---------------------------------------------------------------- Settings

void drawSettings(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const Device& dev = f.dev;
  const int sel = dev.settingsSelected();
  std::vector<ListRow> rows;
  for (int i = 0; i < Device::settingsCount(); ++i) {
    ListRow r;
    r.left = std::to_string(i + 1) + " " + Device::settingsItem(i);
    switch (i) {
      case 0: r.right = std::to_string(dev.brightness()) + "%"; break;
      case 1: r.right = std::to_string(dev.sleepMinutes()) + " min"; break;
      case 2: r.right = f.st.wifiConnected ? "Connected" : ">"; break;
      case 3: r.right = dev.linked() ? "Linked" : !dev.linkCode().empty() ? dev.linkCode() : ">"; break;
      case 4: r.right = dev.dotMatrix() ? "LCD dots" : "Plain"; break;
      case 5: r.right = dev.examActive() ? "On" : "Off"; break;
      case 6:
        r.right = f.st.battery < 0 ? "USB" : std::to_string(f.st.battery > 100 ? 100 : f.st.battery) + "%" +
                                                 (f.st.charging ? "+" : "");
        break;
      default: r.right = ">"; break;  // About: the version is on its page
    }
    if ((i == 0 || i == 1) && i == sel) r.right = "\xE2\x97\x80" + r.right + "\xE2\x96\xB6";
    rows.push_back(r);
  }
  const int top = dev.listTop();
  drawList(rows, sel, top);
  // Brightness: a bar of ten steps in its row (when the row is on screen).
  if (top == 0) {
    const int y = kListY + 6, segW = 4, segGap = 1;
    const int bx = 160;
    const bool on = sel == 0;
    for (int k = 0; k < 10; ++k) {
      const bool lit = (k + 1) * 10 <= dev.brightness();
      s.fillRect(bx + k * (segW + segGap), y + (9 - k), segW, 1 + k, lit ? (on ? p.onAccent : p.accent) : (on ? p.panel : p.panel2));
    }
  }
  if (sel <= 1) softKeys({{"\xE2\x97\x80\xE2\x96\xB6", "Change"}, {"\xE2\x96\xB2\xE2\x96\xBC", "Move"}, {"AC", "Back"}});
  else softKeys({{"=", "Open"}, {"\xE2\x96\xB2\xE2\x96\xBC", "Move"}, {"AC", "Back"}});
}

// ---------------------------------------------------------------- Confirm

void drawConfirm(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const int bx = 24, by = kContentY + 8, bw = kW - 48, bh = 104;
  s.fillRoundRect(bx, by, bw, bh, 10, p.panel);
  s.drawRoundRect(bx, by, bw, bh, 10, p.accent);
  // A "?" badge, then the question.
  s.fillCircle(kW / 2, by + 18, 11, p.accent);
  gd::textCentered(s, kW / 2 + 1, by + 11, "?", kTextScale, p.onAccent);
  const std::string q = f.dev.confirmQuestion();
  if (gd::width(q, 3) <= bw - 16) gd::textCentered(s, kW / 2, by + 36, q, 3, p.text);
  else gd::textCentered(s, kW / 2, by + 40, q, kTextScale, p.text);
  // = Yes (filled), AC No (outlined).
  const int btnW = 96, btnH = 26, gap = 20, y = by + bh - btnH - 12;
  const int x1 = kW / 2 - gap / 2 - btnW, x2 = kW / 2 + gap / 2;
  s.fillRoundRect(x1, y, btnW, btnH, 6, p.accent);
  gd::textCentered(s, x1 + btnW / 2, y + 6, "= Yes", kTextScale, p.onAccent);
  s.drawRoundRect(x2, y, btnW, btnH, 6, p.dim);
  gd::textCentered(s, x2 + btnW / 2, y + 6, "AC No", kTextScale, p.text);
  softKeys({{"=", "Yes"}, {"AC", "No"}});
}

// ---------------------------------------------------------------- Link calculator

void drawLink(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const Device& dev = f.dev;
  if (dev.linkCode().empty()) {
    const int cy = kContentY + 36;
    if (dev.linked()) {
      const int ky = kContentY + 24;
      s.fillCircle(kW / 2, ky, 18, p.good);
      s.drawWideLine(kW / 2 - 9, ky + 1, kW / 2 - 2, ky + 8, 2.2f, p.bg);
      s.drawWideLine(kW / 2 - 2, ky + 8, kW / 2 + 10, ky - 7, 2.2f, p.bg);
      gd::textCentered(s, kW / 2, ky + 26, "Already linked", 3, p.text);
      if (dev.linkPlan().empty()) {
        centredWrapped("This calculator is on your account.", 26, ky + 54, kTextScale, p.dim, 18);
      } else {  // the plan state from the AI server, in a pill, instead of the sentence
        // 2 x 27 columns fit "Free month over: subscribe at <host>" (planStatusText).
        const std::vector<std::string> lines = calc::wrapText(dev.linkPlan(), 27);
        int w = 0;
        for (const std::string& l : lines) w = std::max(w, gd::width(l, kTextScale));
        const int n = std::min(int(lines.size()), 2);  // two lines fit above the soft keys
        const int bw = w + 20, bh = n * 18 + 8, by = ky + 50;
        s.fillRoundRect((kW - bw) / 2, by, bw, bh, 6, p.panel);
        s.drawRoundRect((kW - bw) / 2, by, bw, bh, 6, p.accent);
        for (int i = 0; i < n; ++i) gd::textCentered(s, kW / 2, by + 5 + i * 18, lines[i], kTextScale, p.accent);
      }
    } else {
      // Wi-Fi arcs, struck through.
      for (int r = 22; r >= 8; r -= 7) s.fillArc(kW / 2, cy + 12, r, r - 3, 225, 315, p.dim);
      s.fillCircle(kW / 2, cy + 12, 3, p.dim);
      s.drawWideLine(kW / 2 - 18, cy - 14, kW / 2 + 18, cy + 14, 1.8f, p.accent);
      centredWrapped("Connect to Wi-Fi to get a link code.", 22, cy + 32, kTextScale, p.text, 20);
    }
    softKeys({{"AC", "Back"}});
    return;
  }
  gd::textCentered(s, kW / 2, kContentY + 4, "Your link code", kTextScale, p.dim);
  const std::string& code = dev.linkCode();
  int scale = 5;
  while (scale > 2 && gd::width(code, scale) > kW - 40) --scale;
  const int cw = gd::width(code, scale) + 24, ch = gd::kGlyphH * scale + 16;
  const int cx = (kW - cw) / 2, cy = kContentY + 24;
  s.fillRoundRect(cx, cy, cw, ch, 8, p.panel);
  s.drawRoundRect(cx, cy, cw, ch, 8, p.accent);
  gd::textCentered(s, kW / 2 + scale / 2, cy + 8, code, scale, p.accent);
  const std::string url = stripScheme(dev.linkUrl());
  const std::string text = url.empty() ? "Type this code on the website's link page."
                                       : "Go to " + url + "/link and type this code.";
  centredWrapped(text, 27, cy + ch + 8, kTextScale, p.text, 18);
  softKeys({{"AC", "Back"}});
}

}  // namespace ui
