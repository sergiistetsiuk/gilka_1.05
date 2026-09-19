#pragma once
#include <stdint.h>

namespace SectionLogic
{
inline int wrap(int value, int count) { return (value % count + count) % count; }
inline bool fresh(uint32_t now, uint32_t received, bool valid, uint32_t maxAge)
{ return valid && uint32_t(now - received) <= maxAge; }

constexpr uint32_t ALERT_POLL_MS = 60000;
constexpr uint32_t ALERT_MAX_AGE_MS = 180000;
// SOS: dot=200ms, dash=600ms, symbol gap=200ms, letter gap=600ms,
// repeat gap=1400ms. All outputs use the same phase, starting on the first dot.
constexpr uint32_t SOS_UNIT_MS = 200;
constexpr uint32_t SOS_CYCLE_MS = 34 * SOS_UNIT_MS;
inline bool sosOn(uint32_t now, uint32_t started)
{
  const uint8_t units[] = {1,1,1,1,1,3,3,1,3,1,3,3,1,1,1,1,1,7};
  uint32_t phase = uint32_t(now - started) % SOS_CYCLE_MS;
  for (unsigned i = 0; i < sizeof(units); ++i) {
    uint32_t duration = uint32_t(units[i]) * SOS_UNIT_MS;
    if (phase < duration) return (i % 2) == 0;
    phase -= duration;
  }
  return false;
}

// Zero selects stopwatch; countdown values advance in 30-second steps.
constexpr uint32_t TIMER_STEP_SECONDS = 30;
constexpr uint32_t TIMER_MAX_SECONDS = 99 * 60 + 30;
inline uint32_t adjustTimerSeconds(uint32_t seconds, int direction)
{
  const int64_t next = int64_t(seconds) + int64_t(direction) * TIMER_STEP_SECONDS;
  return next < 0 ? 0 : next > TIMER_MAX_SECONDS ? TIMER_MAX_SECONDS : uint32_t(next);
}

// Alarm indication takes priority over timer heartbeat and simulated detector flashes.
inline bool indicatorOn(uint32_t now, bool alarm, bool running, uint32_t elapsed, bool demoPulse)
{
  if (alarm) return (now / 350) % 2 == 0;
  if (running) return elapsed % 1000 < 150;
  return demoPulse;
}

struct Timer
{
  uint32_t durationMs = 60000;
  uint32_t elapsedMs = 0;
  uint32_t lastTick = 0;
  uint32_t minute = 0;
  bool running = false;
  bool alarm = false;
  bool stopwatch = false;
  bool minuteEvent = false;
  uint32_t nextAlarmBeep = 0;

  void reset(uint32_t duration, bool countUp = false)
  {
    durationMs = duration; elapsedMs = 0; minute = 0;
    running = alarm = minuteEvent = false; stopwatch = countUp;
  }
  void tick(uint32_t now)
  {
    minuteEvent = false;
    if (!running) return;
    const uint32_t delta = now - lastTick;
    lastTick = now;
    const uint64_t elapsed = uint64_t(elapsedMs) + delta;
    elapsedMs = elapsed > UINT32_MAX ? UINT32_MAX : uint32_t(elapsed);
    if (!stopwatch && elapsedMs >= durationMs)
    { elapsedMs = durationMs; running = false; alarm = true; nextAlarmBeep = now; return; }
    const uint32_t currentMinute = elapsedMs / 60000;
    if (currentMinute > minute) { minute = currentMinute; minuteEvent = true; }
  }
  bool takeAlarmBeep(uint32_t now)
  {
    if (!alarm || int32_t(now - nextAlarmBeep) < 0) return false;
    nextAlarmBeep = now + 1000;
    return true;
  }
  void click(uint32_t now)
  {
    if (alarm) { reset(durationMs, stopwatch); return; }
    tick(now);
    if (alarm) return;
    running = !running;
    lastTick = now;
  }
  uint32_t shownSeconds() const
  { return stopwatch ? elapsedMs / 1000 : (durationMs - elapsedMs + 999) / 1000; }
};
}
