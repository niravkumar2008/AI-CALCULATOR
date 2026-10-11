#include "battery.h"

#include <algorithm>

namespace calc {

namespace {
struct Point {
  int mv, pct;
};
// Typical 1-cell LiPo at ~0.1 C (Adafruit / common fuel-gauge tables).
const Point kCurve[] = {{3270, 0},  {3610, 5},  {3690, 10}, {3710, 15}, {3730, 20}, {3750, 25}, {3770, 30},
                        {3790, 35}, {3800, 40}, {3820, 45}, {3840, 50}, {3850, 55}, {3870, 60}, {3910, 65},
                        {3950, 70}, {3980, 75}, {4020, 80}, {4080, 85}, {4110, 90}, {4150, 95}, {4200, 100}};
constexpr int kPoints = sizeof(kCurve) / sizeof(kCurve[0]);
}  // namespace

int lipoPercent(int mv) {
  if (mv <= kCurve[0].mv) return 0;
  if (mv >= kCurve[kPoints - 1].mv) return 100;
  for (int i = 1; i < kPoints; ++i) {
    if (mv <= kCurve[i].mv) {
      const Point a = kCurve[i - 1], b = kCurve[i];
      return a.pct + (mv - a.mv) * (b.pct - a.pct) / (b.mv - a.mv);
    }
  }
  return 100;
}

Charge chargeState(bool vbus, bool statHigh) {
  if (!vbus) return Charge::NoCable;
  return statHigh ? Charge::Full : Charge::Charging;
}

int robustAverage(const int* s, int n) {
  if (n <= 0) return 0;
  if (n > 16) n = 16;
  int v[16];
  std::copy(s, s + n, v);
  std::sort(v, v + n);
  const int lo = n >= 4 ? 1 : 0, hi = n >= 4 ? n - 1 : n;
  long sum = 0;
  for (int i = lo; i < hi; ++i) sum += v[i];
  return static_cast<int>(sum / (hi - lo));
}

}  // namespace calc
