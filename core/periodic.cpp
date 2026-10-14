#include "periodic.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "font.h"

namespace calc {

// ============================================================================= data helpers

const Element& element(int z) {
  if (z < 1) z = 1;
  if (z > kElementCount) z = kElementCount;
  return kElements[z - 1];
}

const char* categoryName(ElemCategory c) {
  switch (c) {
    case ElemCategory::AlkaliMetal: return "Alkali metal";
    case ElemCategory::AlkalineEarth: return "Alkaline earth metal";
    case ElemCategory::TransitionMetal: return "Transition metal";
    case ElemCategory::PostTransition: return "Post-transition metal";
    case ElemCategory::Metalloid: return "Metalloid";
    case ElemCategory::ReactiveNonmetal: return "Reactive nonmetal";
    case ElemCategory::NobleGas: return "Noble gas";
    case ElemCategory::Lanthanide: return "Lanthanide";
    case ElemCategory::Actinide: return "Actinide";
    default: return "Unknown properties";
  }
}

const char* blockName(ElemBlock b) {
  switch (b) {
    case ElemBlock::S: return "s";
    case ElemBlock::P: return "p";
    case ElemBlock::D: return "d";
    default: return "f";
  }
}

const char* stateName(ElemState s) {
  switch (s) {
    case ElemState::Solid: return "solid";
    case ElemState::Liquid: return "liquid";
    case ElemState::Gas: return "gas";
    default: return "unknown";
  }
}

GridPos gridPos(int z) {
  const Element& e = element(z);
  if (e.group > 0) return {static_cast<int8_t>(e.period - 1), static_cast<int8_t>(e.group - 1)};
  const int first = e.period == 6 ? 57 : 89;
  return {static_cast<int8_t>(e.period == 6 ? 7 : 8), static_cast<int8_t>(2 + e.z - first)};
}

int elementAt(int row, int col) {
  if (row < 0 || row >= kGridRows || col < 0 || col >= kGridCols) return 0;
  // Small enough to scan; built once into a lookup table.
  static uint8_t grid[kGridRows][kGridCols];
  static bool built = false;
  if (!built) {
    for (int z = 1; z <= kElementCount; ++z) {
      const GridPos p = gridPos(z);
      grid[p.row][p.col] = static_cast<uint8_t>(z);
    }
    built = true;
  }
  return grid[row][col];
}

namespace {

char upper(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 32) : c; }

bool symbolEq(const char* a, const char* b) {
  while (*a && *b) {
    if (upper(*a) != upper(*b)) return false;
    ++a;
    ++b;
  }
  return *a == 0 && *b == 0;
}

bool symbolStarts(const char* sym, const char* prefix) {
  for (; *prefix; ++prefix, ++sym)
    if (!*sym || upper(*sym) != upper(*prefix)) return false;
  return true;
}

// Appends with truncation; keeps out NUL-terminated.
void append(char* out, size_t n, const char* s) {
  size_t len = std::strlen(out);
  while (*s && len + 1 < n) out[len++] = *s++;
  out[len] = 0;
}

// %g-style number without a trailing ".0" or exponent for normal ranges.
void fmtNum(char* out, size_t n, double v, int sig = 6) {
  std::snprintf(out, n, "%.*g", sig, v);
}

int decimalsOf(float v) {
  for (int d = 0; d < 3; ++d) {
    const double s = std::pow(10.0, d) * v;
    if (std::fabs(s - std::round(s)) < 1e-3 * std::pow(10.0, d)) return d;
  }
  return 3;
}

// Number of code points in a UTF-8 string.
int cpLen(const char* s) {
  int n = 0;
  for (; *s; ++s)
    if ((static_cast<unsigned char>(*s) & 0xC0) != 0x80) ++n;
  return n;
}

// Byte offset of code point `cp` in s (or strlen).
size_t cpOffset(const char* s, int cp) {
  size_t i = 0;
  int n = 0;
  for (; s[i]; ++i) {
    if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
      if (n == cp) return i;
      ++n;
    }
  }
  return i;
}

const char* const kSuperDigits[10] = {"\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3", "\xE2\x81\xB4",
                                      "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7", "\xE2\x81\xB8", "\xE2\x81\xB9"};
const char* const kDash = "\xE2\x80\x94";  // em dash: "no value"

}  // namespace

int findSymbol(const char* sym) {
  if (!sym || !*sym) return 0;
  for (int i = 0; i < kElementCount; ++i)
    if (symbolEq(kElements[i].symbol, sym)) return i + 1;
  return 0;
}

int matchSymbolPrefix(const char* prefix) {
  if (!prefix || !*prefix) return 0;
  if (int z = findSymbol(prefix)) return z;
  for (int i = 0; i < kElementCount; ++i)
    if (symbolStarts(kElements[i].symbol, prefix)) return i + 1;
  return 0;
}

