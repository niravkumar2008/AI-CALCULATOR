// Unit tests for the Periodic Table app (core/periodic.*). The lead hooks
// runPeriodicTests() into tests.cpp; it returns the number of failures and prints
// one line per failure.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>

#include "../core/framebuffer.h"
#include "../core/periodic.h"

using namespace calc;

namespace {

int g_fail = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
  ++g_checks;
  if (!ok) {
    ++g_fail;
    std::printf("FAIL periodic_tests.cpp:%d: %s\n", line, what);
  }
}
#define PCHECK(cond) check((cond), #cond, __LINE__)

bool near(float a, float b, float tol = 0.001f) { return std::fabs(a - b) <= tol; }
bool streq(const char* a, const char* b) { return std::strcmp(a, b) == 0; }

int cpLen(const char* s) {
  int n = 0;
  for (; *s; ++s)
    if ((static_cast<unsigned char>(*s) & 0xC0) != 0x80) ++n;
  return n;
}

int pixelsIn(const Framebuffer& fb, int y0, int y1) {
  int n = 0;
  for (int y = y0; y <= y1; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) n += fb.get(x, y) ? 1 : 0;
  return n;
}

void press(PeriodicApp& app, DKey k, bool shift = false) { app.onKey(k, shift, false); }

void goToZ(PeriodicApp& app, int z) {  // via the search screen from the table
  app.reset();
  press(app, DKey::D3);
  const DKey digits[] = {DKey::D0, DKey::D1, DKey::D2, DKey::D3, DKey::D4,
                         DKey::D5, DKey::D6, DKey::D7, DKey::D8, DKey::D9};
  char buf[8];
  std::snprintf(buf, sizeof buf, "%d", z);
  for (const char* p = buf; *p; ++p) press(app, digits[*p - '0']);
  press(app, DKey::Eq);
}

// Electrons in a configuration string such as "[Ar] 3d10 4s1".
int electrons(const char* cfg) {
  int n = 0;
  if (cfg[0] == '[') {
    const char core[3] = {cfg[1], cfg[2] == ']' ? '\0' : cfg[2], 0};
    n += findSymbol(core);
  }
  for (const char* p = cfg; *p; ++p) {
    if ((*p == 's' || *p == 'p' || *p == 'd' || *p == 'f') && p > cfg && p[-1] >= '1' && p[-1] <= '7') {
      int v = 0;
      const char* q = p + 1;
      while (*q >= '0' && *q <= '9') v = v * 10 + (*q++ - '0');
      n += v;
    }
  }
  return n;
}

