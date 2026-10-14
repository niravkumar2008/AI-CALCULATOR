// Periodic Table app (a TI-84 Plus CE "Periodic"-style app) for the Apps menu.
//
// Self-contained: a const table of all 118 elements in flash (periodic_data.cpp),
// a small key-driven state machine and a text/pixel render for the 125x61 mono
// screen. No heap allocation in the state machine; render() only builds the short
// std::strings Framebuffer::drawText needs. The v15 colour UI draws its own
// screens from the same public state (screen(), cursor(), gridPos(), filter(),
// highlighted(), detailLine(), graph helpers...), so nothing here is mono-only.
//
// Screens and keys (DKey):
//   Table   arrows move over the 18x7 grid + the two f-block rows (gaps skipped),
//           = details, 1 list, 2 list sorted the next way, 3 find, 4 next filter
//           (SHIFT 4 previous), 5 graph, 0 help, AC leaves the app (wantsExit()).
//   Detail  every property, up/down scroll, left/right previous/next element
//           (wraps 118 <-> 1), AC (or =) back to where it was opened from.
//   List    1-118 one per line, up/down move, left/right page, 2 cycles the sort
//           (number / name / symbol), = details, 1 or AC back to the table.
//   Search  digits type an atomic number (3 digits max, 1-118), = jumps to it;
//           or arrows pick letters on an A-Z grid (+ OK cell), = adds the letter,
//           two letters that make a symbol jump at once, OK jumps to the best
//           match; DEL undoes, AC goes back.
//   Graph   a property against Z for 1-118 (missing values skipped, axes auto-
//           scaled), left/right move the cursor (SHIFT: 10 at a time), up/down
//           change the property, = details, AC back to the table.
//   Help    the key legend; any key goes back.
#pragma once
#include <cstddef>
#include <cstdint>

#include "keys.h"  // calc::DKey
#include "framebuffer.h"

namespace calc {

// ----------------------------------------------------------------------------- data

constexpr int kElementCount = 118;
constexpr float kElemNone = -1.0f;  // "no value" for every float property (all real values are > 0)

enum class ElemBlock : uint8_t { S, P, D, F };
enum class ElemCategory : uint8_t {
  AlkaliMetal, AlkalineEarth, TransitionMetal, PostTransition, Metalloid,
  ReactiveNonmetal, NobleGas, Lanthanide, Actinide, Unknown, Count
};
enum class ElemState : uint8_t { Solid, Liquid, Gas, Unknown };  // at 25 °C, 1 atm

struct Element {
  static constexpr uint8_t kMassNumber = 1;  // no stable isotope: mass = mass number of the longest-lived one, shown [98]
  static constexpr uint8_t kPredicted = 2;   // Z >= 104: configuration and oxidation states are predictions

  uint8_t z;
  char symbol[3];
  const char* name;
  float mass;               // standard atomic weight (conventional, u) or a mass number (kMassNumber)
  uint8_t flags;
  uint8_t group;            // 1-18; 0 = in the f-block rows (La-Yb, Ac-No)
  uint8_t period;           // 1-7
  ElemBlock block;
  ElemCategory category;
  ElemState state;
  const char* config;       // noble-gas shorthand, ASCII: "[Ar] 3d10 4s1"
  float electronegativity;  // Pauling, or kElemNone
  float ionization;         // first ionization energy, kJ/mol, or kElemNone
  uint16_t radius;          // empirical atomic radius, pm; 0 = none
  float density;            // g/cm3 for every element (gases too; the UI shows gases in g/L), or kElemNone
  float melt;               // melting point, K, or kElemNone
  float boil;               // boiling point, K, or kElemNone
  int16_t year;             // year discovered; 0 = known since ancient times
  uint16_t oxMask;          // common oxidation states: bit (s + 4) set for state s, s = -4..+11