void formatMass(int z, char* out, size_t n) {
  const Element& e = element(z);
  if (e.massIsNumber()) {
    std::snprintf(out, n, "[%d]", static_cast<int>(e.mass + 0.5f));
  } else {
    fmtNum(out, n, e.mass, 6);
  }
}

void formatConfig(int z, char* out, size_t n, bool superscripts) {
  if (n == 0) return;
  out[0] = 0;
  const char* c = element(z).config;
  if (!superscripts) {
    append(out, n, c);
    return;
  }
  // Digits right after an orbital letter (s/p/d/f) are electron counts.
  bool count = false;
  for (; *c; ++c) {
    const char ch = *c;
    if (ch == 's' || ch == 'p' || ch == 'd' || ch == 'f') {
      char b[2] = {ch, 0};
      append(out, n, b);
      count = true;
    } else if (count && ch >= '0' && ch <= '9') {
      append(out, n, kSuperDigits[ch - '0']);
    } else {
      count = false;
      char b[2] = {ch, 0};
      append(out, n, b);
    }
  }
}

void formatOxidation(int z, char* out, size_t n) {
  if (n == 0) return;
  out[0] = 0;
  const uint16_t m = element(z).oxMask;
  if (!m) {
    append(out, n, kDash);
    return;
  }
  bool first = true;
  for (int s = 11; s >= -4; --s) {
    if (!(m & (1u << (s + 4)))) continue;
    char b[8];
    if (s == 0)
      std::snprintf(b, sizeof b, "0");
    else
      std::snprintf(b, sizeof b, "%+d", s);
    if (!first) append(out, n, ", ");
    append(out, n, b);
    first = false;
  }
}

void formatTemperature(float kelvin, char* out, size_t n) {
  if (!hasValue(kelvin)) {
    std::snprintf(out, n, "%s", kDash);
    return;
  }
  const int d = decimalsOf(kelvin);
  std::snprintf(out, n, "%.*f K (%.*f\xC2\xB0" "C)", d, kelvin, d, kelvin - 273.15);
}

void formatDensity(int z, char* out, size_t n) {
  const Element& e = element(z);
  if (!hasValue(e.density)) {
    std::snprintf(out, n, "%s", kDash);
    return;
  }
  char num[24];
  if (e.state == ElemState::Gas) {
    fmtNum(num, sizeof num, e.density * 1000.0, 4);
    std::snprintf(out, n, "%s g/L", num);
  } else {
    fmtNum(num, sizeof num, e.density, 6);
    std::snprintf(out, n, "%s g/cm\xC2\xB3", num);
  }
}

namespace {

constexpr int kDetailItems = 13;
constexpr size_t kItemBytes = 96;

// One logical (unwrapped) detail item.
void detailItem(int z, int i, char* out, size_t n) {
  const Element& e = element(z);
  char v[64];
  out[0] = 0;
  switch (i) {
    case 0:
      formatMass(z, v, sizeof v);
      std::snprintf(out, n, "Mass %s%s", v, e.massIsNumber() ? " (mass no.)" : " u");
      break;
    case 1:
      if (e.group)
        std::snprintf(out, n, "Group %d   Period %d", e.group, e.period);
      else
        std::snprintf(out, n, "Group %s   Period %d", kDash, e.period);
      break;
    case 2:
      if (e.state == ElemState::Unknown)
        std::snprintf(out, n, "%s-block   state unknown", blockName(e.block));
      else
        std::snprintf(out, n, "%s-block   %s at 25\xC2\xB0" "C", blockName(e.block), stateName(e.state));
      break;
    case 3: std::snprintf(out, n, "%s", categoryName(e.category)); break;
    case 4:
      formatConfig(z, v, sizeof v, true);
      std::snprintf(out, n, "Config %s%s", v, e.predicted() ? " (predicted)" : "");
      break;
    case 5:
      if (hasValue(e.electronegativity)) {
        fmtNum(v, sizeof v, e.electronegativity, 3);
        std::snprintf(out, n, "Electronegativity %s", v);
      } else {
        std::snprintf(out, n, "Electronegativity %s", kDash);
      }
      break;
    case 6:
      if (hasValue(e.ionization)) {
        fmtNum(v, sizeof v, e.ionization, 6);
        std::snprintf(out, n, "Ionization %s kJ/mol", v);
      } else {
        std::snprintf(out, n, "Ionization %s", kDash);
      }
      break;
    case 7:
      if (e.radius)
        std::snprintf(out, n, "Atomic radius %d pm", e.radius);
      else
        std::snprintf(out, n, "Atomic radius %s", kDash);
      break;
    case 8:
      formatDensity(z, v, sizeof v);
      std::snprintf(out, n, "Density %s", v);
      break;
    case 9:
      formatTemperature(e.melt, v, sizeof v);
      std::snprintf(out, n, "Melts %s", v);
      break;
    case 10:
      formatTemperature(e.boil, v, sizeof v);
      std::snprintf(out, n, "Boils %s", v);
      break;
    case 11:
      if (e.year)
        std::snprintf(out, n, "Discovered %d", e.year);
      else
        std::snprintf(out, n, "Discovered: ancient");
      break;
    default:
      formatOxidation(z, v, sizeof v);
      std::snprintf(out, n, "Oxidation %s%s", v, e.predicted() && e.oxMask ? " (predicted)" : "");
      break;
  }
}

}  // namespace