void testSpotChecks() {
  const Element& h = element(1);
  PCHECK(streq(h.symbol, "H") && streq(h.name, "Hydrogen"));
  PCHECK(near(h.mass, 1.008f) && h.state == ElemState::Gas && streq(h.config, "1s1"));
  PCHECK(near(h.electronegativity, 2.2f) && near(h.ionization, 1312.0f, 0.5f));
  PCHECK(gridPos(1).row == 0 && gridPos(1).col == 0);

  const Element& he = element(2);
  PCHECK(streq(he.symbol, "He") && he.category == ElemCategory::NobleGas && he.state == ElemState::Gas);
  PCHECK(!hasValue(he.electronegativity) && he.group == 18 && he.block == ElemBlock::S);
  PCHECK(near(he.mass, 4.0026f) && near(he.ionization, 2372.3f, 0.5f));

  const Element& c = element(6);
  PCHECK(near(c.mass, 12.011f) && c.state == ElemState::Solid && streq(c.config, "[He] 2s2 2p2"));
  PCHECK(near(c.electronegativity, 2.55f) && c.category == ElemCategory::ReactiveNonmetal && c.year == 0);

  const Element& n = element(7);
  PCHECK(streq(n.symbol, "N") && near(n.mass, 14.007f) && n.state == ElemState::Gas);
  PCHECK(near(n.melt, 63.15f) && near(n.boil, 77.36f));

  const Element& o = element(8);
  PCHECK(near(o.mass, 15.999f) && near(o.electronegativity, 3.44f) && o.group == 16 && o.period == 2);

  const Element& na = element(11);
  PCHECK(streq(na.name, "Sodium") && na.category == ElemCategory::AlkaliMetal && streq(na.config, "[Ne] 3s1"));
  PCHECK(near(na.mass, 22.990f) && near(na.melt, 370.95f));

  const Element& cl = element(17);
  PCHECK(streq(cl.symbol, "Cl") && near(cl.mass, 35.45f) && cl.state == ElemState::Gas);
  PCHECK(cl.category == ElemCategory::ReactiveNonmetal && near(cl.electronegativity, 3.16f));

  const Element& ar = element(18);
  PCHECK(near(ar.mass, 39.948f) && ar.category == ElemCategory::NobleGas && streq(ar.config, "[Ne] 3s2 3p6"));

  const Element& k = element(19);
  PCHECK(streq(k.symbol, "K") && near(k.mass, 39.098f) && k.category == ElemCategory::AlkaliMetal);
  PCHECK(streq(k.config, "[Ar] 4s1") && k.period == 4 && k.group == 1);

  const Element& ca = element(20);
  PCHECK(near(ca.mass, 40.078f) && ca.category == ElemCategory::AlkalineEarth && streq(ca.config, "[Ar] 4s2"));

  PCHECK(streq(element(24).config, "[Ar] 3d5 4s1"));  // Cr exception

  const Element& fe = element(26);
  PCHECK(streq(fe.symbol, "Fe") && streq(fe.name, "Iron") && near(fe.mass, 55.845f));
  PCHECK(fe.group == 8 && fe.period == 4 && fe.block == ElemBlock::D && fe.category == ElemCategory::TransitionMetal);
  PCHECK(streq(fe.config, "[Ar] 3d6 4s2") && near(fe.melt, 1811.0f) && near(fe.boil, 3134.0f));
  PCHECK(near(fe.density, 7.874f) && near(fe.electronegativity, 1.83f) && near(fe.ionization, 762.5f, 0.5f));
  PCHECK((fe.oxMask & (1u << (3 + 4))) && (fe.oxMask & (1u << (2 + 4))));

  const Element& cu = element(29);
  PCHECK(streq(cu.config, "[Ar] 3d10 4s1") && near(cu.mass, 63.546f));  // Cu exception

  PCHECK(element(35).state == ElemState::Liquid && streq(element(35).symbol, "Br"));  // Br liquid

  const Element& ag = element(47);
  PCHECK(streq(ag.symbol, "Ag") && streq(ag.config, "[Kr] 4d10 5s1") && near(ag.mass, 107.87f));
  PCHECK(streq(element(46).config, "[Kr] 4d10"));  // Pd exception

  const Element& iod = element(53);
  PCHECK(streq(iod.symbol, "I") && iod.state == ElemState::Solid && near(iod.mass, 126.90f));
  PCHECK(iod.group == 17 && iod.category == ElemCategory::ReactiveNonmetal);

  const Element& xe = element(54);
  PCHECK(streq(xe.symbol, "Xe") && xe.state == ElemState::Gas && xe.category == ElemCategory::NobleGas);
  PCHECK(near(xe.mass, 131.29f));

  const Element& au = element(79);
  PCHECK(streq(au.symbol, "Au") && near(au.mass, 196.97f) && streq(au.config, "[Xe] 4f14 5d10 6s1"));
  PCHECK(au.group == 11 && au.period == 6 && au.year == 0);

  const Element& hg = element(80);
  PCHECK(streq(hg.symbol, "Hg") && hg.state == ElemState::Liquid && near(hg.melt, 234.32f));

  const Element& pb = element(82);
  PCHECK(near(pb.mass, 207.2f) && pb.category == ElemCategory::PostTransition && pb.group == 14);

  const Element& u = element(92);
  PCHECK(streq(u.symbol, "U") && near(u.mass, 238.03f) && u.category == ElemCategory::Actinide);
  PCHECK(!u.massIsNumber() && streq(u.config, "[Rn] 5f3 6d1 7s2") && u.block == ElemBlock::F);

  const Element& tc = element(43);
  char buf[64];
  formatMass(43, buf, sizeof buf);
  PCHECK(tc.massIsNumber() && streq(buf, "[98]"));
  formatMass(26, buf, sizeof buf);
  PCHECK(streq(buf, "55.845"));

  const Element& og = element(118);
  formatMass(118, buf, sizeof buf);
  PCHECK(streq(og.symbol, "Og") && og.massIsNumber() && streq(buf, "[294]") && og.predicted());
  PCHECK(og.group == 18 && og.period == 7 && og.state == ElemState::Unknown);
  PCHECK(gridPos(118).row == 6 && gridPos(118).col == 17);

  // Lu / Lr in group 3; La / Ac start the f-block rows.
  PCHECK(element(71).group == 3 && gridPos(71).row == 5 && gridPos(71).col == 2);
  PCHECK(element(103).group == 3 && gridPos(103).row == 6 && gridPos(103).col == 2);
  PCHECK(element(71).category == ElemCategory::Lanthanide && element(103).category == ElemCategory::Actinide);
  PCHECK(element(57).group == 0 && gridPos(57).row == 7 && gridPos(57).col == 2 && element(57).block == ElemBlock::F);
  PCHECK(element(89).group == 0 && gridPos(89).row == 8 && gridPos(89).col == 2);
  PCHECK(gridPos(70).row == 7 && gridPos(70).col == 15 && gridPos(102).row == 8 && gridPos(102).col == 15);
  PCHECK(streq(element(57).config, "[Xe] 5d1 6s2") && streq(element(89).config, "[Rn] 6d1 7s2"));

  // Formatting.
  formatConfig(26, buf, sizeof buf, true);
  PCHECK(streq(buf, "[Ar] 3d\xE2\x81\xB6 4s\xC2\xB2"));
  formatOxidation(26, buf, sizeof buf);
  PCHECK(streq(buf, "+3, +2"));
  formatOxidation(113, buf, sizeof buf);
  PCHECK(streq(buf, "\xE2\x80\x94"));
  formatTemperature(1811.0f, buf, sizeof buf);
  PCHECK(streq(buf, "1811 K (1538\xC2\xB0" "C)"));
  formatTemperature(13.81f, buf, sizeof buf);
  PCHECK(streq(buf, "13.81 K (-259.34\xC2\xB0" "C)"));
  formatTemperature(kElemNone, buf, sizeof buf);
  PCHECK(streq(buf, "\xE2\x80\x94"));
  formatDensity(1, buf, sizeof buf);
  PCHECK(streq(buf, "0.08988 g/L"));
  formatDensity(26, buf, sizeof buf);
  PCHECK(streq(buf, "7.874 g/cm\xC2\xB3"));
  PCHECK(streq(categoryName(ElemCategory::TransitionMetal), "Transition metal"));
  PCHECK(streq(blockName(ElemBlock::F), "f") && streq(stateName(ElemState::Liquid), "liquid"));
}

