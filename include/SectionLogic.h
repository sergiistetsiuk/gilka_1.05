#pragma once
#include <stdint.h>

namespace SectionLogic
{
inline int wrap(int value, int count) { return (value % count + count) % count; }
inline bool fresh(uint32_t now, uint32_t received, bool valid, uint32_t maxAge)
{ return valid && uint32_t(now - received) <= maxAge; }

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
    { elapsedMs = durationMs; running = false; alarm = true; return; }
    const uint32_t currentMinute = elapsedMs / 60000;
    if (currentMinute > minute) { minute = currentMinute; minuteEvent = true; }
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
