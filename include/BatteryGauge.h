#pragma once
#include <stdint.h>

namespace BatteryGauge
{
// Approximate voltage indication for a 1S Li-ion battery, not a fuel gauge.
inline bool valid(uint32_t mv) { return mv >= 2500 && mv <= 4350; }
inline int percent(uint32_t mv)
{ return mv <= 3200 ? 0 : mv >= 4200 ? 100 : int((mv - 3200) / 10); }
inline int segments(uint32_t mv, int previous = -1)
{
  const uint32_t thresholds[] = {3400, 3700, 3950};
  int level = 0;
  for (int i = 0; i < 3; ++i)
  {
    const uint32_t threshold = previous < 0 ? thresholds[i] :
      previous > i ? thresholds[i] - 30 : thresholds[i] + 30;
    if (mv >= threshold) ++level;
  }
  return level;
}
}
