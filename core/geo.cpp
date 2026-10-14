#include "geo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "font.h"

namespace calc {
namespace geo {

// Tables in geo_data.cpp (generated).
extern const Point kGeoPoints[];
extern const int kGeoPointCount;
extern const Ring kGeoRings[];
extern const int kGeoRingCount;
extern const Country kGeoCountries[];
extern const int kGeoCountryCount;

// ---------------------------------------------------------------- data ----

const char* continentName(Continent c) {
  switch (c) {
    case Continent::Africa: return "Africa";
    case Continent::Asia: return "Asia";
    case Continent::Europe: return "Europe";
    case Continent::NorthAmerica: return "North America";
    case Continent::SouthAmerica: return "South America";
    case Continent::Oceania: return "Oceania";
    case Continent::Antarctica: return "Antarctica";
    case Continent::SevenSeas: return "Seven seas";
    default: return "";
  }
}

int countryCount() { return kGeoCountryCount; }
const Country& country(int i) { return kGeoCountries[i]; }
int ringCount() { return kGeoRingCount; }
const Ring& ring(int i) { return kGeoRings[i]; }
int pointCount() { return kGeoPointCount; }
const Point& point(int i) { return kGeoPoints[i]; }
bool hasCapital(int i) { return kGeoCountries[i].capital[0] != '\0'; }

int findIso(const char* iso3) {
  for (int i = 0; i < kGeoCountryCount; ++i)
    if (std::strcmp(kGeoCountries[i].iso3, iso3) == 0) return i;
  return -1;
}

int findName(const char* name) {
  for (int i = 0; i < kGeoCountryCount; ++i)
    if (std::strcmp(kGeoCountries[i].name, name) == 0) return i;
  return -1;
}

bool containsPoint(int i, float lon, float lat) {
  const Country& c = kGeoCountries[i];
  const float x = lon * 100.0f, y = lat * 100.0f;
  bool in = false;
  for (int r = 0; r < c.ringCount; ++r) {
    const Ring& rg = kGeoRings[c.firstRing + r];
    for (int k = 0, j = rg.count - 1; k < rg.count; j = k++) {
      const Point& a = kGeoPoints[rg.first + k];
      const Point& b = kGeoPoints[rg.first + j];
      if ((a.lat > y) != (b.lat > y)) {
        const float xi = a.lon + (y - a.lat) * float(b.lon - a.lon) / float(b.lat - a.lat);
        if (x < xi) in = !in;
      }
    }
  }
  return in;
}

size_t dataBytes() {
  // A Country row on a 32-bit target: 3 pointers + 8 int16 + uint16 + 3 bytes = 33 -> 36 aligned.
  constexpr size_t kRow32 = 36;
  size_t n = size_t(kGeoPointCount) * sizeof(Point) + size_t(kGeoRingCount) * sizeof(Ring) +
             size_t(kGeoCountryCount) * kRow32;
  for (int i = 0; i < kGeoCountryCount; ++i) {
    const Country& c = kGeoCountries[i];
    n += std::strlen(c.name) + 1 + std::strlen(c.iso3) + 1 + std::strlen(c.capital) + 1;
  }
  return n;
}

std::string asciiFold(const std::string& utf8) {
  // U+00C0..U+00FF and U+0100..U+017F onto ASCII (the 5x7 font has no accents).
  static const char kLatin1[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
  static const char kExtA[] =
      "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlLlLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";
  std::string out;
  for (uint32_t cp : decodeUtf8(utf8)) {
    if (cp >= 0xC0 && cp <= 0xFF) out += kLatin1[cp - 0xC0];
    else if (cp >= 0x100 && cp <= 0x17F) out += kExtA[cp - 0x100];
    else if (cp >= 0x218 && cp <= 0x21B) out += "SsTt"[cp - 0x218];  // comma-below (Chișinău)
    else if (cp == 0x2018 || cp == 0x2019 || cp == 0x02BB || cp == 0x02BC) out += '\'';
    else out += encodeUtf8({cp});
  }
  return out;
}

// ------------------------------------------------------------- helpers ----

namespace {

constexpr int kMapX = 0, kMapY = 8, kMapW = Framebuffer::kWidth, kMapH = 45;  // y 8..52
constexpr int kFootY = 53;                                                     // inverted bar 53..60
constexpr float kSolidBelow = 1.5f;  // px per degree: wider views draw solid land, not borders

// Continent views for the filter (and zoom level 1), degrees.
const GeoBox kContinentBox[] = {
    {-19, -36, 53, 38},    // Africa
    {25, -12, 150, 56},    // Asia
    {-25, 34, 45, 72},     // Europe
    {-170, 5, -10, 84},    // North America
    {-82, -56, -34, 13},   // South America
    {110, -48, 180, 0},    // Oceania
    {-180, -90, 180, -60}, // Antarctica
};
const GeoBox kWorld = {-180, -58, 180, 84};

// ◀ ▶ visits continents in this order (roughly west to east), then west → east inside each.
int continentRank(Continent c) {
  switch (c) {
    case Continent::NorthAmerica: return 0;
    case Continent::SouthAmerica: return 1;
    case Continent::Europe: return 2;
    case Continent::Africa: return 3;
    case Continent::Asia: return 4;
    case Continent::Oceania: return 5;
    case Continent::Antarctica: return 6;
    default: return 7;
  }
}

float lonOf(int16_t v) { return v / 100.0f; }

// UTF-8 aware truncation to `cols` characters.
std::string clip(const std::string& s, int cols) {
  std::vector<uint32_t> cps = decodeUtf8(s);
  if (int(cps.size()) <= cols) return s;
  cps.resize(size_t(cols));
  return encodeUtf8(cps);
}

int textLen(const std::string& s) { return int(decodeUtf8(s).size()); }

// Word wrap to `cols` columns; continuation lines start with `indent` spaces.
std::vector<std::string> wrap(const std::string& text, int cols, int indent) {
  std::vector<std::string> lines;
  const std::vector<uint32_t> cps = decodeUtf8(text);
  std::vector<uint32_t> cur;
  bool hasWord = false;
  size_t i = 0;
  while (i < cps.size()) {
    while (i < cps.size() && cps[i] == ' ') ++i;
    size_t j = i;
    while (j < cps.size() && cps[j] != ' ') ++j;
    if (j == i) break;
    if (hasWord && int(cur.size() + 1 + (j - i)) > cols) {
      lines.push_back(encodeUtf8(cur));
      cur.assign(size_t(indent), ' ');
      hasWord = false;
    }
    if (hasWord) cur.push_back(' ');
    for (size_t k = i; k < j; ++k) {
      if (int(cur.size()) >= cols) {  // a word longer than a line: hard break
        lines.push_back(encodeUtf8(cur));
        cur.assign(size_t(indent), ' ');
      }
      cur.push_back(cps[k]);
    }
    hasWord = true;
    i = j;
  }
  if (hasWord) lines.push_back(encodeUtf8(cur));
  return lines;
}

std::string degrees(float v, char pos, char neg) {
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.2f\xC2\xB0%c", double(std::fabs(v)), v >= 0 ? pos : neg);
  return buf;
}

std::string lowerAscii(const std::string& s) {
  std::string o = asciiFold(s);
  for (char& ch : o)
    if (ch >= 'A' && ch <= 'Z') ch = char(ch - 'A' + 'a');
  return o;
}

// Clipped line inside the map rectangle (Bresenham).
void line(Framebuffer& fb, int x0, int y0, int x1, int y1) {
  const int L = kMapX, T = kMapY, R = kMapX + kMapW - 1, B = kMapY + kMapH - 1;
  if ((x0 < L && x1 < L) || (x0 > R && x1 > R) || (y0 < T && y1 < T) || (y0 > B && y1 > B)) return;
  // Long off-screen segments (deep zoom): clamp the walk with a parametric clip first.
  auto clipT = [](float p, float q, float& t0, float& t1) {
    if (p == 0) return q >= 0;
    const float r = q / p;
    if (p < 0) { if (r > t1) return false; if (r > t0) t0 = r; }
    else { if (r < t0) return false; if (r < t1) t1 = r; }
    return true;
  };
  float t0 = 0, t1 = 1;
  const float dx = float(x1 - x0), dy = float(y1 - y0);
  if (!clipT(-dx, float(x0 - L), t0, t1) || !clipT(dx, float(R - x0), t0, t1) ||
      !clipT(-dy, float(y0 - T), t0, t1) || !clipT(dy, float(B - y0), t0, t1))
    return;
  int ax = int(std::lround(x0 + t0 * dx)), ay = int(std::lround(y0 + t0 * dy));
  int bx = int(std::lround(x0 + t1 * dx)), by = int(std::lround(y0 + t1 * dy));
  int sx = ax < bx ? 1 : -1, sy = ay < by ? 1 : -1;
  int ddx = std::abs(bx - ax), ddy = -std::abs(by - ay), err = ddx + ddy;
  for (;;) {
    if (ax >= L && ax <= R && ay >= T && ay <= B) fb.set(ax, ay, true);
    if (ax == bx && ay == by) break;
    const int e2 = 2 * err;
    if (e2 >= ddy) { err += ddy; ax += sx; }
    if (e2 <= ddx) { err += ddx; ay += sy; }
  }
}

// Longitude shift (0 or ±360) that brings a ring nearest the view centre.
float ringShift(const Ring& rg, float viewLon) {
  const float d = lonOf(kGeoPoints[rg.first].lon) - viewLon;
  if (d > 180) return -360;
  if (d < -180) return 360;
  return 0;
}

}  // namespace

// ------------------------------------------------------------------ app ----

GeoApp::GeoApp() {
  const int n = countryCount();
  order_.resize(size_t(n));
  for (int i = 0; i < n; ++i) order_[size_t(i)] = i;
  std::sort(order_.begin(), order_.end(), [](int a, int b) {
    const Country& A = country(a);
    const Country& B = country(b);
    const int ra = continentRank(A.continent), rb = continentRank(B.continent);
    if (ra != rb) return ra < rb;
    if (A.labelLon != B.labelLon) return A.labelLon < B.labelLon;
    return a < b;
  });
  pos_.assign(size_t(n), 0);
  for (int k = 0; k < n; ++k) pos_[size_t(order_[size_t(k)])] = k;
  reset();
}

void GeoApp::reset() {
  screen_ = Screen::Map;
  exit_ = false;
  zoom_ = 0;
  filter_ = 0;
  cardScroll_ = 0;
  prefix_.clear();
  gridCur_ = 0;
  searchFrom_ = Screen::Map;
  quiz_ = Quiz();
  const int fr = findIso("FRA");
  sel_ = fr >= 0 ? fr : 0;
  rebuildList();
  listCur_ = listTop_ = 0;
}

const char* GeoApp::filterName(int f) {
  static const char* const k[kFilterCount] = {"All", "Africa", "Asia", "Europe",
                                              "North America", "South America", "Oceania"};
  return (f >= 0 && f < kFilterCount) ? k[f] : "";
}

bool GeoApp::inFilter(int c) const {
  if (filter_ == 0) return true;
  return int(country(c).continent) == filter_ - 1;  // filters 1..6 = Continent 0..5
}

void GeoApp::select(int c) {
  if (c < 0 || c >= countryCount()) return;
  sel_ = c;
  cardScroll_ = 0;
  if (!inFilter(c)) {
    filter_ = 0;
    rebuildList();
  }
}

void GeoApp::setFilter(int f) {
  filter_ = f;
  zoom_ = 0;
  if (!inFilter(sel_)) {
    for (int c : order_)
      if (inFilter(c)) { sel_ = c; break; }
  }
  rebuildList();
}

void GeoApp::rebuildList() {
  list_.clear();
  for (int i = 0; i < countryCount(); ++i)
    if (inFilter(i)) list_.push_back(i);
  listCur_ = std::min(listCur_, std::max(0, int(list_.size()) - 1));
  listTop_ = std::min(listTop_, listCur_);
}

int GeoApp::nextCountry(int from, int dir) const {
  const int n = countryCount();
  int k = pos_[size_t(from)];
  for (int step = 0; step < n; ++step) {
    k = (k + (dir > 0 ? 1 : n - 1)) % n;
    const int c = order_[size_t(k)];
    if (inFilter(c)) return c;
  }
  return from;
}

int GeoApp::northSouth(int from, int dir) const {
  const Country& f = country(from);
  const float flat = f.labelLat / 100.0f, flon = f.labelLon / 100.0f;
  const float kx = std::cos(flat * 3.14159265f / 180.0f);
  int best = from;
  float bestCost = 1e30f;
  for (int c = 0; c < countryCount(); ++c) {
    if (c == from || !inFilter(c)) continue;
    const Country& o = country(c);
    const float dy = (o.labelLat / 100.0f - flat) * float(dir);
    if (dy < 0.5f) continue;  // must be clearly north (south)
    float dx = o.labelLon / 100.0f - flon;
    if (dx > 180) dx -= 360;
    if (dx < -180) dx += 360;
    dx *= kx;
    const float cost = dy * dy + 4.0f * dx * dx;  // sideways counts double: ▲ means "up"
    if (cost < bestCost) { bestCost = cost; best = c; }
  }
  return best;
}

GeoBox GeoApp::focusBox() const {
  const Country& c = country(sel_);
  if (zoom_ == 0) return filter_ == 0 ? kWorld : kContinentBox[filter_ - 1];
  if (zoom_ == 1) {
    const int ci = int(c.continent);
    if (ci < int(Continent::SevenSeas)) return kContinentBox[ci];
    const float x = c.labelLon / 100.0f, y = c.labelLat / 100.0f;
    return {x - 25, y - 15, x + 25, y + 15};
  }
  GeoBox b = {c.box[0] / 100.0f, c.box[1] / 100.0f, c.box[2] / 100.0f, c.box[3] / 100.0f};
  // small countries: show at least 6 degrees around them so the neighbours give context
  const float minSpan = 6.0f;
  if (b.lon1 - b.lon0 < minSpan) { const float m = (b.lon0 + b.lon1) / 2; b.lon0 = m - minSpan / 2; b.lon1 = m + minSpan / 2; }
  if (b.lat1 - b.lat0 < minSpan) { const float m = (b.lat0 + b.lat1) / 2; b.lat0 = m - minSpan / 2; b.lat1 = m + minSpan / 2; }
  return b;
}

Viewport GeoApp::viewport() const {
  GeoBox b = focusBox();
  Viewport v;
  v.x = kMapX; v.y = kMapY; v.w = kMapW; v.h = kMapH;
  v.lon = (b.lon0 + b.lon1) / 2;
  v.lat = (b.lat0 + b.lat1) / 2;
  const bool world = (zoom_ == 0 && filter_ == 0);
  float kx = world ? 1.0f : std::cos(v.lat * 3.14159265f / 180.0f);
  kx = std::max(kx, 0.25f);
  const float margin = world ? 1.0f : 1.1f;
  const float spanX = (b.lon1 - b.lon0) * kx * margin, spanY = (b.lat1 - b.lat0) * margin;
  float s = std::min((v.w - 2) / std::max(spanX, 0.01f), (v.h - 2) / std::max(spanY, 0.01f));
  s = std::min(s, 20.0f);
  v.pxPerDegLat = s;
  v.pxPerDegLon = s * kx;
  return v;
}

bool GeoApp::project(float lon, float lat, int& x, int& y) const {
  const Viewport v = viewport();
  float d = lon - v.lon;
  if (d > 180) d -= 360;
  if (d < -180) d += 360;
  x = int(std::lround(v.x + v.w / 2.0f + d * v.pxPerDegLon));
  y = int(std::lround(v.y + v.h / 2.0f - (lat - v.lat) * v.pxPerDegLat));
  return x >= v.x && x < v.x + v.w && y >= v.y && y < v.y + v.h;
}

int GeoApp::searchMatch() const {
  const std::string p = lowerAscii(prefix_);
  for (int c : list_)
    if (lowerAscii(country(c).name).compare(0, p.size(), p) == 0) return c;
  return -1;
}

int GeoApp::searchMatches() const {
  const std::string p = lowerAscii(prefix_);
  int n = 0;
  for (int c : list_)
    if (lowerAscii(country(c).name).compare(0, p.size(), p) == 0) ++n;
  return n;
}

void GeoApp::openList(int cursorCountry) {
  screen_ = Screen::List;
  listCur_ = 0;
  for (size_t i = 0; i < list_.size(); ++i)
    if (list_[i] == cursorCountry) listCur_ = int(i);
  listTop_ = std::max(0, std::min(listCur_ - 2, int(list_.size()) - kListRows));
}

// ----------------------------------------------------------------- quiz ----

void GeoApp::setSeed(uint32_t seed) { rng_ = seed ? seed : 0x9E3779B9u; }

uint32_t GeoApp::rand() {  // xorshift32
  uint32_t x = rng_;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_ = x;
  return x;
}

void GeoApp::newQuestion() {
  std::vector<int> pool;
  for (int c = 0; c < countryCount(); ++c)
    if (hasCapital(c) && inFilter(c)) pool.push_back(c);
  if (pool.size() < 4) {
    pool.clear();
    for (int c = 0; c < countryCount(); ++c)
      if (hasCapital(c)) pool.push_back(c);
  }
  Quiz& q = quiz_;
  int subject = pool[rand() % pool.size()];
  if (subject == q.subject && q.asked > 0 && pool.size() > 1)  // no immediate repeat
    subject = pool[(size_t(std::find(pool.begin(), pool.end(), subject) - pool.begin()) + 1) % pool.size()];
  q.subject = subject;
  q.kind = (rand() & 1) ? QuizKind::CountryOf : QuizKind::CapitalOf;
  q.picked = -1;
  // Distractors: same continent when there are enough, else anywhere in the pool.
  std::vector<int> cand;
  for (int c : pool)
    if (c != subject && country(c).continent == country(subject).continent) cand.push_back(c);
  if (cand.size() < 3) {
    cand.clear();
    for (int c : pool)
      if (c != subject) cand.push_back(c);
  }
  q.correct = int(rand() % 4);
  int slot = 0;
  for (int k = 0; k < 4; ++k) {
    if (k == q.correct) { q.choices[k] = subject; continue; }
    // pick a distinct distractor (capital names are unique in the data; check names anyway)
    for (;;) {
      const size_t pick = rand() % cand.size();
      const int c = cand[pick];
      bool dup = false;
      for (int m = 0; m < k; ++m)
        if (q.choices[m] == c || std::strcmp(country(q.choices[m]).capital, country(c).capital) == 0) dup = true;
      if (!dup) { q.choices[k] = c; cand.erase(cand.begin() + long(pick)); break; }
      cand.erase(cand.begin() + long(pick));
    }
    ++slot;
  }
  (void)slot;
}

// ----------------------------------------------------------------- keys ----

bool GeoApp::onKey(DKey k, bool shift, bool alpha) {
  (void)shift;
  (void)alpha;
  switch (screen_) {
    case Screen::Map: return keyMap(k);
    case Screen::Card: return keyCard(k);
    case Screen::List: return keyList(k);
    case Screen::Search: return keySearch(k);
    case Screen::Quiz: return keyQuiz(k);
  }
  return false;
}

bool GeoApp::keyMap(DKey k) {
  switch (k) {
    case DKey::Left: select(nextCountry(sel_, -1)); return true;
    case DKey::Right: select(nextCountry(sel_, +1)); return true;
    case DKey::Up: select(northSouth(sel_, +1)); return true;
    case DKey::Down: select(northSouth(sel_, -1)); return true;
    case DKey::D1: if (zoom_ < kZoomLevels - 1) ++zoom_; return true;
    case DKey::D0:
      if (zoom_ == 0 && filter_ != 0) setFilter(0);  // second press: the whole world
      zoom_ = 0;
      return true;
    case DKey::Eq: screen_ = Screen::Card; cardScroll_ = 0; return true;
    case DKey::D2: openList(sel_); return true;
    case DKey::D3:
      searchFrom_ = Screen::Map;
      screen_ = Screen::Search;
      prefix_.clear();
      return true;
    case DKey::D4: setFilter((filter_ + 1) % kFilterCount); return true;
    case DKey::D5:
      screen_ = Screen::Quiz;
      quiz_ = Quiz();
      newQuestion();
      return true;
    case DKey::AC: exit_ = true; return true;
    default: return false;
  }
}

std::vector<std::string> GeoApp::cardLines() const {
  const Country& c = country(sel_);
  std::vector<std::string> out = wrap(asciiFold(c.name), Framebuffer::kCols, 1);
  auto add = [&](const std::string& s) {
    for (auto& l : wrap(s, Framebuffer::kCols, 2)) out.push_back(l);
  };
  if (hasCapital(sel_)) add("Capital: " + asciiFold(c.capital));
  else add("Capital: none listed");
  add(std::string("Continent: ") + continentName(c.continent));
  add(std::string(c.isoOfficial ? "ISO code: " : "Code: ") + c.iso3 + (c.isoOfficial ? "" : " (not ISO)"));
  if (hasCapital(sel_)) {
    add("Lat " + degrees(c.capLat / 100.0f, 'N', 'S'));
    add("Lon " + degrees(c.capLon / 100.0f, 'E', 'W'));
  }
  return out;
}

bool GeoApp::keyCard(DKey k) {
  const int lines = int(cardLines().size());
  const int maxScroll = std::max(0, lines - (Framebuffer::kRows - 1));
  switch (k) {
    case DKey::Left: select(nextCountry(sel_, -1)); return true;
    case DKey::Right: select(nextCountry(sel_, +1)); return true;
    case DKey::Up: if (cardScroll_ > 0) --cardScroll_; return true;
    case DKey::Down: if (cardScroll_ < maxScroll) ++cardScroll_; return true;
    case DKey::Eq:
    case DKey::AC: screen_ = Screen::Map; return true;
    default: return false;
  }
}

bool GeoApp::keyList(DKey k) {
  const int n = int(list_.size());
  auto fix = [&]() {
    listCur_ = std::max(0, std::min(listCur_, n - 1));
    if (listCur_ < listTop_) listTop_ = listCur_;
    if (listCur_ >= listTop_ + kListRows) listTop_ = listCur_ - kListRows + 1;
  };
  switch (k) {
    case DKey::Up: listCur_ = listCur_ > 0 ? listCur_ - 1 : n - 1; fix(); return true;
    case DKey::Down: listCur_ = listCur_ < n - 1 ? listCur_ + 1 : 0; fix(); return true;
    case DKey::Left: listCur_ -= kListRows; fix(); return true;
    case DKey::Right: listCur_ += kListRows; fix(); return true;
    case DKey::Eq:
      if (n > 0) select(list_[size_t(listCur_)]);
      screen_ = Screen::Map;
      return true;
    case DKey::D3:
      searchFrom_ = Screen::List;
      screen_ = Screen::Search;
      prefix_.clear();
      return true;
    case DKey::AC: screen_ = Screen::Map; return true;
    default: return false;
  }
}

bool GeoApp::keySearch(DKey k) {
  const int cols = kGridCols, cells = kGridCells;
  switch (k) {
    case DKey::Left: gridCur_ = (gridCur_ + cells - 1) % cells; return true;
    case DKey::Right: gridCur_ = (gridCur_ + 1) % cells; return true;
    case DKey::Up: gridCur_ = gridCur_ >= cols ? gridCur_ - cols : std::min(gridCur_ + 2 * cols, cells - 1); return true;
    case DKey::Down: gridCur_ = gridCur_ + cols < cells ? gridCur_ + cols : gridCur_ % cols; return true;
    case DKey::Eq:
      if (gridCur_ < 26) {
        if (prefix_.size() < 12) prefix_ += char('A' + gridCur_);
      } else {
        const int m = searchMatch();
        openList(m >= 0 ? m : sel_);
      }
      return true;
    case DKey::Del: if (!prefix_.empty()) prefix_.pop_back(); return true;
    case DKey::AC:
      if (searchFrom_ == Screen::List) openList(sel_);
      else screen_ = Screen::Map;
      return true;
    default: return false;
  }
}

bool GeoApp::keyQuiz(DKey k) {
  if (k == DKey::AC) { screen_ = Screen::Map; return true; }
  Quiz& q = quiz_;
  if (q.picked < 0) {
    int a = -1;
    if (k == DKey::D1) a = 0;
    else if (k == DKey::D2) a = 1;
    else if (k == DKey::D3) a = 2;
    else if (k == DKey::D4) a = 3;
    if (a < 0) return false;
    q.picked = a;
    ++q.asked;
    if (a == q.correct) ++q.score;
    return true;
  }
  newQuestion();
  return true;
}

// --------------------------------------------------------------- render ----

void GeoApp::render(Framebuffer& fb) const {
  fb.fillRect(0, 8, Framebuffer::kWidth, Framebuffer::kHeight - 8, false);
  fb.setIcon(Icon::Up, false);
  fb.setIcon(Icon::Down, false);
  switch (screen_) {
    case Screen::Map: drawMap(fb); break;
    case Screen::Card: drawCard(fb); break;
    case Screen::List: drawList(fb); break;
    case Screen::Search: drawSearch(fb); break;
    case Screen::Quiz: drawQuiz(fb); break;
  }
}

namespace {

struct Proj {
  const Viewport& v;
  float cx, cy;
  explicit Proj(const Viewport& vp) : v(vp), cx(vp.x + vp.w / 2.0f), cy(vp.y + vp.h / 2.0f) {}
  float x(const Point& p, float shift) const { return cx + (p.lon / 100.0f + shift - v.lon) * v.pxPerDegLon; }
  float y(const Point& p) const { return cy - (p.lat / 100.0f - v.lat) * v.pxPerDegLat; }
};

void outlineCountry(Framebuffer& fb, const Proj& pj, int ci) {
  const Country& c = country(ci);
  for (int r = 0; r < c.ringCount; ++r) {
    const Ring& rg = ring(c.firstRing + r);
    const float sh = ringShift(rg, pj.v.lon);
    const Point& p0 = point(rg.first);
    const int fx = int(std::lround(pj.x(p0, sh))), fy = int(std::lround(pj.y(p0)));
    int x0 = fx, y0 = fy;
    for (int k = 1; k <= rg.count; ++k) {
      int x1 = fx, y1 = fy;
      if (k < rg.count) {
        const Point& p = point(rg.first + k);
        x1 = int(std::lround(pj.x(p, sh)));
        y1 = int(std::lround(pj.y(p)));
      }
      if (x1 != x0 || y1 != y0 || k == rg.count) line(fb, x0, y0, x1, y1);
      x0 = x1;
      y0 = y1;
    }
  }
}

// Scanline fill (even-odd over all the country's rings) with `on`; returns the
// projected size in pixels (max of width and height) so tiny countries get a marker.
int fillCountry(Framebuffer& fb, const Proj& pj, int ci, bool on, std::vector<float>& xs) {
  const Country& c = country(ci);
  const Viewport& v = pj.v;
  float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
  for (int r = 0; r < c.ringCount; ++r) {
    const Ring& rg = ring(c.firstRing + r);
    const float sh = ringShift(rg, v.lon);
    for (int k = 0; k < rg.count; ++k) {
      const float x = pj.x(point(rg.first + k), sh), y = pj.y(point(rg.first + k));
      x0 = std::min(x0, x); x1 = std::max(x1, x);
      y0 = std::min(y0, y); y1 = std::max(y1, y);
    }
  }
  const int size = int(std::max(x1 - x0, y1 - y0));
  if (x1 < v.x || x0 >= v.x + v.w || y1 < v.y || y0 >= v.y + v.h) return size;  // off screen
  const int ya = std::max(v.y, int(std::floor(y0))), yb = std::min(v.y + v.h - 1, int(std::ceil(y1)));
  for (int y = ya; y <= yb; ++y) {
    xs.clear();
    const float sy = y + 0.5f;
    for (int r = 0; r < c.ringCount; ++r) {
      const Ring& rg = ring(c.firstRing + r);
      const float sh = ringShift(rg, v.lon);
      for (int k = 0, j = rg.count - 1; k < rg.count; j = k++) {
        const Point& a = point(rg.first + k);
        const Point& b = point(rg.first + j);
        const float ay = pj.y(a), by = pj.y(b);
        if ((ay > sy) != (by > sy)) {
          const float ax = pj.x(a, sh), bx = pj.x(b, sh);
          xs.push_back(ax + (sy - ay) * (bx - ax) / (by - ay));
        }
      }
    }
    std::sort(xs.begin(), xs.end());
    for (size_t i = 0; i + 1 < xs.size(); i += 2) {
      const int a = std::max(v.x, int(std::lround(xs[i])));
      const int b = std::min(v.x + v.w - 1, int(std::lround(xs[i + 1])));
      for (int x = a; x <= b; ++x) fb.set(x, y, on);
    }
  }
  return size;
}

}  // namespace

void GeoApp::drawMap(Framebuffer& fb) const {
  const Viewport v = viewport();
  const Proj pj(v);
  const Country& c = country(sel_);
  std::vector<float> xs;
  xs.reserve(64);
  int size;
  if (v.pxPerDegLat < kSolidBelow) {
    // Wide views: borders would merge into a grey mess, so land is solid black and
    // the selected country is cut out in white with its outline.
    for (int i = 0; i < countryCount(); ++i)
      if (i != sel_) fillCountry(fb, pj, i, true, xs);
    size = fillCountry(fb, pj, sel_, false, xs);
    outlineCountry(fb, pj, sel_);
  } else {
    for (int i = 0; i < countryCount(); ++i) outlineCountry(fb, pj, i);
    size = fillCountry(fb, pj, sel_, true, xs);
  }
  int lx, ly;
  if (size < 6) {  // tiny at this zoom: an inverted square ring around it so it can be found
    project(c.labelLon / 100.0f, c.labelLat / 100.0f, lx, ly);
    for (int d = -4; d <= 4; ++d) {
      const int pts[4][2] = {{lx + d, ly - 4}, {lx + d, ly + 4}, {lx - 4, ly + d}, {lx + 4, ly + d}};
      for (int q = 0; q < 4; ++q) {
        if ((q >= 2) && (d == -4 || d == 4)) continue;  // corners once
        const int x = pts[q][0], y = pts[q][1];
        if (y >= v.y && y < v.y + v.h) fb.set(x, y, !fb.get(x, y));
      }
    }
  } else if (hasCapital(sel_)) {  // capital: a 3x3 inverted dot (white on the filled country)
    if (project(c.capLon / 100.0f, c.capLat / 100.0f, lx, ly))
      for (int yy = ly - 1; yy <= ly + 1; ++yy)
        if (yy >= v.y && yy < v.y + v.h) fb.invertRect(lx - 1, yy, 3, 1);
  }
  // filter name, top-left of the map
  if (filter_ != 0) {
    const std::string f = filterName(filter_);
    const int w = textLen(f) * kCharW + 1;
    fb.fillRect(0, 8, w + 1, 9, false);
    fb.drawTextPx(1, 9, f);
  }
  // footer: "France · Paris" on an inverted bar
  std::string foot = asciiFold(c.name);
  if (hasCapital(sel_)) foot += " \xC2\xB7 " + asciiFold(c.capital);
  fb.fillRect(0, kFootY, Framebuffer::kWidth, Framebuffer::kHeight - kFootY, false);
  fb.drawTextPx(1, kFootY + 1, clip(foot, Framebuffer::kCols));
  fb.invertRect(0, kFootY, Framebuffer::kWidth, Framebuffer::kHeight - kFootY);
}

void GeoApp::drawCard(Framebuffer& fb) const {
  const std::vector<std::string> lines = cardLines();
  const int rows = Framebuffer::kRows - 1;
  for (int r = 0; r < rows && cardScroll_ + r < int(lines.size()); ++r) {
    const int i = cardScroll_ + r;
    fb.drawText(0, 1 + r, lines[size_t(i)]);
  }
  // the name lines are a heading: inverted
  const int nameLines = int(wrap(asciiFold(country(sel_).name), Framebuffer::kCols, 1).size());
  for (int r = 0; r < rows; ++r)
    if (cardScroll_ + r < nameLines) fb.invertRect(0, (1 + r) * 8, Framebuffer::kWidth, 8);
  fb.setIcon(Icon::Up, cardScroll_ > 0);
  fb.setIcon(Icon::Down, cardScroll_ + rows < int(lines.size()));
}

void GeoApp::drawList(Framebuffer& fb) const {
  const int n = int(list_.size());
  char buf[16];
  std::snprintf(buf, sizeof buf, "%d/%d", n ? listCur_ + 1 : 0, n);
  const std::string head = std::string(filter_ == 0 ? "Countries" : filterName(filter_));
  fb.drawText(0, 1, head, true);
  fb.drawText(Framebuffer::kCols - textLen(buf), 1, buf);
  for (int r = 0; r < kListRows && listTop_ + r < n; ++r) {
    const int i = listTop_ + r;
    fb.drawText(0, 2 + r, clip(asciiFold(country(list_[size_t(i)]).name), Framebuffer::kCols));
    if (i == listCur_) fb.invertRect(0, (2 + r) * 8, Framebuffer::kWidth, 8);
  }
  fb.setIcon(Icon::Up, listTop_ > 0);
  fb.setIcon(Icon::Down, listTop_ + kListRows < n);
}

void GeoApp::drawSearch(Framebuffer& fb) const {
  fb.drawText(0, 1, "Find: " + prefix_ + "_", true);
  char buf[16];
  std::snprintf(buf, sizeof buf, "%d", searchMatches());
  fb.drawText(Framebuffer::kCols - textLen(buf), 1, buf);
  for (int i = 0; i < kGridCells; ++i) {
    const int r = i / kGridCols, col = i % kGridCols;
    const int x = 4 + col * 13, y = (2 + r) * 8;
    if (i < 26) fb.drawTextPx(x + 4, y, std::string(1, char('A' + i)));
    else fb.drawTextPx(x + 2, y, "OK");
    if (i == gridCur_) fb.invertRect(x + 1, y, 12, 8);
  }
  const int m = searchMatch();
  fb.drawText(0, 5, clip(std::string("\xE2\x86\x92 ") + (m >= 0 ? asciiFold(country(m).name) : "no match"),
                         Framebuffer::kCols));
  fb.drawText(0, 6, "=add DEL erase OK=list");
}

void GeoApp::drawQuiz(Framebuffer& fb) const {
  const Quiz& q = quiz_;
  const Country& s = country(q.subject);
  const bool capitalOf = q.kind == QuizKind::CapitalOf;
  std::string head;
  if (q.picked >= 0) head = q.picked == q.correct ? "Right!" : "Not quite";
  else head = capitalOf ? "Capital of:" : "Whose capital is:";
  char score[16];
  std::snprintf(score, sizeof score, "%d/%d", q.score, q.asked);
  fb.drawText(0, 1, head, true);
  fb.drawText(Framebuffer::kCols - textLen(score), 1, score);
  fb.drawText(0, 2, clip(asciiFold(capitalOf ? s.name : s.capital), Framebuffer::kCols - 1) + "?");
  for (int k = 0; k < 4; ++k) {
    const Country& c = country(q.choices[k]);
    const std::string label = std::string(1, char('1' + k)) + " " + asciiFold(capitalOf ? c.capital : c.name);
    fb.drawText(0, 3 + k, clip(label, Framebuffer::kCols - 1));
    if (q.picked >= 0) {
      if (k == q.correct) fb.invertRect(0, (3 + k) * 8, Framebuffer::kWidth, 8);
      else if (k == q.picked) fb.drawText(Framebuffer::kCols - 1, 3 + k, "x");
    }
  }
}

}  // namespace geo
}  // namespace calc
