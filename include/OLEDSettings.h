#pragma once

#include <stdint.h>

namespace OLEDSettings
{
constexpr uint32_t MAX_TIMEOUT_SECONDS = 86400;

// Reject missing, malformed and overflowing input before changing any settings.
inline bool parseNumber(const char *text, uint32_t minimum,
                        uint32_t maximum, uint32_t &result)
{
  if (!text || !*text)
    return false;

  uint32_t value = 0;
  for (; *text; ++text)
  {
    if (*text < '0' || *text > '9')
      return false;
    const uint32_t digit = *text - '0';
    if (digit > maximum || value > (maximum - digit) / 10)
      return false;
    value = value * 10 + digit;
  }
  if (value < minimum)
    return false;
  result = value;
  return true;
}

inline void normalize(uint8_t brightness, uint8_t &dimBrightness,
                      uint32_t &dimTimeout, uint32_t &offTimeout)
{
  if (dimBrightness > brightness)
    dimBrightness = brightness;
  if (dimTimeout > MAX_TIMEOUT_SECONDS)
    dimTimeout = MAX_TIMEOUT_SECONDS;
  if (offTimeout > MAX_TIMEOUT_SECONDS)
    offTimeout = MAX_TIMEOUT_SECONDS;
  if (offTimeout > 0 && dimTimeout > 0 && offTimeout < dimTimeout)
    offTimeout = dimTimeout;
}
}
