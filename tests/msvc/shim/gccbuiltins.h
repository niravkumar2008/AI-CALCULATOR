#pragma once
#ifdef _MSC_VER
#include <intrin.h>
#include <climits>
inline bool __builtin_mul_overflow(long long a, long long b, long long* r) {
  long long hi;
  long long lo = _mul128(a, b, &hi);
  *r = lo;
  return hi != (lo >> 63);
}
inline bool __builtin_add_overflow(long long a, long long b, long long* r) {
  if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b)) { *r = 0; return true; }
  *r = a + b;
  return false;
}
#endif
