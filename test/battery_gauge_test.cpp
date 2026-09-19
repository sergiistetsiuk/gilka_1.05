#include "BatteryGauge.h"
#include <assert.h>
int main()
{
  using namespace BatteryGauge;
  assert(!valid(0) && !valid(2499) && valid(2500) && valid(4200) && !valid(4351));
  assert(percent(3000) == 0 && percent(3700) == 50 && percent(4200) == 100);
  assert(segments(3300) == 0 && segments(3400) == 1 && segments(3700) == 2 && segments(3950) == 3);
  assert(segments(3710, 1) == 1 && segments(3730, 1) == 2);
  assert(segments(3680, 2) == 2 && segments(3669, 2) == 1);
  assert(segments(4200, 0) == 3 && segments(3200, 3) == 0);
}
