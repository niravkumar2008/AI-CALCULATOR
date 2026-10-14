// Geography app tests: data invariants, capitals, navigation, point-in-polygon,
// quiz, and rendering every screen for every country. Call runGeoTests() from
// the test runner's main(); it returns the number of failures.
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "../core/font.h"
#include "../core/geo.h"

using namespace calc;
using namespace calc::geo;

namespace {

int g_fail = 0, g_pass = 0;
#define GCHECK(cond)                                                               \
  do {                                                                             \
    if (cond) ++g_pass;                                                            \
    else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
  } while (0)

int litPixels(const Framebuffer& fb, int y0, int y1) {
  int n = 0;
  for (int y = y0; y < y1; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) n += fb.get(x, y) ? 1 : 0;
  return n;
}

void press(GeoApp& a, DKey k, int times = 1) {
  for (int i = 0; i < times; ++i) a.onKey(k);
}

void testData() {
  const int n = countryCount();
  GCHECK(n >= 170 && n <= 180);
  GCHECK(n == 177);
  std::set<std::string> iso, names, caps;
  int noCapital = 0;
  for (int i = 0; i < n; ++i) {
    const Country& c = country(i);
    GCHECK(std::strlen(c.iso3) == 3);
    iso.insert(c.iso3);
    names.insert(c.name);
    if (c.ringCount < 1) { GCHECK(c.ringCount >= 1); continue; }
    for (int r = 0; r < c.ringCount; ++r) {
      const int ri = c.firstRing + r;
      if (!(ri < ringCount() && ring(ri).count >= 3 && ring(ri).first + ring(ri).count <= pointCount()))
        GCHECK(!"ring out of range");
    }
    if (!hasCapital(i)) { ++noCapital; continue; }
    caps.insert(c.capital);
    if (!(c.capLon >= -18000 && c.capLon <= 18000 && c.capLat >= -9000 && c.capLat <= 9000 &&
          !(c.capLon == 0 && c.capLat == 0)))
      GCHECK(!"capital out of range");
    if (!(c.box[0] < c.box[2] && c.box[1] < c.box[3])) GCHECK(!"empty zoom box");
  }
  GCHECK(int(iso.size()) == n);    // unique codes
  GCHECK(int(names.size()) == n);  // unique names
  GCHECK(noCapital == 3);          // Antarctica, Fr. S. Antarctic Lands, N. Cyprus
  GCHECK(!hasCapital(findIso("ATA")));
  GCHECK(int(caps.size()) == n - noCapital);  // capital names unique: the quiz relies on it
  // alphabetical (folded) order
  for (int i = 1; i < n; ++i) {
    std::string a = asciiFold(country(i - 1).name), b = asciiFold(country(i).name);
    for (char& ch : a) ch = char(std::tolower(static_cast<unsigned char>(ch)));
    for (char& ch : b) ch = char(std::tolower(static_cast<unsigned char>(ch)));
    if (!(a < b)) GCHECK(!"not sorted");
  }
  // size budget (~60 KB)
  const size_t bytes = dataBytes();
  std::printf("geo: %d countries, %d rings, %d points, %u bytes of tables\n", n, ringCount(), pointCount(),
              unsigned(bytes));
  GCHECK(bytes <= 60u * 1024u);
  // codes without ISO
  GCHECK(!country(findName("Kosovo")).isoOfficial);
  GCHECK(country(findIso("FRA")).isoOfficial);
  GCHECK(findIso("NOR") >= 0);  // ISO_A3 is -99 in NE for Norway and France: _EH fills them
  // every folded name and capital renders without '?' in the 5x7 font
  for (int i = 0; i < n; ++i) {
    for (const char* s : {country(i).name, country(i).capital})
      for (uint32_t cp : decodeUtf8(asciiFold(s)))
        if (!hasGlyph(cp)) { std::printf("  no glyph U+%04X in %s\n", unsigned(cp), s); GCHECK(!"glyph"); }
  }
  GCHECK(asciiFold("Bras\xC3\xADlia") == "Brasilia");
  GCHECK(asciiFold("C\xC3\xB4te d'Ivoire") == "Cote d'Ivoire");
  GCHECK(asciiFold("\xC5\x81\xC3\xB3""d\xC5\xBA") == "Lodz");
}

void testCapitals() {
  struct S { const char* iso; const char* cap; float lat, lon; };
  const S spots[] = {
      {"FRA", "Paris", 48.86f, 2.35f},         {"JPN", "Tokyo", 35.68f, 139.69f},
      {"BRA", "Bras\xC3\xADlia", -15.79f, -47.88f}, {"AUS", "Canberra", -35.28f, 149.13f},
      {"CAN", "Ottawa", 45.42f, -75.70f},      {"EGY", "Cairo", 30.04f, 31.24f},
      {"IND", "New Delhi", 28.61f, 77.21f},    {"KEN", "Nairobi", -1.29f, 36.82f},
      {"MEX", "Mexico City", 19.43f, -99.13f}, {"DEU", "Berlin", 52.52f, 13.40f},
      {"GBR", "London", 51.51f, -0.13f},       {"USA", "Washington, D.C.", 38.90f, -77.04f},
      {"CHN", "Beijing", 39.90f, 116.40f},     {"RUS", "Moscow", 55.76f, 37.62f},
      {"ARG", "Buenos Aires", -34.60f, -58.38f}, {"ZAF", "Pretoria", -25.75f, 28.19f},
      {"NGA", "Abuja", 9.08f, 7.40f},          {"ITA", "Rome", 41.90f, 12.50f},
      {"ESP", "Madrid", 40.42f, -3.70f},       {"TUR", "Ankara", 39.93f, 32.86f},
      {"KOR", "Seoul", 37.57f, 126.98f},       {"IDN", "Jakarta", -6.21f, 106.85f},
      {"TZA", "Dodoma", -6.16f, 35.75f},       {"BOL", "Sucre", -19.03f, -65.26f},
      {"NZL", "Wellington", -41.29f, 174.78f}, {"PER", "Lima", -12.05f, -77.04f},
      {"DNK", "Copenhagen", 55.68f, 12.57f},   {"KAZ", "Astana", 51.17f, 71.43f},
  };
  for (const S& s : spots) {
    const int i = findIso(s.iso);
    GCHECK(i >= 0);
    if (i < 0) continue;
    const Country& c = country(i);
    const bool ok = std::strcmp(c.capital, s.cap) == 0 && std::fabs(c.capLat / 100.0f - s.lat) < 1.0f &&
                    std::fabs(c.capLon / 100.0f - s.lon) < 1.0f;
    if (!ok) std::printf("  %s: got %s %.2f %.2f\n", s.iso, c.capital, c.capLat / 100.0, c.capLon / 100.0);
    GCHECK(ok);
  }
  GCHECK(country(findIso("FRA")).continent == Continent::Europe);
  GCHECK(country(findIso("BRA")).continent == Continent::SouthAmerica);
}

void testPointInPolygon() {
  const int fra = findIso("FRA"), esp = findIso("ESP");
  GCHECK(containsPoint(fra, 2.35f, 48.86f));    // Paris in France
  GCHECK(!containsPoint(esp, 2.35f, 48.86f));   // not in Spain
  GCHECK(containsPoint(esp, -3.70f, 40.42f));   // Madrid in Spain
  GCHECK(!containsPoint(fra, -3.70f, 40.42f));
  GCHECK(containsPoint(findIso("DEU"), 13.40f, 52.52f));
  GCHECK(containsPoint(findIso("BRA"), -47.88f, -15.79f));
  GCHECK(containsPoint(findIso("AUS"), 149.13f, -35.28f) || containsPoint(findIso("AUS"), 147.0f, -34.0f));
  GCHECK(containsPoint(findIso("USA"), -100.0f, 40.0f));
  GCHECK(!containsPoint(findIso("USA"), -100.0f, 60.0f));  // that's Canada
  GCHECK(containsPoint(findIso("CAN"), -100.0f, 60.0f));
  GCHECK(!containsPoint(fra, -30.0f, 40.0f));    // the Atlantic
  // Lesotho is a hole in South Africa
  GCHECK(containsPoint(findIso("LSO"), 28.2f, -29.5f));
  GCHECK(!containsPoint(findIso("ZAF"), 28.2f, -29.5f));
}

void testNavigation() {
  GeoApp a;
  const int n = countryCount();
  GCHECK(a.screen() == GeoApp::Screen::Map);
  GCHECK(a.selected() == findIso("FRA"));
  // ▶ visits every country exactly once per cycle, ◀ too
  for (DKey k : {DKey::Right, DKey::Left}) {
    std::vector<int> seen(size_t(n), 0);
    const int start = a.selected();
    for (int i = 0; i < n; ++i) {
      a.onKey(k);
      ++seen[size_t(a.selected())];
    }
    bool once = true;
    for (int s : seen) once = once && s == 1;
    GCHECK(once);
    GCHECK(a.selected() == start);
  }
  // ▶ goes west → east inside a continent (Spain → France → ...)
  a.select(findIso("ESP"));
  a.onKey(DKey::Right);
  GCHECK(country(a.selected()).labelLon >= country(findIso("ESP")).labelLon);
  // ▲ ▼ land on a country whose centre is north / south
  int movedN = 0, movedS = 0;
  bool dirOk = true;
  for (int c = 0; c < n; ++c) {
    const int up = a.northSouth(c, +1), dn = a.northSouth(c, -1);
    if (up != c) { ++movedN; dirOk = dirOk && country(up).labelLat > country(c).labelLat; }
    if (dn != c) { ++movedS; dirOk = dirOk && country(dn).labelLat < country(c).labelLat; }
  }
  GCHECK(dirOk);
  GCHECK(movedN > n - 10 && movedS > n - 10);
  a.select(findIso("FRA"));
  a.onKey(DKey::Up);
  GCHECK(country(a.selected()).labelLat > country(findIso("FRA")).labelLat);
  GCHECK(country(a.selected()).continent == Continent::Europe);  // a neighbour, not Canada
  a.select(findIso("FRA"));
  a.onKey(DKey::Down);
  GCHECK(country(a.selected()).labelLat < country(findIso("FRA")).labelLat);
  // continent filter: ◀ ▶ stay inside, cycle covers the continent exactly once
  for (int f = 1; f < GeoApp::kFilterCount; ++f) {
    GeoApp b;
    press(b, DKey::D4, f);
    GCHECK(b.filter() == f);
    GCHECK(b.inFilter(b.selected()));
    int members = 0;
    for (int c = 0; c < n; ++c) members += b.inFilter(c) ? 1 : 0;
    std::set<int> seen;
    bool inside = true;
    for (int i = 0; i < members; ++i) {
      b.onKey(DKey::Right);
      inside = inside && b.inFilter(b.selected());
      seen.insert(b.selected());
      b.onKey(DKey::Up);
      inside = inside && b.inFilter(b.selected());
      b.onKey(DKey::Down);
      inside = inside && b.inFilter(b.selected());
    }
    GCHECK(inside);
    GCHECK(int(b.listItems().size()) == members);
    GeoApp c2;
    press(c2, DKey::D4, f);
    std::set<int> cyc;
    for (int i = 0; i < members; ++i) { c2.onKey(DKey::Right); cyc.insert(c2.selected()); }
    GCHECK(int(cyc.size()) == members);
  }
  {
    GeoApp b;
    press(b, DKey::D4, 1);
    GCHECK(std::string(GeoApp::filterName(b.filter())) == "Africa");
    GCHECK(country(b.selected()).continent == Continent::Africa);
    press(b, DKey::D4, 6);
    GCHECK(b.filter() == 0);
  }
  // zoom: 1 in (continent, country), 0 out
  GeoApp z;
  z.select(findIso("ITA"));
  GCHECK(z.zoom() == 0);
  press(z, DKey::D1);
  GCHECK(z.zoom() == 1);
  press(z, DKey::D1, 3);
  GCHECK(z.zoom() == 2);
  {
    const GeoBox b = z.focusBox();
    const Country& c = country(z.selected());
    GCHECK(c.capLon / 100.0f > b.lon0 && c.capLon / 100.0f < b.lon1 && c.capLat / 100.0f > b.lat0 &&
           c.capLat / 100.0f < b.lat1);
    int x, y;
    GCHECK(z.project(12.5f, 41.9f, x, y));  // Rome on screen
    // panning follows the selection
    z.onKey(DKey::Right);
    const GeoBox b2 = z.focusBox();
    const Country& c2 = country(z.selected());
    GCHECK(c2.labelLon / 100.0f >= b2.lon0 && c2.labelLon / 100.0f <= b2.lon1);
  }
  press(z, DKey::D0);
  GCHECK(z.zoom() == 0);
  // every country's zoom-2 view puts its label point on screen
  bool allVisible = true;
  for (int c = 0; c < n; ++c) {
    z.select(c);
    for (int lvl = 0; lvl < 3; ++lvl) {
      press(z, DKey::D0);
      press(z, DKey::D1, lvl);
      int x, y;
      if (lvl > 0 && !z.project(country(c).labelLon / 100.0f, country(c).labelLat / 100.0f, x, y)) {
        allVisible = false;
        std::printf("  %s not visible at zoom %d\n", country(c).name, lvl);
      }
    }
  }
  GCHECK(allVisible);
}

void testScreens() {
  GeoApp a;
  // card
  GCHECK(a.onKey(DKey::Eq));
  GCHECK(a.screen() == GeoApp::Screen::Card);
  {
    const auto lines = a.cardLines();
    std::string all;
    for (auto& l : lines) all += l + "\n";
    GCHECK(all.find("France") != std::string::npos);
    GCHECK(all.find("Capital: Paris") != std::string::npos);
    GCHECK(all.find("Continent: Europe") != std::string::npos);
    GCHECK(all.find("ISO code: FRA") != std::string::npos);
    GCHECK(all.find("48.87\xC2\xB0N") != std::string::npos);
    GCHECK(all.find("2.33\xC2\xB0" "E") != std::string::npos);
    for (auto& l : lines) GCHECK(int(decodeUtf8(l).size()) <= Framebuffer::kCols);
  }
  a.select(findIso("BRA"));
  {
    std::string all;
    for (auto& l : a.cardLines()) all += l + "\n";
    GCHECK(all.find("15.78\xC2\xB0S") != std::string::npos);
    GCHECK(all.find("47.92\xC2\xB0W") != std::string::npos || all.find("\xC2\xB0W") != std::string::npos);
  }
  a.onKey(DKey::AC);
  GCHECK(a.screen() == GeoApp::Screen::Map);
  GCHECK(!a.wantsExit());
  // list: opens on the selection, = selects
  a.select(findIso("FRA"));
  a.onKey(DKey::D2);
  GCHECK(a.screen() == GeoApp::Screen::List);
  GCHECK(a.listItems()[size_t(a.listCursor())] == findIso("FRA"));
  a.onKey(DKey::Down);
  a.onKey(DKey::Eq);
  GCHECK(a.screen() == GeoApp::Screen::Map);
  GCHECK(a.selected() == findIso("FRA") + 1);  // the next name alphabetically
  // letter picker: J, OK -> list at Jamaica, = -> Jamaica on the map
  a.onKey(DKey::D3);
  GCHECK(a.screen() == GeoApp::Screen::Search);
  press(a, DKey::Right, 9);  // A.. -> J
  GCHECK(a.searchCursor() == 9);
  a.onKey(DKey::Eq);
  GCHECK(a.searchPrefix() == "J");
  GCHECK(a.searchMatch() == findName("Jamaica"));
  GCHECK(a.searchMatches() == 3);  // Jamaica, Japan, Jordan
  press(a, DKey::Down);            // J (row 1) -> S (row 2)
  GCHECK(a.searchCursor() == 18);
  a.onKey(DKey::Del);
  GCHECK(a.searchPrefix().empty());
  // "UNI" -> United Arab Emirates, then OK
  a.onKey(DKey::Right, false, false);  // S -> T
  a.onKey(DKey::Right);                // U
  a.onKey(DKey::Eq);
  press(a, DKey::Left, 7);             // N
  a.onKey(DKey::Eq);
  press(a, DKey::Left, 5);             // I
  a.onKey(DKey::Eq);
  GCHECK(a.searchPrefix() == "UNI");
  GCHECK(a.searchMatch() == findName("United Arab Emirates"));
  GCHECK(a.searchMatches() == 3);  // UAE, United Kingdom, United States of America
  while (a.searchCursor() != 26) a.onKey(DKey::Right);
  a.onKey(DKey::Eq);  // OK
  GCHECK(a.screen() == GeoApp::Screen::List);
  press(a, DKey::Down, 2);
  a.onKey(DKey::Eq);
  GCHECK(a.selected() == findName("United States of America"));
  // AC from the map leaves the app
  a.onKey(DKey::AC);
  GCHECK(a.wantsExit());
  a.reset();
  GCHECK(!a.wantsExit());
  GCHECK(a.screen() == GeoApp::Screen::Map);
}

void testQuiz() {
  GeoApp a;
  a.setSeed(12345);
  a.onKey(DKey::D5);
  GCHECK(a.screen() == GeoApp::Screen::Quiz);
  bool ok = true;
  int kinds[2] = {0, 0};
  int right = 0;
  for (int qn = 0; qn < 300; ++qn) {
    const GeoApp::Quiz& q = a.quiz();
    ++kinds[int(q.kind)];
    std::set<int> ids;
    std::set<std::string> answers;
    int correctCount = 0;
    for (int k = 0; k < 4; ++k) {
      ids.insert(q.choices[k]);
      const Country& c = country(q.choices[k]);
      const char* ans = q.kind == GeoApp::QuizKind::CapitalOf ? c.capital : c.name;
      answers.insert(ans);
      const Country& s = country(q.subject);
      const bool isRight = q.kind == GeoApp::QuizKind::CapitalOf ? std::strcmp(ans, s.capital) == 0
                                                                   : std::strcmp(c.capital, s.capital) == 0;
      if (isRight) ++correctCount;
      ok = ok && hasCapital(q.choices[k]);
    }
    ok = ok && ids.size() == 4 && answers.size() == 4 && correctCount == 1 && q.choices[q.correct] == q.subject;
    // answer: right on even questions, wrong on odd ones
    const int pick = (qn % 2 == 0) ? q.correct : (q.correct + 1) % 4;
    if (pick == q.correct) ++right;
    a.onKey(pick == 0 ? DKey::D1 : pick == 1 ? DKey::D2 : pick == 2 ? DKey::D3 : DKey::D4);
    ok = ok && a.quiz().picked == pick;
    a.onKey(DKey::Eq);  // next question
  }
  GCHECK(ok);
  GCHECK(a.quiz().asked == 300);
  GCHECK(a.quiz().score == right);
  GCHECK(kinds[0] > 50 && kinds[1] > 50);  // both directions asked
  // deterministic for a seed
  GeoApp b, c;
  b.setSeed(7);
  c.setSeed(7);
  b.onKey(DKey::D5);
  c.onKey(DKey::D5);
  bool same = true;
  for (int i = 0; i < 20; ++i) {
    same = same && b.quiz().subject == c.quiz().subject && b.quiz().correct == c.quiz().correct &&
           b.quiz().kind == c.quiz().kind;
    b.onKey(DKey::D1); b.onKey(DKey::Eq);
    c.onKey(DKey::D1); c.onKey(DKey::Eq);
  }
  GCHECK(same);
  // a continent filter keeps the quiz on that continent
  GeoApp e;
  e.setSeed(99);
  press(e, DKey::D4, 3);  // Europe
  e.onKey(DKey::D5);
  bool europe = true;
  for (int i = 0; i < 30; ++i) {
    europe = europe && country(e.quiz().subject).continent == Continent::Europe;
    e.onKey(DKey::D1); e.onKey(DKey::Eq);
  }
  GCHECK(europe);
  e.onKey(DKey::AC);
  GCHECK(e.screen() == GeoApp::Screen::Map);
}

void testRenderAll() {
  GeoApp a;
  a.setSeed(1);
  const int n = countryCount();
  bool mapOk = true, statusClean = true, others = true;
  for (int c = 0; c < n; ++c) {
    for (int lvl = 0; lvl < 3; ++lvl) {
      a.reset();
      a.select(c);
      press(a, DKey::D1, lvl);
      Framebuffer fb;
      fb.clear();
      a.render(fb);
      mapOk = mapOk && litPixels(fb, 8, 53) > 0 && litPixels(fb, 53, 61) > 0;
      statusClean = statusClean && litPixels(fb, 0, 8) == 0;
    }
    // the selected country is filled at country zoom: its capital dot is white on black
    a.reset();
    a.select(c);
    a.onKey(DKey::Eq);  // card, scrolled through
    for (int s = 0; s < 6; ++s) {
      Framebuffer fb;
      a.render(fb);
      others = others && litPixels(fb, 8, 61) > 0 && litPixels(fb, 0, 8) == 0;
      a.onKey(DKey::Down);
    }
    a.onKey(DKey::AC);
    a.onKey(DKey::D2);  // list
    { Framebuffer fb; a.render(fb); others = others && litPixels(fb, 8, 61) > 0; }
    a.onKey(DKey::D3);  // picker from the list
    { Framebuffer fb; a.render(fb); others = others && litPixels(fb, 8, 61) > 0; }
    a.onKey(DKey::AC);
    GCHECK(a.screen() == GeoApp::Screen::List);
    a.onKey(DKey::AC);
    a.onKey(DKey::D5);  // quiz, before and after answering
    { Framebuffer fb; a.render(fb); others = others && litPixels(fb, 8, 61) > 0; }
    a.onKey(DKey::D2);
    { Framebuffer fb; a.render(fb); others = others && litPixels(fb, 8, 61) > 0; }
  }
  GCHECK(mapOk);
  GCHECK(statusClean);
  GCHECK(others);
  // every filter's map draws
  for (int f = 0; f < GeoApp::kFilterCount; ++f) {
    GeoApp b;
    press(b, DKey::D4, f);
    Framebuffer fb;
    b.render(fb);
    GCHECK(litPixels(fb, 8, 53) > 20);
  }
  // France at country zoom: the Paris dot is inverted inside the filled country
  GeoApp f;
  press(f, DKey::D1, 2);
  Framebuffer fb;
  f.render(fb);
  int x, y;
  GCHECK(f.project(2.33f, 48.87f, x, y));
  GCHECK(!fb.get(x, y));                                  // white dot ...
  GCHECK(fb.get(x + 4, y + 2) || fb.get(x - 4, y + 2));   // ... in black fill
}

}  // namespace

int runGeoTests() {
  g_fail = g_pass = 0;
  testData();
  testCapitals();
  testPointInPolygon();
  testNavigation();
  testScreens();
  testQuiz();
  testRenderAll();
  std::printf("geo tests: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail;
}