void testInvariants() {
  std::set<std::string> syms, names;
  std::set<int> cells;
  int blocks[4] = {};
  bool ordered = true, gridOk = true, posOk = true, blockOk = true, cfgOk = true, valuesOk = true;
  for (int i = 0; i < kElementCount; ++i) {
    const Element& e = kElements[i];
    if (e.z != i + 1) ordered = false;
    syms.insert(e.symbol);
    names.insert(e.name);
    const GridPos p = gridPos(e.z);
    if (p.row < 0 || p.row >= kGridRows || p.col < 0 || p.col >= kGridCols || elementAt(p.row, p.col) != e.z) {
      gridOk = false;
      std::printf("  grid: Z=%d\n", e.z);
    }
    cells.insert(p.row * kGridCols + p.col);
    // Position matches period/group.
    if (e.group > 0) {
      if (p.row != e.period - 1 || p.col != e.group - 1) posOk = false;
    } else {
      if (!((e.period == 6 && p.row == 7) || (e.period == 7 && p.row == 8))) posOk = false;
    }
    // Block from the group.
    ElemBlock want = e.group == 0 ? ElemBlock::F
                     : (e.group <= 2 || e.z == 2) ? ElemBlock::S
                     : e.group <= 12 ? ElemBlock::D : ElemBlock::P;
    if (e.block != want) {
      blockOk = false;
      std::printf("  block: Z=%d\n", e.z);
    }
    ++blocks[static_cast<int>(e.block)];
    if (electrons(e.config) != e.z) {
      cfgOk = false;
      std::printf("  config electrons: Z=%d %s -> %d\n", e.z, e.config, electrons(e.config));
    }
    if (!(e.mass > 0) || (e.year < 0) || (e.density == 0) || (e.melt == 0) || (e.boil == 0)) valuesOk = false;
    if (hasValue(e.melt) && hasValue(e.boil) && e.melt >= e.boil && e.z != 33) {  // As sublimes
      valuesOk = false;
      std::printf("  melt >= boil: Z=%d\n", e.z);
    }
    if (hasValue(e.electronegativity) && (e.electronegativity < 0.5f || e.electronegativity > 4.0f)) valuesOk = false;
  }
  PCHECK(ordered);
  PCHECK(syms.size() == 118 && names.size() == 118);
  PCHECK(gridOk);
  PCHECK(cells.size() == 118);  // no two share a cell
  PCHECK(posOk);
  PCHECK(blockOk);
  PCHECK(blocks[0] == 14 && blocks[1] == 36 && blocks[2] == 40 && blocks[3] == 28);
  PCHECK(cfgOk);
  PCHECK(valuesOk);
  PCHECK(kElementTableBytes == sizeof(Element) * 118);
  // Every period ends on a noble-gas column and starts on group 1.
  const int ends[] = {2, 10, 18, 36, 54, 86, 118};
  for (int z : ends) PCHECK(element(z).group == 18);
  // Counting cells: 7 periods hold 2, 8, 8, 18, 18, 18 (+14 in the f-row), 18 (+14).
  int perRow[kGridRows] = {};
  for (int r = 0; r < kGridRows; ++r)
    for (int c = 0; c < kGridCols; ++c) perRow[r] += elementAt(r, c) ? 1 : 0;
  PCHECK(perRow[0] == 2 && perRow[1] == 8 && perRow[2] == 8 && perRow[3] == 18 && perRow[4] == 18);
  PCHECK(perRow[5] == 18 && perRow[6] == 18 && perRow[7] == 14 && perRow[8] == 14);
  PCHECK(elementAt(-1, 0) == 0 && elementAt(0, 5) == 0 && elementAt(9, 0) == 0);
  // Detail text fits the 25-column screen for every element.
  bool fits = true;
  for (int z = 1; z <= kElementCount; ++z) {
    char line[128];
    const int nLines = detailLine(z, -1, nullptr, 0);
    if (nLines < 13) fits = false;
    for (int i = 0; i < nLines; ++i) {
      detailLine(z, i, line, sizeof line);
      if (cpLen(line) > 25 || line[0] == 0) {
        fits = false;
        std::printf("  detail line too long/empty: Z=%d [%s]\n", z, line);
      }
    }
  }
  PCHECK(fits);
}

