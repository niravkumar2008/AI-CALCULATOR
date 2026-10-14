// The calculator's keys (DKey), on their own so the app modules (geo.h,
// periodic.h) can take key presses without including device.h, which owns
// those modules. device.h includes this; dkeyName() is in device.cpp.
#pragma once
#include <cstdint>

namespace calc {

// Every key on the keypad, top-left to bottom-right, with the fx-115ES legends
// (the product's shell; stage 14). Row 2 is CALC, ∫dx, x⁻¹, logₐb. On the
// fx-115ES, Abs is SHIFT hyp, x³ is SHIFT x² and ∛ is SHIFT √. CALC / SHIFT SOLVE
// and ∫dx / SHIFT d/dx work as on the fx-115ES (core/numeric.h); ALPHA CALC types "=".
// nPr / nCr are SHIFT × / SHIFT ÷ (Pol / Rec on SHIFT + / −).
enum class DKey : uint8_t {
  Shift, Alpha, Up, Down, Left, Right, Mode, On,
  Calc, Integral, Inv, LogAB,
  Frac, Sqrt, Sq, Pow, Log, Ln,
  Neg, Dms, Hyp, Sin, Cos, Tan,
  Rcl, Eng, Open, Close, SD, MPlus,
  D7, D8, D9, Del, AC,
  D4, D5, D6, Mul, Div,
  D1, D2, D3, Add, Sub,
  D0, Dot, Exp10, Ans, Eq,
  Count
};
const char* dkeyName(DKey k);  // "SHIFT", "sin", "7" ... for logs and tests

}  // namespace calc
