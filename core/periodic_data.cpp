// The element table for the Periodic Table app (periodic.h): all 118 elements,
// const, in flash. One line per element, in field order:
//   Z, symbol, name, mass, flags, group, period, block, category, state at 25 °C,
//   electron configuration, Pauling electronegativity, first ionization energy
//   (kJ/mol), empirical atomic radius (pm, 0 = none), density (g/cm3), melting
//   point (K), boiling point (K), year discovered (0 = ancient), oxidation-state
//   mask (bit s+4 = state s; the trailing comment spells them out).
// kNone = no value; the UI shows "—". Nothing is estimated here beyond what the
// sources below list.
//
// Sources (fetched 2026-10-11):
// - PubChem periodic table, https://pubchem.ncbi.nlm.nih.gov/rest/pug/periodictable/JSON
//   (names [US spelling], Pauling electronegativity, oxidation states, standard
//   state, melting/boiling points, density, year discovered, category).
// - Standard atomic weights: IUPAC conventional/abridged values as printed in the
//   CRC Handbook (e.g. Fe 55.845, Cu 63.546); for elements with no stable isotope
//   the mass number of the longest-lived isotope in brackets (Tc [98], Pm [145],
//   Po [209] ... Og [294]).
// - First ionization energies: CRC Handbook values as tabulated on Wikipedia,
//   "Molar ionization energies of the elements" (1-103; 104+ are only predicted,
//   so stored as none).
// - Empirical atomic radii: J. C. Slater, J. Chem. Phys. 41, 3199 (1964), via
//   Wikipedia "Atomic radii of the elements (data page)"; none where Slater gives none.
// - Electron configurations: NIST ground states (Madelung order + the measured
//   exceptions Cr, Cu, Nb, Mo, Ru, Rh, Pd, Ag, La, Ce, Gd, Pt, Au, Ac, Th, Pa, U,
//   Np, Cm; Lr = 7s2 7p1), written in n,l order. Z >= 104 are predictions (kPredicted).
// Layout and classification choices (documented, not from one source):
// - IUPAC 2021 table layout: Lu and Lr in group 3 (d-block); La-Yb and Ac-No are
//   the f-block rows (group 0 here). Lu/Lr keep the lanthanide/actinide category.
// - 10 categories: PubChem's, with Nonmetal + Halogen = reactive nonmetal, At as a
//   metalloid, 104-108 transition metals, 109-118 "unknown properties".
// - State at 25 °C is "unknown" for Z >= 104 (never observed in bulk).
// - He has no melting point at 1 atm (stored none). As sublimes at 887 K (the
//   stored 1090 K melting point is at 28 atm). Several heavy-element values are
//   the sources' estimates (e.g. Fr melting point, Fm-Lr melting points, At density).
#include "periodic.h"