int detailLine(int z, int index, char* out, size_t n, int width) {
  if (n) out[0] = 0;
  if (width < 8) width = 8;
  int line = 0;
  char item[kItemBytes];
  for (int i = 0; i < kDetailItems; ++i) {
    detailItem(z, i, item, sizeof item);
    const char* s = item;
    bool firstPiece = true;
    while (true) {
      const int avail = firstPiece ? width : width - 2;  // continuation lines are indented 2
      const int len = cpLen(s);
      int take = len;
      if (len > avail) {
        // Break at the last space that fits, else hard-break.
        take = avail;
        for (int c = avail; c > 0; --c) {
          if (s[cpOffset(s, c)] == ' ') {
            take = c;
            break;
          }
        }
      }
      if (line == index && n) {
        out[0] = 0;
        if (!firstPiece) append(out, n, "  ");
        const size_t bytes = cpOffset(s, take);
        size_t len0 = std::strlen(out);
        for (size_t b = 0; b < bytes && len0 + 1 < n; ++b) out[len0++] = s[b];
        out[len0] = 0;
      }
      ++line;
      s += cpOffset(s, take);
      while (*s == ' ') ++s;
      firstPiece = false;
      if (!*s) break;
    }
  }
  return line;
}

// ============================================================================= filters

const char* filterName(Filter f) {
  switch (f) {
    case Filter::All: return "All elements";
    case Filter::AlkaliMetal: return "Alkali metals";
    case Filter::AlkalineEarth: return "Alkaline earth metals";
    case Filter::TransitionMetal: return "Transition metals";
    case Filter::PostTransition: return "Post-trans. metals";
    case Filter::Metalloid: return "Metalloids";
    case Filter::ReactiveNonmetal: return "Reactive nonmetals";
    case Filter::NobleGas: return "Noble gases";
    case Filter::Lanthanide: return "Lanthanides";
    case Filter::Actinide: return "Actinides";
    case Filter::UnknownCategory: return "Unknown properties";
    case Filter::BlockS: return "s-block";
    case Filter::BlockP: return "p-block";
    case Filter::BlockD: return "d-block";
    case Filter::BlockF: return "f-block";
    case Filter::Solid: return "Solid at 25\xC2\xB0" "C";
    case Filter::Liquid: return "Liquid at 25\xC2\xB0" "C";
    case Filter::Gas: return "Gas at 25\xC2\xB0" "C";
    default: return "State unknown";
  }
}

bool matchesFilter(int z, Filter f) {
  const Element& e = element(z);
  const int fi = static_cast<int>(f);
  if (f == Filter::All) return true;
  if (fi >= static_cast<int>(Filter::AlkaliMetal) && fi <= static_cast<int>(Filter::UnknownCategory))
    return static_cast<int>(e.category) == fi - static_cast<int>(Filter::AlkaliMetal);
  switch (f) {
    case Filter::BlockS: return e.block == ElemBlock::S;
    case Filter::BlockP: return e.block == ElemBlock::P;
    case Filter::BlockD: return e.block == ElemBlock::D;
    case Filter::BlockF: return e.block == ElemBlock::F;
    case Filter::Solid: return e.state == ElemState::Solid;
    case Filter::Liquid: return e.state == ElemState::Liquid;
    case Filter::Gas: return e.state == ElemState::Gas;
    case Filter::UnknownState: return e.state == ElemState::Unknown;
    default: return false;
  }
}

// ============================================================================= graph

const char* graphPropName(GraphProp p) {
  switch (p) {
    case GraphProp::Electronegativity: return "Electroneg.";
    case GraphProp::Ionization: return "Ioniz. kJ/mol";
    case GraphProp::Radius: return "Radius pm";
    case GraphProp::Density: return "Density g/cm\xC2\xB3";
    case GraphProp::Melting: return "Melting pt K";
    case GraphProp::Boiling: return "Boiling pt K";
    default: return "Atomic mass";
  }
}

