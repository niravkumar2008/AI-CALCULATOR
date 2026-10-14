// Geography (Apps 5) in colour: a filled world map drawn from the core's polygon data
// (core/geo.h: 177 countries, 288 rings, 7.6k points), the country card, the A-Z list,
// the letter picker and the capitals quiz. State comes from Device::geo(); only the
// drawing is here.
//
// The map: every country is scan-line filled (even-odd over its rings, so lakes and
// enclaves stay holes), then outlined with thin borders when zoomed in. The core's
// focusBox() is fitted to this screen's 320 x 104 map rectangle with the core's own rules
// (equirectangular, cos(latitude) when zoomed in), so the same countries are in view as
// on the mono screen. Names are UTF-8; the 5x7 font has no accented letters, so they are
// folded (geo::asciiFold: "Brasília" -> "Brasilia").
#include <algorithm>
#include <cmath>
#include <vector>

#include "text.h"
#include "ui_draw.h"

using calc::Device;
namespace geo = calc::geo;

namespace ui {

namespace {

LGFX_Sprite& S() { return lcd::canvas(); }

// The map rectangle and footer.
constexpr int kMapX = 0, kMapY = kContentY, kMapW = kW, kMapH = 104;
constexpr int kFootY = kMapY + kMapH;  // 128..147

const uint16_t kSea = rgb565(0x12, 0x2c, 0x4a);
const uint16_t kSeaDeep = rgb565(0x0e, 0x22, 0x3a);
const uint16_t kLand = rgb565(0x5f, 0x7f, 0x63);
const uint16_t kLandOut = rgb565(0x34, 0x45, 0x3d);   // outside the continent filter
const uint16_t kBorder = rgb565(0x2a, 0x3d, 0x30);
const uint16_t kGraticule = rgb565(0x1a, 0x3a, 0x5c);

struct View {
  float lon, lat, sx, sy;  // centre, px per degree
  int x, y, w, h;
};

// GeoApp::viewport()'s rules, fitted to our rectangle.
View fit(const geo::GeoApp& g, int x, int y, int w, int h) {
  const geo::GeoBox b = g.focusBox();
  View v;
  v.x = x, v.y = y, v.w = w, v.h = h;
  v.lon = (b.lon0 + b.lon1) / 2;
  v.lat = (b.lat0 + b.lat1) / 2;
  const bool world = g.zoom() == 0 && g.filter() == 0;
  float kx = world ? 1.0f : std::cos(v.lat * 3.14159265f / 180.0f);
  kx = std::max(kx, 0.25f);
  const float margin = world ? 1.0f : 1.1f;
  const float spanX = (b.lon1 - b.lon0) * kx * margin, spanY = (b.lat1 - b.lat0) * margin;
  float s = std::min((w - 2) / std::max(spanX, 0.01f), (h - 2) / std::max(spanY, 0.01f));
  s = std::min(s, 50.0f);
  v.sy = s;
  v.sx = s * kx;
  return v;
}

float ringShift(const geo::Ring& rg, float viewLon) {  // the ±360 that brings a ring nearest the centre
  const float d = geo::point(rg.first).lon / 100.0f - viewLon;
  return d > 180 ? -360.0f : d < -180 ? 360.0f : 0.0f;
}

// Projected points of one country (all rings), reused between countries: no allocation
// after the first frame.
struct Poly {
  std::vector<float> px, py;
  std::vector<uint16_t> start;  // ring k = [start[k], start[k+1])
  float x0, y0, x1, y1;
};

void project(const View& v, int ci, Poly& p) {
  const geo::Country& c = geo::country(ci);
  p.px.clear();
  p.py.clear();
  p.start.clear();
  p.x0 = p.y0 = 1e9f;
  p.x1 = p.y1 = -1e9f;
  const float cx = v.x + v.w / 2.0f, cy = v.y + v.h / 2.0f;
  for (int r = 0; r < c.ringCount; ++r) {
    const geo::Ring& rg = geo::ring(c.firstRing + r);
    const float sh = ringShift(rg, v.lon) - v.lon;
    p.start.push_back(uint16_t(p.px.size()));
    for (int k = 0; k < rg.count; ++k) {
      const geo::Point& q = geo::point(rg.first + k);
      const float x = cx + (q.lon / 100.0f + sh) * v.sx, y = cy - (q.lat / 100.0f - v.lat) * v.sy;
      p.px.push_back(x);
      p.py.push_back(y);
      p.x0 = std::min(p.x0, x), p.x1 = std::max(p.x1, x);
      p.y0 = std::min(p.y0, y), p.y1 = std::max(p.y1, y);
    }
  }
  p.start.push_back(uint16_t(p.px.size()));
}

bool offScreen(const View& v, const Poly& p) {
  return p.x1 < v.x || p.x0 >= v.x + v.w || p.y1 < v.y || p.y0 >= v.y + v.h;
}

// Even-odd scan-line fill, sampling each row at its centre.
void fill(const View& v, const Poly& p, uint16_t color) {
  static std::vector<float> xs;
  LGFX_Sprite& s = S();
  const int ya = std::max(v.y, int(std::floor(p.y0))), yb = std::min(v.y + v.h - 1, int(std::ceil(p.y1)));
  for (int y = ya; y <= yb; ++y) {
    xs.clear();
    const float sy = y + 0.5f;
    for (size_t r = 0; r + 1 < p.start.size(); ++r) {
      const int a0 = p.start[r], n = p.start[r + 1] - a0;
      for (int k = 0, j = n - 1; k < n; j = k++) {
        const float ay = p.py[size_t(a0 + k)], by = p.py[size_t(a0 + j)];
        if ((ay > sy) != (by > sy)) {
          const float ax = p.px[size_t(a0 + k)], bx = p.px[size_t(a0 + j)];
          xs.push_back(ax + (sy - ay) * (bx - ax) / (by - ay));
        }
      }
    }
    std::sort(xs.begin(), xs.end());
    for (size_t i = 0; i + 1 < xs.size(); i += 2) {
      const int a = std::max(v.x, int(std::lround(xs[i])));
      const int b = std::min(v.x + v.w - 1, int(std::lround(xs[i + 1])) - 1);
      if (b >= a) s.drawFastHLine(a, y, b - a + 1, color);
    }
  }
}

void outline(const Poly& p, uint16_t color) {
  LGFX_Sprite& s = S();
  for (size_t r = 0; r + 1 < p.start.size(); ++r) {
    const int a0 = p.start[r], n = p.start[r + 1] - a0;
    int lx = int(std::lround(p.px[size_t(a0 + n - 1)])), ly = int(std::lround(p.py[size_t(a0 + n - 1)]));
    for (int k = 0; k < n; ++k) {
      const int x = int(std::lround(p.px[size_t(a0 + k)])), y = int(std::lround(p.py[size_t(a0 + k)]));
      if (x != lx || y != ly) s.drawLine(lx, ly, x, y, color);
      lx = x, ly = y;
    }
  }
}

bool projectPoint(const View& v, float lon, float lat, int& x, int& y) {
  float d = lon - v.lon;
  if (d > 180) d -= 360;
  if (d < -180) d += 360;
  x = int(std::lround(v.x + v.w / 2.0f + d * v.sx));
  y = int(std::lround(v.y + v.h / 2.0f - (lat - v.lat) * v.sy));
  return x >= v.x && x < v.x + v.w && y >= v.y && y < v.y + v.h;
}

std::string folded(const char* utf8) { return geo::asciiFold(utf8); }

void drawMap(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const geo::GeoApp& g = f.dev.geo();
  const View v = fit(g, kMapX, kMapY, kMapW, kMapH);
  s.fillRect(kMapX, kMapY, kMapW, kMapH, kSea);
  s.setClipRect(kMapX, kMapY, kMapW, kMapH);
  // The world's edge (beyond ±180° / the poles in the world view) a shade darker.
  int ex0, ey0, ex1, ey1;
  projectPoint(v, v.lon - 180, 90, ex0, ey0);
  projectPoint(v, v.lon + 180, -90, ex1, ey1);
  if (g.zoom() == 0 && g.filter() == 0) {
    if (ex0 > kMapX) s.fillRect(kMapX, kMapY, ex0 - kMapX, kMapH, kSeaDeep);
    if (ex1 < kMapX + kMapW) s.fillRect(ex1, kMapY, kMapX + kMapW - ex1, kMapH, kSeaDeep);
  }
  // A light graticule every 30° (every 10° when zoomed in).
  const int step = v.sy < 3 ? 30 : 10;
  for (int lat = -90 + step; lat < 90; lat += step) {
    int x, y;
    projectPoint(v, v.lon, float(lat), x, y);
    if (y >= kMapY && y < kMapY + kMapH) s.drawFastHLine(std::max(kMapX, ex0), y, std::min(ex1, kMapX + kMapW) - std::max(kMapX, ex0), kGraticule);
  }
  for (int lon = -180; lon < 180; lon += step) {
    int x, y;
    if (projectPoint(v, float(lon), v.lat, x, y)) s.drawFastVLine(x, kMapY, kMapH, kGraticule);
  }
  static Poly poly;
  const bool borders = v.sy >= 1.2f;  // in the world view 1-px borders would merge into a mesh
  const int sel = g.selected();
  for (int i = 0; i < geo::countryCount(); ++i) {
    if (i == sel) continue;
    project(v, i, poly);
    if (offScreen(v, poly)) continue;
    fill(v, poly, g.inFilter(i) ? kLand : kLandOut);
    if (borders) outline(poly, kBorder);
  }
  project(v, sel, poly);
  const int size = int(std::max(poly.x1 - poly.x0, poly.y1 - poly.y0));
  if (!offScreen(v, poly)) {
    fill(v, poly, p.accent);
    outline(poly, rgb565(0xff, 0xc4, 0xa8));
  }
  const geo::Country& c = geo::country(sel);
  int lx, ly;
  if (size < 7) {  // tiny at this zoom: a ring around it so it can be found
    if (projectPoint(v, c.labelLon / 100.0f, c.labelLat / 100.0f, lx, ly)) {
      s.drawCircle(lx, ly, 8, p.accent);
      s.drawCircle(lx, ly, 9, p.accent);
      s.drawCircle(lx, ly, 10, color::white);
    }
  } else if (geo::hasCapital(sel) && projectPoint(v, c.capLon / 100.0f, c.capLat / 100.0f, lx, ly)) {
    s.fillCircle(lx, ly, 4, p.bg);
    s.fillCircle(lx, ly, 2, color::white);
    s.drawCircle(lx, ly, 5, color::white);
  }
  s.clearClipRect();
  // The continent filter, top left.
  if (g.filter() != 0) {
    const std::string name = geo::GeoApp::filterName(g.filter());
    const int w = gd::width(name, kTextScale, gd::tight(kTextScale)) + 12;
    s.fillRoundRect(4, kMapY + 4, w, 18, 5, p.bg);
    s.drawRoundRect(4, kMapY + 4, w, 18, 5, p.accent);
    gd::text(s, 10, kMapY + 6, name, kTextScale, p.text, gd::tight(kTextScale));
  }
  // Zoom level, top right: three small bars on a chip.
  s.fillRoundRect(kW - 35, kMapY + 3, 31, 22, 5, p.bg);
  for (int z = 0; z < geo::GeoApp::kZoomLevels; ++z)
    s.fillRoundRect(kW - 30 + z * 8, kMapY + 16 - z * 4, 6, 6 + z * 4, 1, z <= g.zoom() ? p.accent : p.panel2);
  // Footer: a pin, the name, the capital.
  s.fillRect(0, kFootY, kW, kContentBottom - kFootY, p.bar);
  s.fillCircle(14, kFootY + 7, 5, p.accent);
  s.fillTriangle(10, kFootY + 9, 18, kFootY + 9, 14, kFootY + 16, p.accent);
  s.fillCircle(14, kFootY + 7, 2, p.bar);
  const std::string name = folded(c.name);
  const std::string cap = geo::hasCapital(sel) ? folded(c.capital) : std::string();
  int x = 26;
  const int maxW = kW - x - 8;
  const int nameW = gd::width(name, kTextScale), capW = cap.empty() ? 0 : gd::width(" \xC2\xB7 " + cap, kTextScale);
  if (nameW + capW <= maxW) {
    x = gd::text(s, x, kFootY + 3, name, kTextScale, p.text);
    if (!cap.empty()) gd::text(s, x, kFootY + 3, " \xC2\xB7 " + cap, kTextScale, p.dim);
  } else {
    const int adv = gd::tight(kTextScale);
    const int nw = gd::width(name, kTextScale, adv);
    if (cap.empty() || nw + gd::width(" \xC2\xB7 " + cap, kTextScale, adv) > maxW) {
      fitText(x, kFootY + 3, name, kTextScale, p.text, maxW);
    } else {
      x = gd::text(s, x, kFootY + 3, name, kTextScale, p.text, adv);
      gd::text(s, x, kFootY + 3, " \xC2\xB7 " + cap, kTextScale, p.dim, adv);
    }
  }
  softKeys({{"=", "Info"}, {"1/0", "Zoom"}, {"2", "List"}, {"5", "Quiz"}});
}

void drawCard(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const geo::GeoApp& g = f.dev.geo();
  const geo::Country& c = geo::country(g.selected());
  const std::string name = folded(c.name);
  // The name as a heading (3x when it fits), then "label: value" lines.
  if (gd::width(name, 3) <= kW - 20) gd::text(s, kMarginX, kContentY + 3, name, 3, p.accent);
  else fitText(kMarginX, kContentY + 6, name, kTextScale, p.accent, kW - 2 * kMarginX);
  s.fillRect(kMarginX, kContentY + 27, kW - 2 * kMarginX, 1, p.rule);
  const std::vector<std::string> lines = g.cardLines();
  const int nameLines = int(calc::wrapText(name, calc::Framebuffer::kCols).size());
  const int first = nameLines + g.cardScroll(), visible = 5;
  for (int r = 0; r < visible && first + r < int(lines.size()); ++r) {
    const std::string& l = lines[size_t(first + r)];
    const int y = kContentY + 33 + r * 19;
    const size_t colon = l.find(": ");
    if (colon != std::string::npos && l[0] != ' ') {
      const int x = gd::text(s, kMarginX, y, l.substr(0, colon + 1), kTextScale, p.dim);
      gd::text(s, x + 4, y, l.substr(colon + 2), kTextScale, p.text);
    } else {
      gd::text(s, kMarginX, y, l, kTextScale, l.rfind("Lat", 0) == 0 || l.rfind("Lon", 0) == 0 ? p.dim : p.text);
    }
  }
  scrollBar(kContentY + 33, visible * 19, g.cardScroll(), visible, int(lines.size()) - nameLines);
  softKeys({{"\xE2\x97\x80\xE2\x96\xB6", "Next country"}, {"AC", "Map"}});
}

void drawCountryList(const Frame& f) {
  const geo::GeoApp& g = f.dev.geo();
  const std::vector<int>& items = g.listItems();
  std::vector<ListRow> rows;
  // Only the rows on screen are built (the list holds up to 177).
  const int top = g.listTop();
  for (int i = 0; i < int(items.size()); ++i) {
    ListRow r;
    if (i >= top && i < top + 5) {
      const geo::Country& c = geo::country(items[size_t(i)]);
      r.left = folded(c.name);
      if (geo::hasCapital(items[size_t(i)])) {
        const std::string cap = folded(c.capital);
        const int adv = gd::tight(kTextScale);
        if (gd::width(r.left, kTextScale) + gd::width(cap, kTextScale, adv) + 40 <= kW - 18) r.right = cap;
      }
    }
    rows.push_back(r);
  }
  drawList(rows, g.listCursor(), top);
  if (items.empty()) gd::textCentered(S(), kW / 2, kContentY + 40, "No countries here.", kTextScale, pal().dim);
  softKeys({{"\xE2\x96\xB2\xE2\x96\xBC", "Move"}, {"=", "Show"}, {"3", "Find"}, {"AC", "Map"}});
}

void letterGrid(int cursor, int cells, int cols, int y0) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const int pitchX = 34, pitchY = 23, w = 32, h = 21;
  const int x0 = (kW - cols * pitchX + 2) / 2;
  for (int i = 0; i < cells; ++i) {
    const int x = x0 + (i % cols) * pitchX, y = y0 + (i / cols) * pitchY;
    const bool sel = i == cursor;
    const bool ok = i == 26;
    s.fillRoundRect(x, y, w, h, 4, sel ? p.accent : ok ? rgb565(0x4d, 0x2c, 0x1f) : p.panel2);
    if (ok && !sel) s.drawRoundRect(x, y, w, h, 4, p.accent);
    const std::string t = ok ? "OK" : std::string(1, char('A' + i));
    gd::textCentered(s, x + w / 2 + 1, y + 4, t, kTextScale, sel ? p.onAccent : ok ? p.accent : p.text);
  }
}

void drawSearch(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const geo::GeoApp& g = f.dev.geo();
  s.fillRoundRect(6, kContentY + 2, kW - 12, 22, 5, p.panel);
  int x = gd::text(s, 14, kContentY + 6, "Find:", kTextScale, p.dim);
  x = gd::text(s, x + 8, kContentY + 6, g.searchPrefix(), kTextScale, p.text);
  s.fillRect(x + 1, kContentY + 5, 2, 15, p.accent);
  const int m = g.searchMatch();
  const std::string match = m >= 0 ? "\xE2\x86\x92 " + folded(geo::country(m).name) : "no match";
  letterGrid(g.searchCursor(), geo::GeoApp::kGridCells, geo::GeoApp::kGridCols, kContentY + 29);
  fitText(14, kContentY + 101, match, kTextScale, m >= 0 ? p.accent : p.dim, kW - 28);
  softKeys({{"=", g.searchCursor() == 26 ? "List" : "Add"}, {"DEL", "Erase"}, {"AC", "Back"}});
}

void drawQuiz(const Frame& f) {
  LGFX_Sprite& s = S();
  const Palette& p = pal();
  const geo::GeoApp& g = f.dev.geo();
  const geo::GeoApp::Quiz& q = g.quiz();
  const bool capitalOf = q.kind == geo::GeoApp::QuizKind::CapitalOf;
  const geo::Country& subj = geo::country(q.subject);
  const bool answered = q.picked >= 0;
  std::string head = capitalOf ? "What is the capital of" : "Which country's capital is";
  uint16_t headColor = p.dim;
  if (answered) {
    head = q.picked == q.correct ? "Right!" : "Not quite.";
    headColor = q.picked == q.correct ? p.good : rgb565(0xff, 0x8a, 0x8a);
  }
  gd::text(s, kMarginX, kContentY + 3, head, kTextScale, headColor);
  const std::string subject = folded(capitalOf ? subj.name : subj.capital) + "?";
  if (gd::width(subject, 3) <= kW - 20) gd::text(s, kMarginX, kContentY + 20, subject, 3, p.text);
  else fitText(kMarginX, kContentY + 24, subject, kTextScale, p.text, kW - 20);
  for (int k = 0; k < 4; ++k) {
    const geo::Country& c = geo::country(q.choices[k]);
    const int y = kContentY + 45 + k * 20;
    uint16_t fill = p.panel, ink = p.text, badge = p.panel2, badgeInk = p.text;
    if (answered && k == q.correct) fill = p.good, ink = p.bg, badge = p.bg, badgeInk = p.good;
    else if (answered && k == q.picked) fill = p.error, ink = color::white, badge = color::white, badgeInk = p.error;
    s.fillRoundRect(6, y, kW - 12, 18, 4, fill);
    s.fillRoundRect(8, y + 2, 16, 14, 3, badge);
    gd::text(s, 11, y + 2, std::string(1, char('1' + k)), kTextScale, badgeInk);
    fitText(30, y + 2, folded(capitalOf ? c.capital : c.name), kTextScale, ink, kW - 30 - 30);
    if (answered && k == q.correct) gd::text(s, kW - 26, y + 2, "\xE2\x9C\x93", kTextScale, ink);
    if (answered && k == q.picked && k != q.correct) gd::text(s, kW - 26, y + 2, "x", kTextScale, ink);
  }
  if (answered) softKeys({{"=", "Next"}, {"AC", "Map"}});
  else softKeys({{"1-4", "Answer"}, {"AC", "Map"}});
}

}  // namespace

