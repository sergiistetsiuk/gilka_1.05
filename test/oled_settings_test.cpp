#include "OLEDSettings.h"
#include <assert.h>
#include <stdint.h>

int main()
{
  uint32_t value = 42;
  const char *invalid[] = {nullptr, "", "-1", "+1", " 1", "1 ", "1s", "1.5",
                           "86401", "4294967296", "999999999999999999999"};
  for (const char *text : invalid)
  {
    assert(!OLEDSettings::parseNumber(text, 0, 86400, value));
    assert(value == 42);
  }
  assert(OLEDSettings::parseNumber("0", 0, 86400, value) && value == 0);
  assert(OLEDSettings::parseNumber("86400", 0, 86400, value) && value == 86400);
  assert(!OLEDSettings::parseNumber("0", 1, 255, value));
  assert(!OLEDSettings::parseNumber("256", 1, 255, value));
  assert(OLEDSettings::parseNumber("1", 1, 255, value) && value == 1);
  assert(OLEDSettings::parseNumber("255", 1, 255, value) && value == 255);

  uint8_t dim = 20;
  uint32_t dimTime = UINT32_MAX, offTime = 300;
  OLEDSettings::normalize(5, dim, dimTime, offTime);
  assert(dim == 5 && dimTime == 86400 && offTime == 86400);

  dimTime = 30;
  offTime = 0;
  OLEDSettings::normalize(120, dim, dimTime, offTime);
  assert(dimTime == 30 && offTime == 0);

  dimTime = 0;
  offTime = UINT32_MAX;
  OLEDSettings::normalize(120, dim, dimTime, offTime);
  assert(dimTime == 0 && offTime == 86400);

  dim = 20;
  dimTime = 30;
  offTime = 300;
  OLEDSettings::normalize(120, dim, dimTime, offTime);
  assert(dim == 20 && dimTime == 30 && offTime == 300);
}