void testNavigation() {
  PeriodicApp app;
  PCHECK(app.screen() == PeriodicApp::Screen::Table && app.cursor() == 1 && !app.wantsExit());
  press(app, DKey::Left);
  PCHECK(app.cursor() == 1);  // H: nothing to the left
  press(app, DKey::Up);
  PCHECK(app.cursor() == 1);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 2);  // H -> He across the gap
  press(app, DKey::Down);
  PCHECK(app.cursor() == 10);  // He -> Ne
  press(app, DKey::Right);
  PCHECK(app.cursor() == 11);  // Ne -> Na (wraps to the next row)
  press(app, DKey::Left);
  PCHECK(app.cursor() == 10);  // and back
  app.reset();
  press(app, DKey::Down);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 4);  // Li -> Be
  press(app, DKey::Right);
  PCHECK(app.cursor() == 5);  // Be -> B across the d-block gap
  press(app, DKey::Up);
  PCHECK(app.cursor() == 2);  // B -> He (nearest in period 1)
  // Down the group-3 column into the f-rows.
  app.reset();
  for (int i = 0; i < 3; ++i) press(app, DKey::Down);  // H Li Na K
  press(app, DKey::Right);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 21);  // Sc
  press(app, DKey::Down);
  PCHECK(app.cursor() == 39);  // Y
  press(app, DKey::Down);
  PCHECK(app.cursor() == 71);  // Lu
  press(app, DKey::Down);
  PCHECK(app.cursor() == 103);  // Lr
  press(app, DKey::Down);
  PCHECK(app.cursor() == 57);  // La (f-row)
  press(app, DKey::Down);
  PCHECK(app.cursor() == 89);  // Ac
  press(app, DKey::Down);
  PCHECK(app.cursor() == 89);  // bottom: stays
  press(app, DKey::Up);
  PCHECK(app.cursor() == 57);
  // Og -> Right goes to La (next row), No -> Right stays.
  goToZ(app, 118);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::Table && app.cursor() == 118);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 57);
  goToZ(app, 102);
  press(app, DKey::AC);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 102);
  // Every element is reachable with Right from H (reading order covers the grid).
  app.reset();
  std::set<int> seen = {1};
  for (int i = 0; i < 200; ++i) {
    press(app, DKey::Right);
    seen.insert(app.cursor());
  }
  PCHECK(seen.size() == 118);

  // Detail: = opens, left/right wrap, up/down scroll, AC back.
  app.reset();
  press(app, DKey::Eq);
  PCHECK(app.screen() == PeriodicApp::Screen::Detail && app.cursor() == 1);
  press(app, DKey::Left);
  PCHECK(app.cursor() == 118);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 1);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 2);
  press(app, DKey::Up);
  PCHECK(app.detailScroll() == 0);
  for (int i = 0; i < 40; ++i) press(app, DKey::Down);
  const int lines = detailLine(2, -1, nullptr, 0);
  PCHECK(app.detailScroll() == lines - PeriodicApp::kDetailRows);
  press(app, DKey::Right);
  PCHECK(app.detailScroll() == 0 && app.cursor() == 3);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::Table && app.cursor() == 3);
  press(app, DKey::AC);
  PCHECK(app.wantsExit());
  PCHECK(!app.onKey(DKey::Down, false, false));  // ignored once exiting
  app.reset();
  PCHECK(!app.wantsExit());

  // Help.
  press(app, DKey::D0);
  PCHECK(app.screen() == PeriodicApp::Screen::Help);
  press(app, DKey::Eq);
  PCHECK(app.screen() == PeriodicApp::Screen::Table);
}

