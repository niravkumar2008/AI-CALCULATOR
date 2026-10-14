# numeric staging: what changes

New files (copy as-is):
- core/numeric.h, core/numeric.cpp: CALC / SOLVE runners, integrate / derivative / solve, token helpers.
- tests/numeric_tests.cpp: `int runNumericTests()` (192 checks; returns failures). Not wired into
  tests/tests.cpp or build_and_run.bat (the integrator does that).

Modified (complete copies; the live v15 files were identical to the base, so these are drop-in):
- core/calc_engine.h: three tokens added at the end of `enum class Tok`, just before `Count`:
  `Eq` ("="), `Integ` ("∫(", opens a bracket), `Deriv` ("d/dx(", opens a bracket). Existing token
  values are unchanged (saved states and the Device's `Tok::Count` sentinel keep working).
- core/calc_engine.cpp:
  1. `#include "numeric.h"`.
  2. Three rows appended to the `kTok` table for the new tokens.
  3. `startsOperand()` also accepts `Integ` / `Deriv` (implicit ×: `2∫(X,0,1)`).
  4. `Parser::primary()` sends `Integ` / `Deriv` to a new `Parser::calculus()` (~40 lines): keeps f
     as tokens up to the first top-level comma (no ∫ / d/dx inside f: Syntax ERROR), evaluates
     a [, b] [, tol], then calls `numeric::evalTemplate()`. Syntax errors inside f point into f.
  `Tok::Eq` is not handled by the evaluator, so a plain = on "X=1" stays a Syntax ERROR.
  parseExpression() (AI text) is unchanged: it does not read "∫(", "d/dx(" or "=".

Checks: full existing suite (tests.cpp + geo/periodic/engine_regress/fuzz, all core/*.cpp) with
these files overlaid on the live v15 worktree: 1219 passed, 0 failed (it was 1211 without; the 8
extra are tests.cpp's glyph checks covering the new tokens). numeric tests: 192 checks, 0 failed.