namespace calc {

namespace {
constexpr float kNone = kElemNone;
constexpr uint8_t kMassNumber = Element::kMassNumber;
constexpr uint8_t kPredicted = Element::kPredicted;
using B = ElemBlock;
using C = ElemCategory;
using S = ElemState;
}  // namespace

const Element kElements[kElementCount] = {
  {  1, "H", "Hydrogen", 1.008f, 0, 1, 1, B::S, C::ReactiveNonmetal, S::Gas, "1s1", 2.2f, 1312.0f, 25, 8.988e-05f, 13.81f, 20.28f, 1766, 0x0028},  // +1,-1
  {  2, "He", "Helium", 4.0026f, 0, 18, 1, B::S, C::NobleGas, S::Gas, "1s2", kNone, 2372.3f, 0, 0.0001785f, kNone, 4.22f, 1868, 0x0010},  // 0
  {  3, "Li", "Lithium", 6.94f, 0, 1, 2, B::S, C::AlkaliMetal, S::Solid, "[He] 2s1", 0.98f, 520.2f, 145, 0.534f, 453.65f, 1615.0f, 1817, 0x0020},  // +1
  {  4, "Be", "Beryllium", 9.0122f, 0, 2, 2, B::S, C::AlkalineEarth, S::Solid, "[He] 2s2", 1.57f, 899.5f, 105, 1.85f, 1560.0f, 2744.0f, 1798, 0x0040},  // +2
  {  5, "B", "Boron", 10.81f, 0, 13, 2, B::P, C::Metalloid, S::Solid, "[He] 2s2 2p1", 2.04f, 800.6f, 85, 2.37f, 2348.0f, 4273.0f, 1808, 0x0080},  // +3
  {  6, "C", "Carbon", 12.011f, 0, 14, 2, B::P, C::ReactiveNonmetal, S::Solid, "[He] 2s2 2p2", 2.55f, 1086.5f, 70, 2.267f, 3823.0f, 4098.0f, 0, 0x0141},  // +4,+2,-4
  {  7, "N", "Nitrogen", 14.007f, 0, 15, 2, B::P, C::ReactiveNonmetal, S::Gas, "[He] 2s2 2p3", 3.04f, 1402.3f, 65, 0.0012506f, 63.15f, 77.36f, 1772, 0x03EE},  // +5,+4,+3,+2,+1,-1,-2,-3
  {  8, "O", "Oxygen", 15.999f, 0, 16, 2, B::P, C::ReactiveNonmetal, S::Gas, "[He] 2s2 2p4", 3.44f, 1313.9f, 60, 0.001429f, 54.36f, 90.2f, 1774, 0x0004},  // -2
  {  9, "F", "Fluorine", 18.998f, 0, 17, 2, B::P, C::ReactiveNonmetal, S::Gas, "[He] 2s2 2p5", 3.98f, 1681.0f, 50, 0.001696f, 53.53f, 85.03f, 1670, 0x0008},  // -1
  { 10, "Ne", "Neon", 20.180f, 0, 18, 2, B::P, C::NobleGas, S::Gas, "[He] 2s2 2p6", kNone, 2080.7f, 0, 0.0008999f, 24.56f, 27.07f, 1898, 0x0010},  // 0
  { 11, "Na", "Sodium", 22.990f, 0, 1, 3, B::S, C::AlkaliMetal, S::Solid, "[Ne] 3s1", 0.93f, 495.8f, 180, 0.97f, 370.95f, 1156.0f, 1807, 0x0020},  // +1
  { 12, "Mg", "Magnesium", 24.305f, 0, 2, 3, B::S, C::AlkalineEarth, S::Solid, "[Ne] 3s2", 1.31f, 737.7f, 150, 1.74f, 923.0f, 1363.0f, 1808, 0x0040},  // +2
  { 13, "Al", "Aluminum", 26.982f, 0, 13, 3, B::P, C::PostTransition, S::Solid, "[Ne] 3s2 3p1", 1.61f, 577.5f, 125, 2.7f, 933.437f, 2792.0f, 0, 0x0080},  // +3
  { 14, "Si", "Silicon", 28.085f, 0, 14, 3, B::P, C::Metalloid, S::Solid, "[Ne] 3s2 3p2", 1.9f, 786.5f, 110, 2.3296f, 1687.0f, 3538.0f, 1854, 0x0141},  // +4,+2,-4
  { 15, "P", "Phosphorus", 30.974f, 0, 15, 3, B::P, C::ReactiveNonmetal, S::Solid, "[Ne] 3s2 3p3", 2.19f, 1011.8f, 100, 1.82f, 317.3f, 553.65f, 1669, 0x0282},  // +5,+3,-3
  { 16, "S", "Sulfur", 32.06f, 0, 16, 3, B::P, C::ReactiveNonmetal, S::Solid, "[Ne] 3s2 3p4", 2.58f, 999.6f, 100, 2.067f, 388.36f, 717.75f, 0, 0x0504},  // +6,+4,-2
  { 17, "Cl", "Chlorine", 35.45f, 0, 17, 3, B::P, C::ReactiveNonmetal, S::Gas, "[Ne] 3s2 3p5", 3.16f, 1251.2f, 100, 0.003214f, 171.65f, 239.11f, 1774, 0x0A28},  // +7,+5,+1,-1
  { 18, "Ar", "Argon", 39.948f, 0, 18, 3, B::P, C::NobleGas, S::Gas, "[Ne] 3s2 3p6", kNone, 1520.6f, 0, 0.0017837f, 83.8f, 87.3f, 1894, 0x0010},  // 0
  { 19, "K", "Potassium", 39.098f, 0, 1, 4, B::S, C::AlkaliMetal, S::Solid, "[Ar] 4s1", 0.82f, 418.8f, 220, 0.89f, 336.53f, 1032.0f, 1807, 0x0020},  // +1
  { 20, "Ca", "Calcium", 40.078f, 0, 2, 4, B::S, C::AlkalineEarth, S::Solid, "[Ar] 4s2", 1.0f, 589.8f, 180, 1.54f, 1115.0f, 1757.0f, 0, 0x0040},  // +2
  { 21, "Sc", "Scandium", 44.956f, 0, 3, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d1 4s2", 1.36f, 633.1f, 160, 2.99f, 1814.0f, 3109.0f, 1879, 0x0080},  // +3
  { 22, "Ti", "Titanium", 47.867f, 0, 4, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d2 4s2", 1.54f, 658.8f, 140, 4.5f, 1941.0f, 3560.0f, 1791, 0x01C0},  // +4,+3,+2
  { 23, "V", "Vanadium", 50.942f, 0, 5, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d3 4s2", 1.63f, 650.9f, 135, 6.0f, 2183.0f, 3680.0f, 1801, 0x03C0},  // +5,+4,+3,+2
  { 24, "Cr", "Chromium", 51.996f, 0, 6, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d5 4s1", 1.66f, 652.9f, 140, 7.15f, 2180.0f, 2944.0f, 1797, 0x04C0},  // +6,+3,+2
  { 25, "Mn", "Manganese", 54.938f, 0, 7, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d5 4s2", 1.55f, 717.3f, 140, 7.3f, 1519.0f, 2334.0f, 1774, 0x09C0},  // +7,+4,+3,+2
  { 26, "Fe", "Iron", 55.845f, 0, 8, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d6 4s2", 1.83f, 762.5f, 140, 7.874f, 1811.0f, 3134.0f, 0, 0x00C0},  // +3,+2
  { 27, "Co", "Cobalt", 58.933f, 0, 9, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d7 4s2", 1.88f, 760.4f, 135, 8.86f, 1768.0f, 3200.0f, 1735, 0x00C0},  // +3,+2
  { 28, "Ni", "Nickel", 58.693f, 0, 10, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d8 4s2", 1.91f, 737.1f, 135, 8.912f, 1728.0f, 3186.0f, 1751, 0x00C0},  // +3,+2
  { 29, "Cu", "Copper", 63.546f, 0, 11, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d10 4s1", 1.9f, 745.5f, 135, 8.933f, 1357.77f, 2835.0f, 0, 0x0060},  // +2,+1
  { 30, "Zn", "Zinc", 65.38f, 0, 12, 4, B::D, C::TransitionMetal, S::Solid, "[Ar] 3d10 4s2", 1.65f, 906.4f, 135, 7.134f, 692.68f, 1180.0f, 1746, 0x0040},  // +2
  { 31, "Ga", "Gallium", 69.723f, 0, 13, 4, B::P, C::PostTransition, S::Solid, "[Ar] 3d10 4s2 4p1", 1.81f, 578.8f, 130, 5.91f, 302.91f, 2477.0f, 1875, 0x0080},  // +3
  { 32, "Ge", "Germanium", 72.630f, 0, 14, 4, B::P, C::Metalloid, S::Solid, "[Ar] 3d10 4s2 4p2", 2.01f, 762.0f, 125, 5.323f, 1211.4f, 3106.0f, 1886, 0x0140},  // +4,+2
  { 33, "As", "Arsenic", 74.922f, 0, 15, 4, B::P, C::Metalloid, S::Solid, "[Ar] 3d10 4s2 4p3", 2.18f, 947.0f, 115, 5.776f, 1090.0f, 887.0f, 0, 0x0282},  // +5,+3,-3
  { 34, "Se", "Selenium", 78.971f, 0, 16, 4, B::P, C::ReactiveNonmetal, S::Solid, "[Ar] 3d10 4s2 4p4", 2.55f, 941.0f, 115, 4.809f, 493.65f, 958.0f, 1817, 0x0504},  // +6,+4,-2
  { 35, "Br", "Bromine", 79.904f, 0, 17, 4, B::P, C::ReactiveNonmetal, S::Liquid, "[Ar] 3d10 4s2 4p5", 2.96f, 1139.9f, 115, 3.11f, 265.95f, 331.95f, 1826, 0x0228},  // +5,+1,-1
  { 36, "Kr", "Krypton", 83.798f, 0, 18, 4, B::P, C::NobleGas, S::Gas, "[Ar] 3d10 4s2 4p6", 3.0f, 1350.8f, 0, 0.003733f, 115.79f, 119.93f, 1898, 0x0010},  // 0
  { 37, "Rb", "Rubidium", 85.468f, 0, 1, 5, B::S, C::AlkaliMetal, S::Solid, "[Kr] 5s1", 0.82f, 403.0f, 235, 1.53f, 312.46f, 961.0f, 1861, 0x0020},  // +1
  { 38, "Sr", "Strontium", 87.62f, 0, 2, 5, B::S, C::AlkalineEarth, S::Solid, "[Kr] 5s2", 0.95f, 549.5f, 200, 2.64f, 1050.0f, 1655.0f, 1790, 0x0040},  // +2
  { 39, "Y", "Yttrium", 88.906f, 0, 3, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d1 5s2", 1.22f, 600.0f, 180, 4.47f, 1795.0f, 3618.0f, 1794, 0x0080},  // +3
  { 40, "Zr", "Zirconium", 91.224f, 0, 4, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d2 5s2", 1.33f, 640.1f, 155, 6.52f, 2128.0f, 4682.0f, 1789, 0x0100},  // +4
  { 41, "Nb", "Niobium", 92.906f, 0, 5, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d4 5s1", 1.6f, 652.1f, 145, 8.57f, 2750.0f, 5017.0f, 1801, 0x0280},  // +5,+3
  { 42, "Mo", "Molybdenum", 95.95f, 0, 6, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d5 5s1", 2.16f, 684.3f, 145, 10.2f, 2896.0f, 4912.0f, 1778, 0x0400},  // +6
  { 43, "Tc", "Technetium", 98.0f, kMassNumber, 7, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d5 5s2", 1.9f, 686.9f, 135, 11.0f, 2430.0f, 4538.0f, 1937, 0x0D00},  // +7,+6,+4
  { 44, "Ru", "Ruthenium", 101.07f, 0, 8, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d7 5s1", 2.2f, 710.2f, 130, 12.1f, 2607.0f, 4423.0f, 1827, 0x0080},  // +3
  { 45, "Rh", "Rhodium", 102.91f, 0, 9, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d8 5s1", 2.28f, 719.7f, 135, 12.4f, 2237.0f, 3968.0f, 1803, 0x0080},  // +3
  { 46, "Pd", "Palladium", 106.42f, 0, 10, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d10", 2.2f, 804.4f, 140, 12.0f, 1828.05f, 3236.0f, 1803, 0x00C0},  // +3,+2
  { 47, "Ag", "Silver", 107.87f, 0, 11, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d10 5s1", 1.93f, 731.0f, 160, 10.501f, 1234.93f, 2435.0f, 0, 0x0020},  // +1
  { 48, "Cd", "Cadmium", 112.41f, 0, 12, 5, B::D, C::TransitionMetal, S::Solid, "[Kr] 4d10 5s2", 1.69f, 867.8f, 155, 8.69f, 594.22f, 1040.0f, 1817, 0x0040},  // +2
  { 49, "In", "Indium", 114.82f, 0, 13, 5, B::P, C::PostTransition, S::Solid, "[Kr] 4d10 5s2 5p1", 1.78f, 558.3f, 155, 7.31f, 429.75f, 2345.0f, 1863, 0x0080},  // +3
  { 50, "Sn", "Tin", 118.71f, 0, 14, 5, B::P, C::PostTransition, S::Solid, "[Kr] 4d10 5s2 5p2", 1.96f, 708.6f, 145, 7.287f, 505.08f, 2875.0f, 0, 0x0140},  // +4,+2
  { 51, "Sb", "Antimony", 121.76f, 0, 15, 5, B::P, C::Metalloid, S::Solid, "[Kr] 4d10 5s2 5p3", 2.05f, 834.0f, 145, 6.685f, 903.78f, 1860.0f, 0, 0x0282},  // +5,+3,-3
  { 52, "Te", "Tellurium", 127.60f, 0, 16, 5, B::P, C::Metalloid, S::Solid, "[Kr] 4d10 5s2 5p4", 2.1f, 869.3f, 140, 6.232f, 722.66f, 1261.0f, 1782, 0x0504},  // +6,+4,-2
  { 53, "I", "Iodine", 126.90f, 0, 17, 5, B::P, C::ReactiveNonmetal, S::Solid, "[Kr] 4d10 5s2 5p5", 2.66f, 1008.4f, 140, 4.93f, 386.85f, 457.55f, 1811, 0x0A28},  // +7,+5,+1,-1
  { 54, "Xe", "Xenon", 131.29f, 0, 18, 5, B::P, C::NobleGas, S::Gas, "[Kr] 4d10 5s2 5p6", 2.6f, 1170.4f, 0, 0.005887f, 161.36f, 165.03f, 1898, 0x0010},  // 0
  { 55, "Cs", "Cesium", 132.91f, 0, 1, 6, B::S, C::AlkaliMetal, S::Solid, "[Xe] 6s1", 0.79f, 375.7f, 260, 1.93f, 301.59f, 944.0f, 1860, 0x0020},  // +1
  { 56, "Ba", "Barium", 137.33f, 0, 2, 6, B::S, C::AlkalineEarth, S::Solid, "[Xe] 6s2", 0.89f, 502.9f, 215, 3.62f, 1000.0f, 2170.0f, 1808, 0x0040},  // +2
  { 57, "La", "Lanthanum", 138.91f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 5d1 6s2", 1.1f, 538.1f, 195, 6.15f, 1191.0f, 3737.0f, 1839, 0x0080},  // +3
  { 58, "Ce", "Cerium", 140.12f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f1 5d1 6s2", 1.12f, 534.4f, 185, 6.77f, 1071.0f, 3697.0f, 1803, 0x0180},  // +4,+3
  { 59, "Pr", "Praseodymium", 140.91f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f3 6s2", 1.13f, 527.0f, 185, 6.77f, 1204.0f, 3793.0f, 1885, 0x0080},  // +3
  { 60, "Nd", "Neodymium", 144.24f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f4 6s2", 1.14f, 533.1f, 185, 7.01f, 1294.0f, 3347.0f, 1885, 0x0080},  // +3
  { 61, "Pm", "Promethium", 145.0f, kMassNumber, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f5 6s2", kNone, 540.0f, 185, 7.26f, 1315.0f, 3273.0f, 1945, 0x0080},  // +3
  { 62, "Sm", "Samarium", 150.36f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f6 6s2", 1.17f, 544.5f, 185, 7.52f, 1347.0f, 2067.0f, 1879, 0x00C0},  // +3,+2
  { 63, "Eu", "Europium", 151.96f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f7 6s2", kNone, 547.1f, 185, 5.24f, 1095.0f, 1802.0f, 1901, 0x00C0},  // +3,+2
  { 64, "Gd", "Gadolinium", 157.25f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f7 5d1 6s2", 1.2f, 593.4f, 180, 7.9f, 1586.0f, 3546.0f, 1880, 0x0080},  // +3
  { 65, "Tb", "Terbium", 158.93f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f9 6s2", kNone, 565.8f, 175, 8.23f, 1629.0f, 3503.0f, 1843, 0x0080},  // +3
  { 66, "Dy", "Dysprosium", 162.50f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f10 6s2", 1.22f, 573.0f, 175, 8.55f, 1685.0f, 2840.0f, 1886, 0x0080},  // +3
  { 67, "Ho", "Holmium", 164.93f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f11 6s2", 1.23f, 581.0f, 175, 8.8f, 1747.0f, 2973.0f, 1878, 0x0080},  // +3
  { 68, "Er", "Erbium", 167.26f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f12 6s2", 1.24f, 589.3f, 175, 9.07f, 1802.0f, 3141.0f, 1843, 0x0080},  // +3
  { 69, "Tm", "Thulium", 168.93f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f13 6s2", 1.25f, 596.7f, 175, 9.32f, 1818.0f, 2223.0f, 1879, 0x0080},  // +3
  { 70, "Yb", "Ytterbium", 173.05f, 0, 0, 6, B::F, C::Lanthanide, S::Solid, "[Xe] 4f14 6s2", kNone, 603.4f, 175, 6.9f, 1092.0f, 1469.0f, 1878, 0x00C0},  // +3,+2
  { 71, "Lu", "Lutetium", 174.97f, 0, 3, 6, B::D, C::Lanthanide, S::Solid, "[Xe] 4f14 5d1 6s2", 1.27f, 523.5f, 175, 9.84f, 1936.0f, 3675.0f, 1907, 0x0080},  // +3
  { 72, "Hf", "Hafnium", 178.49f, 0, 4, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d2 6s2", 1.3f, 658.5f, 155, 13.3f, 2506.0f, 4876.0f, 1923, 0x0100},  // +4
  { 73, "Ta", "Tantalum", 180.95f, 0, 5, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d3 6s2", 1.5f, 761.0f, 145, 16.4f, 3290.0f, 5731.0f, 1802, 0x0200},  // +5
  { 74, "W", "Tungsten", 183.84f, 0, 6, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d4 6s2", 2.36f, 770.0f, 135, 19.3f, 3695.0f, 5828.0f, 1783, 0x0400},  // +6
  { 75, "Re", "Rhenium", 186.21f, 0, 7, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d5 6s2", 1.9f, 760.0f, 135, 20.8f, 3459.0f, 5869.0f, 1925, 0x0D00},  // +7,+6,+4
  { 76, "Os", "Osmium", 190.23f, 0, 8, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d6 6s2", 2.2f, 840.0f, 130, 22.57f, 3306.0f, 5285.0f, 1803, 0x0180},  // +4,+3
  { 77, "Ir", "Iridium", 192.22f, 0, 9, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d7 6s2", 2.2f, 880.0f, 135, 22.42f, 2719.0f, 4701.0f, 1803, 0x0180},  // +4,+3
  { 78, "Pt", "Platinum", 195.08f, 0, 10, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d9 6s1", 2.28f, 870.0f, 135, 21.46f, 2041.55f, 4098.0f, 1735, 0x0140},  // +4,+2
  { 79, "Au", "Gold", 196.97f, 0, 11, 6, B::D, C::TransitionMetal, S::Solid, "[Xe] 4f14 5d10 6s1", 2.54f, 890.1f, 135, 19.282f, 1337.33f, 3129.0f, 0, 0x00A0},  // +3,+1
  { 80, "Hg", "Mercury", 200.59f, 0, 12, 6, B::D, C::TransitionMetal, S::Liquid, "[Xe] 4f14 5d10 6s2", 2.0f, 1007.1f, 150, 13.5336f, 234.32f, 629.88f, 0, 0x0060},  // +2,+1
  { 81, "Tl", "Thallium", 204.38f, 0, 13, 6, B::P, C::PostTransition, S::Solid, "[Xe] 4f14 5d10 6s2 6p1", 1.62f, 589.4f, 190, 11.8f, 577.0f, 1746.0f, 1861, 0x00A0},  // +3,+1
  { 82, "Pb", "Lead", 207.2f, 0, 14, 6, B::P, C::PostTransition, S::Solid, "[Xe] 4f14 5d10 6s2 6p2", 2.33f, 715.6f, 180, 11.342f, 600.61f, 2022.0f, 0, 0x0140},  // +4,+2
  { 83, "Bi", "Bismuth", 208.98f, 0, 15, 6, B::P, C::PostTransition, S::Solid, "[Xe] 4f14 5d10 6s2 6p3", 2.02f, 703.0f, 160, 9.807f, 544.55f, 1837.0f, 1753, 0x0280},  // +5,+3
  { 84, "Po", "Polonium", 209.0f, kMassNumber, 16, 6, B::P, C::Metalloid, S::Solid, "[Xe] 4f14 5d10 6s2 6p4", 2.0f, 812.1f, 190, 9.32f, 527.0f, 1235.0f, 1898, 0x0140},  // +4,+2
  { 85, "At", "Astatine", 210.0f, kMassNumber, 17, 6, B::P, C::Metalloid, S::Solid, "[Xe] 4f14 5d10 6s2 6p5", 2.2f, 899.003f, 0, 7.0f, 575.0f, kNone, 1940, 0x0AA8},  // +7,+5,+3,+1,-1
  { 86, "Rn", "Radon", 222.0f, kMassNumber, 18, 6, B::P, C::NobleGas, S::Gas, "[Xe] 4f14 5d10 6s2 6p6", kNone, 1037.0f, 0, 0.00973f, 202.0f, 211.45f, 1900, 0x0010},  // 0
  { 87, "Fr", "Francium", 223.0f, kMassNumber, 1, 7, B::S, C::AlkaliMetal, S::Solid, "[Rn] 7s1", 0.7f, 393.0f, 0, kNone, 300.0f, kNone, 1939, 0x0020},  // +1
  { 88, "Ra", "Radium", 226.0f, kMassNumber, 2, 7, B::S, C::AlkalineEarth, S::Solid, "[Rn] 7s2", 0.9f, 509.3f, 215, 5.0f, 973.0f, 1413.0f, 1898, 0x0040},  // +2
  { 89, "Ac", "Actinium", 227.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 6d1 7s2", 1.1f, 499.0f, 195, 10.07f, 1324.0f, 3471.0f, 1899, 0x0080},  // +3
  { 90, "Th", "Thorium", 232.04f, 0, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 6d2 7s2", 1.3f, 587.0f, 180, 11.72f, 2023.0f, 5061.0f, 1828, 0x0100},  // +4
  { 91, "Pa", "Protactinium", 231.04f, 0, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f2 6d1 7s2", 1.5f, 568.0f, 180, 15.37f, 1845.0f, kNone, 1913, 0x0300},  // +5,+4
  { 92, "U", "Uranium", 238.03f, 0, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f3 6d1 7s2", 1.38f, 597.6f, 175, 18.95f, 1408.0f, 4404.0f, 1789, 0x0780},  // +6,+5,+4,+3
  { 93, "Np", "Neptunium", 237.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f4 6d1 7s2", 1.36f, 604.5f, 175, 20.25f, 917.0f, 4175.0f, 1940, 0x0780},  // +6,+5,+4,+3
  { 94, "Pu", "Plutonium", 244.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f6 7s2", 1.28f, 584.7f, 175, 19.84f, 913.0f, 3501.0f, 1940, 0x0780},  // +6,+5,+4,+3
  { 95, "Am", "Americium", 243.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f7 7s2", 1.3f, 578.0f, 175, 13.69f, 1449.0f, 2284.0f, 1944, 0x0780},  // +6,+5,+4,+3
  { 96, "Cm", "Curium", 247.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f7 6d1 7s2", 1.3f, 581.0f, 0, 13.51f, 1618.0f, 3400.0f, 1944, 0x0080},  // +3
  { 97, "Bk", "Berkelium", 247.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f9 7s2", 1.3f, 601.0f, 0, 14.0f, 1323.0f, kNone, 1949, 0x0180},  // +4,+3
  { 98, "Cf", "Californium", 251.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f10 7s2", 1.3f, 608.0f, 0, kNone, 1173.0f, kNone, 1950, 0x0080},  // +3
  { 99, "Es", "Einsteinium", 252.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f11 7s2", 1.3f, 619.0f, 0, kNone, 1133.0f, kNone, 1952, 0x0080},  // +3
  {100, "Fm", "Fermium", 257.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f12 7s2", 1.3f, 629.0f, 0, kNone, 1800.0f, kNone, 1952, 0x0080},  // +3
  {101, "Md", "Mendelevium", 258.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f13 7s2", 1.3f, 636.0f, 0, kNone, 1100.0f, kNone, 1955, 0x00C0},  // +3,+2
  {102, "No", "Nobelium", 259.0f, kMassNumber, 0, 7, B::F, C::Actinide, S::Solid, "[Rn] 5f14 7s2", 1.3f, 639.0f, 0, kNone, 1100.0f, kNone, 1957, 0x00C0},  // +3,+2
  {103, "Lr", "Lawrencium", 266.0f, kMassNumber, 3, 7, B::D, C::Actinide, S::Solid, "[Rn] 5f14 7s2 7p1", 1.3f, 479.0f, 0, kNone, 1900.0f, kNone, 1961, 0x0080},  // +3
  {104, "Rf", "Rutherfordium", 267.0f, kMassNumber|kPredicted, 4, 7, B::D, C::TransitionMetal, S::Unknown, "[Rn] 5f14 6d2 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1964, 0x0100},  // +4
  {105, "Db", "Dubnium", 268.0f, kMassNumber|kPredicted, 5, 7, B::D, C::TransitionMetal, S::Unknown, "[Rn] 5f14 6d3 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1967, 0x0380},  // +5,+4,+3
  {106, "Sg", "Seaborgium", 269.0f, kMassNumber|kPredicted, 6, 7, B::D, C::TransitionMetal, S::Unknown, "[Rn] 5f14 6d4 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1974, 0x0790},  // +6,+5,+4,+3,0
  {107, "Bh", "Bohrium", 270.0f, kMassNumber|kPredicted, 7, 7, B::D, C::TransitionMetal, S::Unknown, "[Rn] 5f14 6d5 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1976, 0x0B80},  // +7,+5,+4,+3
  {108, "Hs", "Hassium", 269.0f, kMassNumber|kPredicted, 8, 7, B::D, C::TransitionMetal, S::Unknown, "[Rn] 5f14 6d6 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1984, 0x17C0},  // +8,+6,+5,+4,+3,+2
  {109, "Mt", "Meitnerium", 278.0f, kMassNumber|kPredicted, 9, 7, B::D, C::Unknown, S::Unknown, "[Rn] 5f14 6d7 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1982, 0x35A0},  // +9,+8,+6,+4,+3,+1
  {110, "Ds", "Darmstadtium", 281.0f, kMassNumber|kPredicted, 10, 7, B::D, C::Unknown, S::Unknown, "[Rn] 5f14 6d8 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1994, 0x1550},  // +8,+6,+4,+2,0
  {111, "Rg", "Roentgenium", 282.0f, kMassNumber|kPredicted, 11, 7, B::D, C::Unknown, S::Unknown, "[Rn] 5f14 6d9 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1994, 0x02A8},  // +5,+3,+1,-1
  {112, "Cn", "Copernicium", 285.0f, kMassNumber|kPredicted, 12, 7, B::D, C::Unknown, S::Unknown, "[Rn] 5f14 6d10 7s2", kNone, kNone, 0, kNone, kNone, kNone, 1996, 0x0070},  // +2,+1,0
  {113, "Nh", "Nihonium", 286.0f, kMassNumber|kPredicted, 13, 7, B::P, C::Unknown, S::Unknown, "[Rn] 5f14 6d10 7s2 7p1", kNone, kNone, 0, kNone, kNone, kNone, 2004, 0x0000},  // 
  {114, "Fl", "Flerovium", 289.0f, kMassNumber|kPredicted, 14, 7, B::P, C::Unknown, S::Unknown, "[Rn] 5f14 6d10 7s2 7p2", kNone, kNone, 0, kNone, kNone, kNone, 1998, 0x0570},  // +6,+4,+2,+1,0
  {115, "Mc", "Moscovium", 290.0f, kMassNumber|kPredicted, 15, 7, B::P, C::Unknown, S::Unknown, "[Rn] 5f14 6d10 7s2 7p3", kNone, kNone, 0, kNone, kNone, kNone, 2003, 0x00A0},  // +3,+1
  {116, "Lv", "Livermorium", 293.0f, kMassNumber|kPredicted, 16, 7, B::P, C::Unknown, S::Unknown, "[Rn] 5f14 6d10 7s2 7p4", kNone, kNone, 0, kNone, kNone, kNone, 2000, 0x0144},  // +4,+2,-2
  {117, "Ts", "Tennessine", 294.0f, kMassNumber|kPredicted, 17, 7, B::P, C::Unknown, S::Unknown, "[Rn] 5f14 6d10 7s2 7p5", kNone, kNone, 0, kNone, kNone, kNone, 2010, 0x02A8},  // +5,+3,+1,-1
  {118, "Og", "Oganesson", 294.0f, kMassNumber|kPredicted, 18, 7, B::P, C::Unknown, S::Unknown, "[Rn] 5f14 6d10 7s2 7p6", kNone, kNone, 0, kNone, kNone, kNone, 2006, 0x0578},  // +6,+4,+2,+1,0,-1
};

const size_t kElementTableBytes = sizeof(kElements);

}  // namespace calc