bool propertyValue(int z, GraphProp p, float& v) {
  const Element& e = element(z);
  switch (p) {
    case GraphProp::Electronegativity: v = e.electronegativity; break;
    case GraphProp::Ionization: v = e.ionization; break;
    case GraphProp::Radius: v = e.radius ? static_cast<float>(e.radius) : kElemNone; break;
    case GraphProp::Density: v = e.density; break;
    case GraphProp::Melting: v = e.melt; break;
    case GraphProp::Boiling: v = e.boil; break;
    default: v = e.mass; break;
  }
  return hasValue(v);
}

void graphRange(GraphProp p, float& lo, float& hi) {
  bool any = false;
  lo = hi = 0;
  for (int z = 1; z <= kElementCount; ++z) {
    float v;
    if (!propertyValue(z, p, v)) continue;
    if (!any || v < lo) lo = v;
    if (!any || v > hi) hi = v;
    any = true;
  }
  if (!any) {
    lo = 0;
    hi = 1;
  } else if (!(hi > lo)) {
    hi = lo + 1;
  }
}

// ============================================================================= the app

namespace {

constexpr int kTableX = 9;   // left of column 0
constexpr int kTableY = 8;   // top of row 0
constexpr int kPitchX = 6;   // 5 px cell + 1 px gap
constexpr int kPitchY = 5;   // 4 px cell + 1 px gap
constexpr int kFRowGap = 1;  // extra space above the f-block rows
constexpr int kFooterY = 54;

int cellX(int col) { return kTableX + col * kPitchX; }
int cellY(int row) { return kTableY + row * kPitchY + (row >= 7 ? kFRowGap : 0); }

// Graph geometry.
constexpr int kPlotX0 = 3;   // x of Z = 1 (Z = 118 at x 120)
constexpr int kPlotTop = 17;
constexpr int kPlotBottom = 51;

int plotY(float v, float lo, float hi) {
  const float t = (v - lo) / (hi - lo);
  int y = kPlotBottom - static_cast<int>(t * (kPlotBottom - kPlotTop) + 0.5f);
  if (y < kPlotTop) y = kPlotTop;
  if (y > kPlotBottom) y = kPlotBottom;
  return y;
}

bool isDigit(DKey k) {
  switch (k) {
    case DKey::D0: case DKey::D1: case DKey::D2: case DKey::D3: case DKey::D4:
    case DKey::D5: case DKey::D6: case DKey::D7: case DKey::D8: case DKey::D9: return true;
    default: return false;
  }
}

int digitOf(DKey k) {
  switch (k) {
    case DKey::D1: return 1;
    case DKey::D2: return 2;
    case DKey::D3: return 3;
    case DKey::D4: return 4;
    case DKey::D5: return 5;
    case DKey::D6: return 6;
    case DKey::D7: return 7;
    case DKey::D8: return 8;
    case DKey::D9: return 9;
    default: return 0;
  }
}

bool lessBy(PeriodicApp::Sort s, int a, int b) {
  const Element& x = element(a);
  const Element& y = element(b);
  int c = 0;
  if (s == PeriodicApp::Sort::Name) c = std::strcmp(x.name, y.name);
  if (s == PeriodicApp::Sort::Symbol) c = std::strcmp(x.symbol, y.symbol);
  return c != 0 ? c < 0 : a < b;
}

}  // namespace

void PeriodicApp::reset() {
  screen_ = back_ = Screen::Table;
  cursor_ = 1;
  filter_ = Filter::All;
  scroll_ = 0;
  graphProp_ = GraphProp::Electronegativity;
  letters_[0] = letters_[1] = letters_[2] = 0;
  number_ = 0;
  searchCell_ = 0;
  exit_ = false;
  setSort(Sort::Number);
}

void PeriodicApp::setSort(Sort s) {
  sort_ = s;
  for (int i = 0; i < kElementCount; ++i) order_[i] = static_cast<uint8_t>(i + 1);
  // Insertion sort: 118 items, no allocation.
  for (int i = 1; i < kElementCount; ++i) {
    const uint8_t v = order_[i];
    int j = i - 1;
    while (j >= 0 && lessBy(s, v, order_[j])) {
      order_[j + 1] = order_[j];
      --j;
    }
    order_[j + 1] = v;
  }
  syncList();
}

void PeriodicApp::syncList() {
  for (int i = 0; i < kElementCount; ++i)
    if (order_[i] == cursor_) listPos_ = i;
  if (listPos_ < listTop_) listTop_ = listPos_;
  if (listPos_ >= listTop_ + kListRows) listTop_ = listPos_ - kListRows + 1;
  if (listTop_ > kElementCount - kListRows) listTop_ = kElementCount - kListRows;
  if (listTop_ < 0) listTop_ = 0;
}