void testSearch() {
  PeriodicApp app;
  goToZ(app, 26);
  PCHECK(app.screen() == PeriodicApp::Screen::Detail && app.cursor() == 26);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::Table && app.cursor() == 26);

  // 119 can't be typed; DEL undoes digits; AC goes back.
  app.reset();
  press(app, DKey::D3);
  PCHECK(app.screen() == PeriodicApp::Screen::Search);
  press(app, DKey::D1);
  press(app, DKey::D1);
  press(app, DKey::D9);
  PCHECK(app.searchNumber() == 11);
  press(app, DKey::D8);
  PCHECK(app.searchNumber() == 118 && app.searchMatch() == 118);
  press(app, DKey::Del);
  PCHECK(app.searchNumber() == 11);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::Table && app.searchNumber() == 0);
  press(app, DKey::D3);
  press(app, DKey::D0);  // a leading 0 is ignored
  PCHECK(app.searchNumber() == 0);
  press(app, DKey::AC);

  // Symbol: F then E -> Fe opens at once.
  app.reset();
  press(app, DKey::D3);
  for (int i = 0; i < 5; ++i) press(app, DKey::Right);  // A -> F
  PCHECK(app.searchCell() == 5);
  press(app, DKey::Eq);
  PCHECK(streq(app.searchLetters(), "F") && app.searchMatch() == 9);  // exact F
  press(app, DKey::Left);  // E
  press(app, DKey::Eq);
  PCHECK(app.screen() == PeriodicApp::Screen::Detail && app.cursor() == 26);

  // One letter + OK: C -> carbon. X + OK -> xenon (prefix). Q -> no match.
  app.reset();
  press(app, DKey::D3);
  press(app, DKey::Right);
  press(app, DKey::Right);  // C
  press(app, DKey::Eq);
  press(app, DKey::Left);
  press(app, DKey::Left);
  press(app, DKey::Left);  // wraps to OK (cell 26)
  PCHECK(app.searchCell() == 26);
  press(app, DKey::Eq);
  PCHECK(app.screen() == PeriodicApp::Screen::Detail && app.cursor() == 6);
  PCHECK(matchSymbolPrefix("X") == 54 && matchSymbolPrefix("Q") == 0 && matchSymbolPrefix("") == 0);
  PCHECK(findSymbol("fe") == 26 && findSymbol("FE") == 26 && findSymbol("Xx") == 0 && findSymbol("O") == 8);
  // Grid moves: Up/Down by a row, clamped.
  app.reset();
  press(app, DKey::D3);
  press(app, DKey::Up);
  PCHECK(app.searchCell() == 0);
  press(app, DKey::Down);
  press(app, DKey::Down);
  PCHECK(app.searchCell() == 18);
  press(app, DKey::Down);
  PCHECK(app.searchCell() == 18);
  // DEL undoes a letter; two letters that aren't a symbol stay put.
  press(app, DKey::Eq);  // S
  press(app, DKey::Eq);  // Ss: not a symbol
  PCHECK(app.screen() == PeriodicApp::Screen::Search && streq(app.searchLetters(), "Ss"));
  press(app, DKey::Del);
  PCHECK(streq(app.searchLetters(), "S"));
}

