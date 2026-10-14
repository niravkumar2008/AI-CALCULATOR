// BASE-N mode (MODE 7), like the Casio fx-115ES / fx-991ES PLUS base-n mode.
//
// BASE-N is the COMP screen (View::Calc) with integers: the Device evaluates its expression
// with evaluateBaseN() instead of evaluate(), so COMP's results can't change. A small parser
// with the real one's habits (implicit multiplication, auto-closed brackets, the shared
// kMaxNesting budget, an EvalRun) on the same tokens.
//
// As the fx-ES user's guide says:
//   - the number mode (base): DEC (x²), HEX (x^□), BIN (log), OCT (ln); DEC when BASE-N opens.
//     A shown result is converted when the base changes.
//   - values are 32-bit two's complement integers, −2147483648..2147483647; BIN's input and
//     output range is 16 bits (−32768..32767: 0000000000000000..0111111111111111 and
//     1000000000000000..1111111111111111). OCT 00000000000..37777777777, HEX 00000000..FFFFFFFF
//     (80000000 and up are the negatives).
//   - hex digits A-F on the (−) °'" hyp sin cos tan keys; no decimal point, no ×10ˣ: a
//     non-integer value (a memory set in COMP) is cut to an integer (toward 0), and so is ÷.
//   - SHIFT 3 (BASE): 1:and 2:or 3:xnor 4:xor 5:Not( 6:Neg(; ▼ 1:d 2:h 3:b 4:o, a prefix that
//     says the base of the number right after it ("d10+h10+b10+o10" = 36).
//   - priorities (fx-ES "Calculation Priority Sequence"): brackets and functions, the sign and
//     the prefixes, implicit ×, × ÷, + −, and, then or xor xnor (left to right).
// Choices where the guide says nothing (PORT_NOTES 2026-10-12):
//   - a digit the base doesn't have (2 in BIN, A in DEC without h) is a Syntax ERROR; a number,
//     or any step's result, outside the 32-bit range is a Math ERROR; the final result must
//     also fit the current base (BIN: 16 bits), else Math ERROR. A literal's value is its
//     bits: BIN takes up to 16 bits, HEX / OCT up to 32 (FFFFFFFF = −1), DEC up to 2147483647.
//   - − (or +) where a value belongs is its sign (the (−) key types A in BASE-N); x ÷ 0 and
//     Neg(−2147483648) are Math ERRORs.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "calc_engine.h"

namespace calc {

enum class Base : uint8_t { Dec, Hex, Bin, Oct };

struct BaseResult {
  CalcError error = CalcError::None;
  int errorPos = 0;  // token index for Goto, as EvalResult
  int32_t value = 0;
};

// Evaluates a BASE-N expression (tokens as typed) in number mode `base`. vars: the memories
// and Ans (a non-integer one is cut to an integer; one outside 32 bits is a Math ERROR).
BaseResult evaluateBaseN(const std::vector<Tok>& toks, const Vars& vars, Base base);

bool baseFits(int64_t v, Base base);       // in the base's output range (BIN: 16 bits)
std::string baseText(int32_t v, Base base);  // "−5", "0000022B", "0000001000101011", "00000001053"
const char* baseName(Base base);             // the status bar's "Dec" "Hex" "Bin" "Oct"

}  // namespace calc
