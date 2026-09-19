#pragma once
#include <Arduino.h>
#include "BatteryGauge.h"

namespace BatteryMonitor
{
constexpr uint8_t PIN = 9;
constexpr uint32_t CAPACITY_MAH = 6000;
// 100 kohm from battery + to ADC, 100 kohm from ADC to GND: Vbattery = 2 * Vadc.
static uint32_t millivolts = 0, sampledAt = 0;
static bool ready = false, valid = false;
static int bars = -1;

inline void sample()
{
  uint32_t sum = 0;
  analogReadMilliVolts(PIN); // Discard first conversion after switching ADC input.
  for (int i = 0; i < 16; ++i) sum += analogReadMilliVolts(PIN);
  const uint32_t measured = (sum * 2 + 8) / 16;
  const bool good = BatteryGauge::valid(measured);
  millivolts = good && valid ? (millivolts * 3 + measured + 2) / 4 : measured;
  valid = good;
  bars = valid ? BatteryGauge::segments(millivolts, bars) : -1;
  sampledAt = millis();
}
inline void begin()
{
  pinMode(PIN, INPUT);
  analogSetPinAttenuation(PIN, ADC_11db);
  ready = true;
  sample();
}
inline void tick()
{ if (ready && uint32_t(millis() - sampledAt) >= 1000) sample(); }
}