void PeriodicApp::moveTable(int dRow, int dCol) {
  const GridPos p = gridPos(cursor_);
  if (dCol != 0) {
    // Next occupied cell in the row; past the end, continue on the next/previous row
    // (reading order), stopping at H and Lr/No's row ends... i.e. at the grid corners.
    int row = p.row, col = p.col;
    while (true) {
      col += dCol;
      if (col < 0 || col >= kGridCols) {
        row += dCol;
        if (row < 0 || row >= kGridRows) return;  // H going left / last f-row cell going right: stay
        col = dCol > 0 ? 0 : kGridCols - 1;
      }
      if (int z = elementAt(row, col)) {
        cursor_ = z;
        return;
      }
    }
  }
  if (dRow != 0) {
    const int row = p.row + dRow;
    if (row < 0 || row >= kGridRows) return;
    // Same column if occupied, else the nearest occupied column (ties go left).
    for (int d = 0; d < kGridCols; ++d) {
      if (int z = elementAt(row, p.col - d)) {
        cursor_ = z;
        return;
      }
      if (int z = elementAt(row, p.col + d)) {
        cursor_ = z;
        return;
      }
    }
  }
}

void PeriodicApp::openDetail(Screen from) {
  back_ = from;
  screen_ = Screen::Detail;
  scroll_ = 0;
}

void PeriodicApp::jumpTo(int z) {
  if (z < 1 || z > kElementCount) return;
  cursor_ = z;
  syncList();
  letters_[0] = letters_[1] = 0;
  number_ = 0;
  openDetail(back_);
}

int PeriodicApp::searchMatch() const {
  if (number_ > 0) return number_ <= kElementCount ? number_ : 0;
  return matchSymbolPrefix(letters_);
}

void PeriodicApp::searchKey(DKey k) {
  if (isDigit(k)) {
    const int v = number_ * 10 + digitOf(k);
    if (v >= 1 && v <= kElementCount) number_ = v;  // ignore digits that overflow 118 or a leading 0
    letters_[0] = letters_[1] = 0;
    return;
  }
  switch (k) {
    case DKey::Left: searchCell_ = (searchCell_ + kSearchCells - 1) % kSearchCells; break;
    case DKey::Right: searchCell_ = (searchCell_ + 1) % kSearchCells; break;
    case DKey::Up:
      if (searchCell_ >= kSearchCols) searchCell_ -= kSearchCols;
      break;
    case DKey::Down:
      if (searchCell_ + kSearchCols < kSearchCells) searchCell_ += kSearchCols;
      break;
    case DKey::Del:
      if (number_ > 0) {
        number_ /= 10;
      } else if (letters_[1]) {
        letters_[1] = 0;
      } else {
        letters_[0] = 0;
      }
      break;
    case DKey::AC:
      letters_[0] = letters_[1] = 0;
      number_ = 0;
      screen_ = back_;
      break;
    case DKey::Eq: {
      if (number_ > 0) {
        jumpTo(number_);
        break;
      }
      if (searchCell_ == kSearchCells - 1) {  // OK
        if (int z = searchMatch()) jumpTo(z);
        break;
      }
      const char letter = static_cast<char>('A' + searchCell_);
      if (!letters_[0]) {
        letters_[0] = letter;
      } else {
        letters_[1] = static_cast<char>(letter + 32);  // second letter is lower case
        if (int z = findSymbol(letters_)) jumpTo(z);  // a full two-letter symbol: go
      }
      break;
    }
    default: break;
  }
}