void testSortAndList() {
  PeriodicApp app;
  PCHECK(app.sort() == PeriodicApp::Sort::Number && app.listAt(0) == 1 && app.listAt(117) == 118);
  press(app, DKey::D2);  // table: 2 = list sorted the next way (by name)
  PCHECK(app.screen() == PeriodicApp::Screen::List && app.sort() == PeriodicApp::Sort::Name);
  PCHECK(app.listAt(0) == 89 && app.listAt(117) == 40);  // Actinium ... Zirconium
  bool alpha = true;
  for (int i = 1; i < kElementCount; ++i)
    if (std::strcmp(element(app.listAt(i - 1)).name, element(app.listAt(i)).name) > 0) alpha = false;
  PCHECK(alpha);
  press(app, DKey::D2);
  PCHECK(app.sort() == PeriodicApp::Sort::Symbol && app.listAt(0) == 89 && app.listAt(117) == 40);  // Ac ... Zr
  bool bySym = true;
  for (int i = 1; i < kElementCount; ++i)
    if (std::strcmp(element(app.listAt(i - 1)).symbol, element(app.listAt(i)).symbol) > 0) bySym = false;
  PCHECK(bySym);
  press(app, DKey::D2);
  PCHECK(app.sort() == PeriodicApp::Sort::Number);
  // The list follows the cursor; Down moves, AC returns to the table on the new element.
  PCHECK(app.cursor() == 1 && app.listPos() == 0);
  press(app, DKey::Up);
  PCHECK(app.cursor() == 118 && app.listTop() == 113);  // wraps
  press(app, DKey::Down);
  PCHECK(app.cursor() == 1 && app.listTop() == 0);
  for (int i = 0; i < 25; ++i) press(app, DKey::Down);
  PCHECK(app.cursor() == 26 && app.listTop() == 21);
  press(app, DKey::Right);
  PCHECK(app.cursor() == 31);
  press(app, DKey::Left);
  press(app, DKey::Left);
  PCHECK(app.cursor() == 21);
  press(app, DKey::Eq);
  PCHECK(app.screen() == PeriodicApp::Screen::Detail && app.cursor() == 21);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::List);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::Table && app.cursor() == 21);
  // Sorting keeps the selected element.
  press(app, DKey::D2);
  PCHECK(app.listAt(app.listPos()) == 21);
}