  bool massIsNumber() const { return (flags & kMassNumber) != 0; }
  bool predicted() const { return (flags & kPredicted) != 0; }
};

extern const Element kElements[kElementCount];
extern const size_t kElementTableBytes;  // sizeof(kElements), for reporting

inline bool hasValue(float v) { return v >= 0.0f; }

// Element z (1..118); out-of-range z is clamped.
const Element& element(int z);

const char* categoryName(ElemCategory c);  // "Transition metal"
const char* blockName(ElemBlock b);        // "s", "p", "d", "f"
const char* stateName(ElemState s);        // "solid", "liquid", "gas", "unknown"

// Grid placement (IUPAC 2021 layout: Lu and Lr in group 3, the f-block rows hold
// La-Yb and Ac-No). Rows 0-6 = periods 1-7, row 7 = La-Yb, row 8 = Ac-No;
// columns 0-17 = groups 1-18 (the f-block rows use columns 2-15).
constexpr int kGridRows = 9;
constexpr int kGridCols = 18;
struct GridPos {
  int8_t row;
  int8_t col;
};
GridPos gridPos(int z);
int elementAt(int row, int col);  // 0 = empty cell

// Exact symbol, case-insensitive ("fe", "FE" -> 26); 0 if none.
int findSymbol(const char* sym);
// Best match for a typed prefix: the exact symbol if there is one, else the
// lowest Z whose symbol starts with it; 0 if none.
int matchSymbolPrefix(const char* prefix);

// Text helpers (UTF-8 into a caller buffer, always NUL-terminated; "—" = none).
void formatMass(int z, char* out, size_t n);        // "55.845", "[98]"
void formatConfig(int z, char* out, size_t n, bool superscripts);  // "[Ar] 3d⁶ 4s²"
void formatOxidation(int z, char* out, size_t n);   // "+3, +2"
void formatTemperature(float kelvin, char* out, size_t n);  // "1811 K (1538°C)"
void formatDensity(int z, char* out, size_t n);     // "7.874 g/cm³", gases "0.08988 g/L"

// The detail screen's text, already wrapped to `width` characters: returns the
// number of lines and copies line `index` (if in range) into out.
int detailLine(int z, int index, char* out, size_t n, int width = 25);

// ----------------------------------------------------------------------------- filters

enum class Filter : uint8_t {
  All,
  AlkaliMetal, AlkalineEarth, TransitionMetal, PostTransition, Metalloid,  // the categories, in
  ReactiveNonmetal, NobleGas, Lanthanide, Actinide, UnknownCategory,       // ElemCategory order
  BlockS, BlockP, BlockD, BlockF,
  Solid, Liquid, Gas, UnknownState,
  Count
};
const char* filterName(Filter f);  // "Alkali metals", "s-block", "Gas at 25°C"...
bool matchesFilter(int z, Filter f);

// ----------------------------------------------------------------------------- graph

enum class GraphProp : uint8_t { Electronegativity, Ionization, Radius, Density, Melting, Boiling, Mass, Count };
const char* graphPropName(GraphProp p);   // short title, e.g. "Ionization kJ/mol"
bool propertyValue(int z, GraphProp p, float& v);  // false if the element has no value
// Auto-scale range over the elements that have a value; hi > lo always.
void graphRange(GraphProp p, float& lo, float& hi);

// ----------------------------------------------------------------------------- the app

class PeriodicApp {
 public:
  enum class Screen : uint8_t { Table, Detail, List, Search, Graph, Help };
  enum class Sort : uint8_t { Number, Name, Symbol, Count };

  static constexpr int kSearchCells = 27;   // A-Z + OK
  static constexpr int kSearchCols = 9;     // cells per grid row
  static constexpr int kListRows = 5;       // visible list lines on the mono screen
  static constexpr int kDetailRows = 5;     // visible detail lines on the mono screen

  PeriodicApp() { reset(); }

  // Back to the table on hydrogen, no filter, sorted by number.
  void reset();
  // Returns true if the key changed anything (redraw).
  bool onKey(DKey k, bool shift, bool alpha);
  // Draws rows 1-6 (y 8-60); row 0 is the host's status bar. Doesn't clear.
  void render(Framebuffer& fb) const;
  // AC on the table: the host should close the app (and call reset() next time).
  bool wantsExit() const { return exit_; }
  // Clears wantsExit() so the host can open the app again where it was left.
  void resume() { exit_ = false; }

  Screen screen() const { return screen_; }
  int cursor() const { return cursor_; }           // selected element (all screens), 1..118
  Filter filter() const { return filter_; }
  bool highlighted(int z) const { return filter_ != Filter::All && matchesFilter(z, filter_); }
  Sort sort() const { return sort_; }
  int listPos() const { return listPos_; }         // index of cursor() in the sorted list
  int listTop() const { return listTop_; }         // first visible list index
  int listAt(int i) const { return order_[i < 0 ? 0 : i >= kElementCount ? kElementCount - 1 : i]; }
  int detailScroll() const { return scroll_; }
  GraphProp graphProp() const { return graphProp_; }
  const char* searchLetters() const { return letters_; }  // "", "F", "Fe"
  int searchNumber() const { return number_; }            // 0 = none typed
  int searchCell() const { return searchCell_; }          // 0-25 = A-Z, 26 = OK
  int searchMatch() const;                                // element the search would open, 0 = none

 private:
  void moveTable(int dRow, int dCol);
  void openDetail(Screen from);
  void setSort(Sort s);
  void syncList();
  void jumpTo(int z);
  void searchKey(DKey k);

  void drawTable(Framebuffer& fb) const;
  void drawDetail(Framebuffer& fb) const;
  void drawList(Framebuffer& fb) const;
  void drawSearch(Framebuffer& fb) const;
  void drawGraph(Framebuffer& fb) const;
  void drawHelp(Framebuffer& fb) const;

  Screen screen_ = Screen::Table;
  Screen back_ = Screen::Table;  // where Detail / Search / Help return to
  int cursor_ = 1;
  Filter filter_ = Filter::All;
  Sort sort_ = Sort::Number;
  int listPos_ = 0;
  int listTop_ = 0;
  int scroll_ = 0;
  GraphProp graphProp_ = GraphProp::Electronegativity;
  char letters_[3] = {};
  int number_ = 0;
  int searchCell_ = 0;
  bool exit_ = false;
  uint8_t order_[kElementCount] = {};
};

}  // namespace calc