bool PeriodicApp::onKey(DKey k, bool shift, bool alpha) {
  (void)alpha;  // no letter entry via ALPHA on every build: the search grid covers it
  if (exit_) return false;
  const int beforeCursor = cursor_;
  const Screen beforeScreen = screen_;
  const int beforeScroll = scroll_;

  switch (screen_) {
    case Screen::Table:
      switch (k) {
        case DKey::Up: moveTable(-1, 0); break;
        case DKey::Down: moveTable(1, 0); break;
        case DKey::Left: moveTable(0, -1); break;
        case DKey::Right: moveTable(0, 1); break;
        case DKey::Eq: openDetail(Screen::Table); break;
        case DKey::AC: exit_ = true; return true;
        case DKey::D1: syncList(); screen_ = Screen::List; break;
        case DKey::D2:
          setSort(static_cast<Sort>((static_cast<int>(sort_) + 1) % static_cast<int>(Sort::Count)));
          screen_ = Screen::List;
          break;
        case DKey::D3: back_ = Screen::Table; screen_ = Screen::Search; break;
        case DKey::D4: {
          const int n = static_cast<int>(Filter::Count);
          filter_ = static_cast<Filter>((static_cast<int>(filter_) + (shift ? n - 1 : 1)) % n);
          return true;
        }
        case DKey::D5: screen_ = Screen::Graph; break;
        case DKey::D0: back_ = Screen::Table; screen_ = Screen::Help; break;
        default: return false;
      }
      break;

    case Screen::List:
      switch (k) {
        case DKey::Up: listPos_ = (listPos_ + kElementCount - 1) % kElementCount; break;
        case DKey::Down: listPos_ = (listPos_ + 1) % kElementCount; break;
        case DKey::Left: listPos_ = listPos_ >= kListRows ? listPos_ - kListRows : 0; break;
        case DKey::Right:
          listPos_ = listPos_ + kListRows < kElementCount ? listPos_ + kListRows : kElementCount - 1;
          break;
        case DKey::Eq: openDetail(Screen::List); break;
        case DKey::AC:
        case DKey::D1: screen_ = Screen::Table; break;
        case DKey::D2:
          setSort(static_cast<Sort>((static_cast<int>(sort_) + 1) % static_cast<int>(Sort::Count)));
          return true;
        case DKey::D3: back_ = Screen::List; screen_ = Screen::Search; break;
        case DKey::D5: screen_ = Screen::Graph; break;
        case DKey::D0: back_ = Screen::List; screen_ = Screen::Help; break;
        default: return false;
      }
      if (screen_ == Screen::List) {
        cursor_ = order_[listPos_];
        syncList();
      }
      return true;

    case Screen::Detail: {
      const int lines = detailLine(cursor_, -1, nullptr, 0);
      const int maxScroll = lines > kDetailRows ? lines - kDetailRows : 0;
      switch (k) {
        case DKey::Up: if (scroll_ > 0) --scroll_; break;
        case DKey::Down: if (scroll_ < maxScroll) ++scroll_; break;
        case DKey::Left: cursor_ = cursor_ > 1 ? cursor_ - 1 : kElementCount; scroll_ = 0; break;
        case DKey::Right: cursor_ = cursor_ < kElementCount ? cursor_ + 1 : 1; scroll_ = 0; break;
        case DKey::AC:
        case DKey::Eq: screen_ = back_; back_ = Screen::Table; break;
        default: return false;
      }
      syncList();
      break;
    }

    case Screen::Search: {
      const int n0 = number_;
      const char l0 = letters_[0], l1 = letters_[1];
      const int c0 = searchCell_;
      searchKey(k);
      return screen_ != beforeScreen || number_ != n0 || letters_[0] != l0 || letters_[1] != l1 ||
             searchCell_ != c0 || cursor_ != beforeCursor;
    }

    case Screen::Graph: {
      const int step = shift ? 10 : 1;
      const int nProps = static_cast<int>(GraphProp::Count);
      switch (k) {
        case DKey::Left: cursor_ = cursor_ - step < 1 ? 1 : cursor_ - step; break;
        case DKey::Right: cursor_ = cursor_ + step > kElementCount ? kElementCount : cursor_ + step; break;
        case DKey::Up: graphProp_ = static_cast<GraphProp>((static_cast<int>(graphProp_) + nProps - 1) % nProps); return true;
        case DKey::Down: graphProp_ = static_cast<GraphProp>((static_cast<int>(graphProp_) + 1) % nProps); return true;
        case DKey::Eq: openDetail(Screen::Graph); break;
        case DKey::AC: screen_ = Screen::Table; break;
        default: return false;
      }
      syncList();
      break;
    }

    case Screen::Help:
      screen_ = back_;
      back_ = Screen::Table;
      return true;
  }
  return cursor_ != beforeCursor || screen_ != beforeScreen || scroll_ != beforeScroll;
}

// ============================================================================= render

void PeriodicApp::render(Framebuffer& fb) const {
  fb.setIcon(Icon::Up, false);
  fb.setIcon(Icon::Down, false);
  switch (screen_) {
    case Screen::Table: drawTable(fb); break;
    case Screen::Detail: drawDetail(fb); break;
    case Screen::List: drawList(fb); break;
    case Screen::Search: drawSearch(fb); break;
    case Screen::Graph: drawGraph(fb); break;
    case Screen::Help: drawHelp(fb); break;
  }
}