void testFilters() {
  PeriodicApp app;
  PCHECK(app.filter() == Filter::All && !app.highlighted(1));
  press(app, DKey::D4);
  PCHECK(app.filter() == Filter::AlkaliMetal && app.highlighted(3) && !app.highlighted(1));
  press(app, DKey::D4, true);
  PCHECK(app.filter() == Filter::All);
  press(app, DKey::D4, true);
  PCHECK(app.filter() == Filter::UnknownState);
  auto count = [](Filter f) {
    int n = 0;
    for (int z = 1; z <= kElementCount; ++z) n += matchesFilter(z, f) ? 1 : 0;
    return n;
  };
  PCHECK(count(Filter::All) == 118);
  PCHECK(count(Filter::AlkaliMetal) == 6 && count(Filter::AlkalineEarth) == 6);
  PCHECK(count(Filter::NobleGas) == 6 && count(Filter::Liquid) == 2 && count(Filter::Gas) == 11);
  PCHECK(count(Filter::Lanthanide) == 15 && count(Filter::Actinide) == 15);
  PCHECK(count(Filter::BlockF) == 28 && count(Filter::BlockS) == 14);
  PCHECK(count(Filter::UnknownCategory) == 10 && count(Filter::UnknownState) == 15);
  int total = 0;
  for (int f = static_cast<int>(Filter::AlkaliMetal); f <= static_cast<int>(Filter::UnknownCategory); ++f)
    total += count(static_cast<Filter>(f));
  PCHECK(total == 118);  // the categories partition the table
  // A full cycle returns to All, every filter has a name.
  app.reset();
  bool named = true;
  for (int i = 0; i < static_cast<int>(Filter::Count); ++i) {
    if (!filterName(app.filter())[0]) named = false;
    press(app, DKey::D4);
  }
  PCHECK(app.filter() == Filter::All && named);
}

void testGraph() {
  for (int p = 0; p < static_cast<int>(GraphProp::Count); ++p) {
    float lo, hi;
    graphRange(static_cast<GraphProp>(p), lo, hi);
    PCHECK(hi > lo && std::isfinite(lo) && std::isfinite(hi) && lo >= 0);
    PCHECK(graphPropName(static_cast<GraphProp>(p))[0] != 0);
  }
  float lo, hi;
  graphRange(GraphProp::Electronegativity, lo, hi);
  PCHECK(near(lo, 0.7f) && near(hi, 3.98f));
  graphRange(GraphProp::Mass, lo, hi);
  PCHECK(near(lo, 1.008f) && near(hi, 294.0f));
  float v;
  PCHECK(!propertyValue(2, GraphProp::Electronegativity, v) && propertyValue(26, GraphProp::Melting, v) && near(v, 1811));
  PCHECK(!propertyValue(118, GraphProp::Ionization, v) && !propertyValue(2, GraphProp::Radius, v));

  PeriodicApp app;
  press(app, DKey::D5);
  PCHECK(app.screen() == PeriodicApp::Screen::Graph && app.graphProp() == GraphProp::Electronegativity);
  press(app, DKey::Left);
  PCHECK(app.cursor() == 1);  // clamps at H
  press(app, DKey::Right, true);
  PCHECK(app.cursor() == 11);
  for (int i = 0; i < 20; ++i) press(app, DKey::Right, true);
  PCHECK(app.cursor() == 118);  // clamps at Og
  press(app, DKey::Up);
  PCHECK(app.graphProp() == GraphProp::Mass);  // wraps backwards
  press(app, DKey::Down);
  press(app, DKey::Down);
  PCHECK(app.graphProp() == GraphProp::Ionization);
  press(app, DKey::Eq);
  PCHECK(app.screen() == PeriodicApp::Screen::Detail);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::Graph);
  press(app, DKey::AC);
  PCHECK(app.screen() == PeriodicApp::Screen::Table && app.cursor() == 118);
}

