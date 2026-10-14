// CMPLX mode (MODE 6), like the Casio fx-115ES / fx-991ES PLUS complex-number mode (linear display).
//
// CMPLX is the COMP screen (View::Calc) with complex values: the Device evaluates its expression
// with evaluateComplex() instead of evaluate(). COMP never comes here, so real-mode results can't
// change. evaluateComplex() is a small parser with the real one's structure (same priorities,
// implicit multiplication, auto-closed brackets, the shared kMaxNesting budget, an EvalRun) that
// works on the same tokens; every step whose values are all real goes back to the real engine
// (calc::realOp), so 1÷3 is still 1⌟3 and sin(30) still 1⌟2 in CMPLX.
//
// What it supports, as the Casio does:
//   i (the ENG key in CMPLX), + − × ÷, implicit ×, (−), x², x³, x⁻¹, ^ with an integer exponent
//   (|n| ≤ 1E9), Abs( (|z|), and SHIFT 2's menu: arg( (in the angle unit, (−180°, 180°]),
//   Conjg(, ▶r∠θ / ▶a+bi (only at the very end: shows that form whatever SETUP says).
//   Polar entry r∠θ (∠ = SHIFT (−)): r and θ real, θ in the angle unit; ∠ has the nPr / nCr
//   priority (looser than implicit ×, tighter than × ÷), so 2∠30×2 = (2∠30)×2.
//   Variables A-F X Y M, Ans and PreAns hold complex values (their imaginary parts: ImagVars).
// What is a Math ERROR with a non-real value (the fx-ES can't either): every other function
// (sin cos log ln √ ³√ eˣ 10ˣ Int Rnd ...), x!, %, ⌟, ⁿ√, nPr / nCr, a non-integer or complex
// exponent, a complex r or θ in r∠θ, ÷ 0. √(−1) is a Math ERROR too (√ is a real function, as on
// the Casio: type i). ∫( d/dx( "=" are Syntax ERROR (the keys don't type them in CMPLX).
//
// Display (complexText): "3+4i", "−i", "1⌟2−1⌟2i" (fractions as in COMP, S⇔D for both parts),
// or "5∠53.13010235" (θ in the angle unit). A real value shows as in COMP unless ▶r∠θ asked.
// Too long for one row (25 characters): complexParts() gives the real part (or r) and the
// imaginary part (or ∠θ); the Device shows one with "R⇔I" and SHIFT = swaps them, as the Casio.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "calc_engine.h"

namespace calc {

struct Cx {
  Num re, im;  // im is an exact 0 for a real value
  bool real() const { return im.v == 0; }
  static Cx of(const Num& re) {
    Cx z;
    z.re = re;
    z.im = Num::ofInt(0);
    return z;
  }
};

// SETUP ▼ 2:CMPLX: how a non-real result shows.
enum class CxForm : uint8_t { Rect, Polar };  // a+bi, r∠θ

// The imaginary parts of the memories (calc::Vars holds the real parts). Only the Device's CMPLX
// mode sets them; leaving CMPLX clears them (the Casio keeps the real part only).
struct ImagVars {
  Num a, b, c, d, e, f, x, y, m, ans, preAns;
  Num& get(Tok t);  // like Vars::get (anything else: ans)
  const Num& get(Tok t) const { return const_cast<ImagVars*>(this)->get(t); }
  bool any() const;  // any non-zero imaginary part
};

struct CxResult {
  CalcError error = CalcError::None;
  int errorPos = 0;  // token index for Goto, as EvalResult
  Cx value;
  int forced = -1;  // a trailing ▶a+bi (0) or ▶r∠θ (1), else -1
};

// Evaluates a CMPLX expression (tokens as typed). vars: the real parts (read only, like
// evaluate()); imag: the imaginary parts. random: Ran# / RanInt#.
CxResult evaluateComplex(const std::vector<Tok>& toks, Vars& vars, const ImagVars& imag, AngleUnit unit,
                         double (*random)());

// |z| and arg z (arg in the angle unit, (−180°, 180°]; arg 0 = 0). False: out of range.
bool cxAbs(const Cx& z, Num& out);
bool cxArg(const Cx& z, AngleUnit unit, Num& out);

// The whole value as one row: "3+4i" / "5∠53.13010235" (form Polar always gives r∠θ; "0" for 0).
std::string complexText(const Cx& z, CxForm form, bool preferFraction, NormMode norm, AngleUnit unit);
// The two halves for the R⇔I display: "3" and "4i" ("−4i"), or "5" and "∠53.13010235".
void complexParts(const Cx& z, CxForm form, bool preferFraction, NormMode norm, AngleUnit unit, std::string& first,
                  std::string& second);

}  // namespace calc