void PeriodicApp::drawTable(Framebuffer& fb) const {
  for (int z = 1; z <= kElementCount; ++z) {
    const GridPos p = gridPos(z);
    const int x = cellX(p.col), y = cellY(p.row);
    if (highlighted(z)) {
      fb.fillRect(x, y, 5, 4, true);
    } else {
      fb.fillRect(x, y, 5, 1, true);
      fb.fillRect(x, y + 3, 5, 1, true);
      fb.fillRect(x, y, 1, 4, true);
      fb.fillRect(x + 4, y, 1, 4, true);
    }
  }
  // Cursor: invert the cell plus its 1 px gap ring (a 7x6 block), distinct from both cell styles.
  const GridPos c = gridPos(cursor_);
  fb.invertRect(cellX(c.col) - 1, cellY(c.row) - 1, 7, 6);

  // The empty box between groups 2 and 13 (rows 0-2, columns 2-11): filter or key hints.
  const int bx = cellX(2), by = kTableY;
  if (filter_ != Filter::All) {
    // Up to two 12-character lines, broken at a space.
    const char* name = filterName(filter_);
    const int len = cpLen(name);
    int brk = len;
    if (len > 12) {
      brk = 12;
      for (int i = 12; i > 0; --i)
        if (name[cpOffset(name, i)] == ' ') {
          brk = i;
          break;
        }
    }
    fb.drawTextPx(bx, by, std::string(name, cpOffset(name, brk)));
    const char* rest = name + cpOffset(name, brk);
    while (*rest == ' ') ++rest;
    if (*rest) fb.drawTextPx(bx, by + 8, rest);
  } else {
    fb.drawTextPx(bx, by, "0Help 1List");
    fb.drawTextPx(bx, by + 8, "3Find 5Graph");
  }

  // Footer: "Fe 26 Iron 55.845"
  const Element& e = element(cursor_);
  char mass[16], line[48];
  formatMass(cursor_, mass, sizeof mass);
  std::snprintf(line, sizeof line, "%s %d %s %s", e.symbol, e.z, e.name, mass);
  if (cpLen(line) > Framebuffer::kCols) std::snprintf(line, sizeof line, "%s %d %s", e.symbol, e.z, e.name);
  fb.drawTextPx(0, kFooterY, line);
}

void PeriodicApp::drawDetail(Framebuffer& fb) const {
  const Element& e = element(cursor_);
  char line[kItemBytes];
  std::snprintf(line, sizeof line, "%d %s %s", e.z, e.symbol, e.name);
  fb.drawText(0, 1, line);
  fb.drawText(23, 1, "\xE2\x86\x90\xE2\x86\x92");  // left/right arrows: previous/next element
  fb.invertRect(0, 8, Framebuffer::kWidth, 8);
  const int lines = detailLine(cursor_, -1, nullptr, 0);
  for (int r = 0; r < kDetailRows; ++r) {
    const int i = scroll_ + r;
    if (i >= lines) break;
    detailLine(cursor_, i, line, sizeof line);
    fb.drawText(0, 2 + r, line);
  }
  fb.setIcon(Icon::Up, scroll_ > 0);
  fb.setIcon(Icon::Down, scroll_ + kDetailRows < lines);
}

void PeriodicApp::drawList(Framebuffer& fb) const {
  static const char* const kSortNames[] = {"number", "name", "symbol"};
  char line[64];
  std::snprintf(line, sizeof line, "LIST by %s", kSortNames[static_cast<int>(sort_)]);
  fb.drawText(0, 1, line, true);
  fb.drawText(19, 1, "2=sort");
  for (int r = 0; r < kListRows; ++r) {
    const int i = listTop_ + r;
    if (i >= kElementCount) break;
    const int z = order_[i];
    const Element& e = element(z);
    char name[40], mass[16];
    std::snprintf(name, sizeof name, "%s", e.name);
    if (cpLen(name) > 10) std::snprintf(name, sizeof name, "%.9s\xE2\x80\xA6", e.name);  // 9 chars + "…"
    std::snprintf(mass, sizeof mass, "%.5g", e.mass);
    if (e.massIsNumber()) std::snprintf(mass, sizeof mass, "[%d]", static_cast<int>(e.mass + 0.5f));
    // "%3d %-2s %-10s %6s" with a UTF-8-aware name pad
    std::snprintf(line, sizeof line, "%3d %-2s %s", z, e.symbol, name);
    int pad = 10 - cpLen(name);
    while (pad-- > 0) append(line, sizeof line, " ");
    char m[16];
    std::snprintf(m, sizeof m, " %6s", mass);
    append(line, sizeof line, m);
    fb.drawText(0, 2 + r, line);
    if (i == listPos_) fb.invertRect(0, (2 + r) * 8, Framebuffer::kWidth, 8);
  }
  fb.setIcon(Icon::Up, listTop_ > 0);
  fb.setIcon(Icon::Down, listTop_ + kListRows < kElementCount);
}