void testRender() {
  static Framebuffer fb;
  PeriodicApp app;
  bool ok = true;
  auto draw = [&](int minPixels) {
    fb.clear();
    app.render(fb);
    if (pixelsIn(fb, 8, Framebuffer::kHeight - 1) < minPixels) ok = false;
    if (pixelsIn(fb, 0, 6) != 0) ok = false;  // never draws over the status bar text
  };
  for (int z = 1; z <= kElementCount; ++z) {
    goToZ(app, z);
    draw(50);  // detail
    for (int i = 0; i < 20; ++i) {
      press(app, DKey::Down);
      draw(20);
    }
    press(app, DKey::AC);
    draw(200);  // table
    press(app, DKey::D1);
    draw(100);  // list
    press(app, DKey::AC);
    press(app, DKey::D3);
    draw(100);  // search
    press(app, DKey::AC);
    press(app, DKey::D5);
    for (int p = 0; p < static_cast<int>(GraphProp::Count); ++p) {
      draw(100);
      press(app, DKey::Down);
    }
    press(app, DKey::AC);
    press(app, DKey::D0);
    draw(100);
  }
  // Every filter on the table, every sort in the list.
  app.reset();
  for (int f = 0; f < static_cast<int>(Filter::Count); ++f) {
    draw(200);
    press(app, DKey::D4);
  }
  for (int s = 0; s < 3; ++s) {
    press(app, DKey::D2);
    draw(100);
  }
  PCHECK(ok);

  // Table: the cursor cell is inverted and the footer shows "Fe 26 Iron 55.845".
  goToZ(app, 26);
  press(app, DKey::AC);
  fb.clear();
  app.render(fb);
  const GridPos p = gridPos(26);
  const int cx = 9 + p.col * 6, cy = 8 + p.row * 5;
  PCHECK(fb.get(cx - 1, cy - 1) && fb.get(cx + 2, cy + 1) && !fb.get(cx, cy));  // ring on, inside on, outline off
  PCHECK(pixelsIn(fb, 54, 60) > 40);
  Framebuffer ref;
  ref.drawTextPx(0, 54, "Fe 26 Iron 55.845");
  bool footer = true;
  for (int y = 54; y <= 60; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x)
      if (fb.get(x, y) != ref.get(x, y)) footer = false;
  PCHECK(footer);
  // Detail scroll arrows.
  goToZ(app, 26);
  fb.clear();
  app.render(fb);
  PCHECK(!fb.icon(Icon::Up) && fb.icon(Icon::Down));
}

}  // namespace

int runPeriodicTests() {
  g_fail = 0;
  g_checks = 0;
  testSpotChecks();
  testInvariants();
  testNavigation();
  testSearch();
  testSortAndList();
  testFilters();
  testGraph();
  testRender();
  std::printf("periodic: %d checks, %d failed; element table %u bytes (%u per element)\n", g_checks, g_fail,
              static_cast<unsigned>(kElementTableBytes), static_cast<unsigned>(sizeof(Element)));
  return g_fail;
}