std::string geoTitle(const Device& dev, std::string& right) {
  const geo::GeoApp& g = dev.geo();
  switch (g.screen()) {
    case geo::GeoApp::Screen::List: {
      const int n = int(g.listItems().size());
      right = std::to_string(n ? g.listCursor() + 1 : 0) + "/" + std::to_string(n);
      return g.filter() == 0 ? "COUNTRIES" : std::string("COUNTRIES: ") + geo::GeoApp::filterName(g.filter());
    }
    case geo::GeoApp::Screen::Search:
      right = std::to_string(g.searchMatches());
      return "FIND A COUNTRY";
    case geo::GeoApp::Screen::Quiz:
      right = std::to_string(g.quiz().score) + "/" + std::to_string(g.quiz().asked);
      return "CAPITALS QUIZ";
    default: return "GEOGRAPHY";
  }
}

void drawGeo(const Frame& f) {
  switch (f.dev.geo().screen()) {
    case geo::GeoApp::Screen::Map: return drawMap(f);
    case geo::GeoApp::Screen::Card: return drawCard(f);
    case geo::GeoApp::Screen::List: return drawCountryList(f);
    case geo::GeoApp::Screen::Search: return drawSearch(f);
    case geo::GeoApp::Screen::Quiz: return drawQuiz(f);
  }
}

}  // namespace ui