void PeriodicApp::drawSearch(Framebuffer& fb) const {
  fb.drawText(0, 1, "FIND: number or symbol", true);
  char line[64];
  if (number_ > 0)
    std::snprintf(line, sizeof line, "Z: %d_", number_);
  else
    std::snprintf(line, sizeof line, "Sym: %s_", letters_);
  const int z = searchMatch();
  if (z) {
    const Element& e = element(z);
    char right[48];
    std::snprintf(right, sizeof right, "  %s %s", e.symbol, e.name);
    append(line, sizeof line, right);
  } else if (letters_[0]) {
    append(line, sizeof line, "  no match");
  }
  fb.drawTextPx(0, 17, line);
  // A-Z + OK grid: 9 cells per row, 13 px pitch.
  for (int i = 0; i < kSearchCells; ++i) {
    const int cx = 4 + (i % kSearchCols) * 13;
    const int cy = 27 + (i / kSearchCols) * 9;
    if (i < 26) {
      char s[2] = {static_cast<char>('A' + i), 0};
      fb.drawTextPx(cx + 4, cy, s);
    } else {
      fb.drawTextPx(cx + 2, cy, "OK");
    }
    if (i == searchCell_) fb.invertRect(cx, cy - 1, 13, 9);
  }
  fb.drawTextPx(0, kFooterY, "=pick DEL undo AC back");
}

void PeriodicApp::drawGraph(Framebuffer& fb) const {
  float lo, hi;
  graphRange(graphProp_, lo, hi);
  char a[16], b[16], line[64];
  auto axis = [](char* out, size_t n, float v) {  // "2372", "3.98", "9e-05"
    if (v >= 100.0f)
      std::snprintf(out, n, "%.0f", v);
    else if (v >= 0.01f || v == 0.0f)
      std::snprintf(out, n, "%.3g", v);
    else
      std::snprintf(out, n, "%.0e", v);
  };
  axis(a, sizeof a, lo);
  axis(b, sizeof b, hi);
  fb.drawTextPx(0, 8, graphPropName(graphProp_));
  // Range on the right of the title row: "lo-hi", or just the top if that doesn't fit.
  std::snprintf(line, sizeof line, "%s-%s", a, b);
  const int titleW = cpLen(graphPropName(graphProp_)) * kCharW;
  if (titleW + cpLen(line) * kCharW + 2 > Framebuffer::kWidth) std::snprintf(line, sizeof line, "%s", b);
  const int w = cpLen(line) * kCharW;
  if (titleW + w + 2 <= Framebuffer::kWidth) fb.drawTextPx(Framebuffer::kWidth - w + 1, 8, line);
  // Axes.
  fb.fillRect(1, kPlotTop - 1, 1, kPlotBottom - kPlotTop + 3, true);
  fb.fillRect(1, kPlotBottom + 1, Framebuffer::kWidth - 2, 1, true);
  for (int z = 1; z <= kElementCount; ++z) {
    float v;
    if (propertyValue(z, graphProp_, v)) fb.set(kPlotX0 + z - 1, plotY(v, lo, hi), true);
  }
  // Cursor: a dotted vertical line and a 3x3 box on the point.
  const int cx = kPlotX0 + cursor_ - 1;
  for (int y = kPlotTop; y <= kPlotBottom; y += 2) fb.set(cx, y, !fb.get(cx, y));
  float v;
  const bool has = propertyValue(cursor_, graphProp_, v);
  if (has) {
    const int py = plotY(v, lo, hi);
    fb.fillRect(cx - 1, py - 1, 3, 3, true);
  }
  // Readout: "26 Fe 1.83"
  const Element& e = element(cursor_);
  char val[24];
  if (has)
    std::snprintf(val, sizeof val, "%.6g", v);
  else
    std::snprintf(val, sizeof val, "\xE2\x80\x94");
  std::snprintf(line, sizeof line, "%d %s %s", e.z, e.symbol, e.name);
  const int vw = cpLen(val);
  if (cpLen(line) + 1 + vw > Framebuffer::kCols) std::snprintf(line, sizeof line, "%d %s", e.z, e.symbol);
  fb.drawTextPx(0, kFooterY, line);
  fb.drawTextPx((Framebuffer::kCols - vw) * kCharW, kFooterY, val);
}

void PeriodicApp::drawHelp(Framebuffer& fb) const {
  fb.drawText(0, 1, "PERIODIC TABLE  keys", true);
  fb.drawText(0, 2, "arrows move  = details");
  fb.drawText(0, 3, "1 list   2 sort list");
  fb.drawText(0, 4, "3 find by Z or symbol");
  fb.drawText(0, 5, "4 filter  5 graph");
  fb.drawText(0, 6, "AC back/exit  0 help");
}

}  // namespace calc
