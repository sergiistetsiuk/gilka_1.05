#include "SectionLogic.h"
#include <assert.h>

int main()
{
  using namespace SectionLogic;
  assert(wrap(-1, 6) == 5 && wrap(6, 6) == 0);
  assert(adjustTimerSeconds(60, 1) == 90);
  assert(adjustTimerSeconds(60, -1) == 30);
  assert(adjustTimerSeconds(30, -1) == 0);
  assert(adjustTimerSeconds(0, -1) == 0);
  assert(adjustTimerSeconds(0, 1) == 30);
  assert(adjustTimerSeconds(TIMER_MAX_SECONDS, 1) == TIMER_MAX_SECONDS);
  for (uint32_t seconds = 0; seconds < TIMER_MAX_SECONDS; seconds += 30)
    assert(adjustTimerSeconds(seconds, 1) == seconds + 30);
  assert(!indicatorOn(0, false, false, 0, false));
  assert(indicatorOn(0, false, true, 0, false));
  assert(indicatorOn(149, false, true, 149, false));
  assert(!indicatorOn(150, false, true, 150, false));
  assert(!indicatorOn(999, false, true, 999, false));
  assert(indicatorOn(1000, false, true, 1000, false));
  assert(!indicatorOn(1000, false, false, 1000, false)); // Pause turns heartbeat off.
  assert(indicatorOn(0, true, false, 0, false));
  assert(!indicatorOn(350, true, true, 0, true)); // Alarm OFF phase overrides other flashes.
  assert(indicatorOn(700, true, false, 0, false));
  assert(indicatorOn(0, false, false, 0, true)); // Simulation works independently of sound.
  // Check complete SOS pulse widths and pauses independently at every millisecond.
  const uint32_t starts[] = {0,400,800,1600,2400,3200,4400,4800,5200};
  const uint32_t lengths[] = {200,200,200,600,600,600,200,200,200};
  for (uint32_t elapsed=0; elapsed<SOS_CYCLE_MS*2; ++elapsed) {
    bool expected=false;
    for (unsigned pulse=0; pulse<9; ++pulse)
      if (elapsed%SOS_CYCLE_MS>=starts[pulse] && elapsed%SOS_CYCLE_MS<starts[pulse]+lengths[pulse]) expected=true;
    assert(sosOn(1234+elapsed,1234)==expected);
    assert(sosOn(uint32_t(UINT32_MAX-1000+elapsed),UINT32_MAX-1000)==expected);
  }
  assert(ALERT_POLL_MS==60000);
  assert(fresh(180000,0,true,ALERT_MAX_AGE_MS));
  assert(!fresh(180001,0,true,ALERT_MAX_AGE_MS));
  Timer t;
  t.reset(120000);
  t.click(100);
  t.tick(60100);
  assert(t.minuteEvent && t.shownSeconds() == 60);
  t.tick(60101);
  assert(!t.minuteEvent);
  t.click(70100);
  t.tick(500000);
  assert(!t.running && t.shownSeconds() == 50);
  t.click(500000);
  t.tick(550000);
  assert(t.alarm && !t.running && t.shownSeconds() == 0);
  assert(t.takeAlarmBeep(550000));
  assert(!t.takeAlarmBeep(550000));
  assert(!t.takeAlarmBeep(550999));
  assert(t.takeAlarmBeep(551000));
  t.click(551001);
  assert(!t.takeAlarmBeep(552000));
  assert(!t.alarm && t.shownSeconds() == 120);
  t.reset(60000);
  t.click(UINT32_MAX - 500);
  t.tick(499);
  assert(t.elapsedMs == 1000);
  t.reset(30000);
  t.click(UINT32_MAX - 30500);
  t.tick(UINT32_MAX - 500);
  assert(t.alarm && t.takeAlarmBeep(UINT32_MAX - 500));
  assert(!t.takeAlarmBeep(498));
  assert(t.takeAlarmBeep(499));
  t.reset(0, true);
  t.click(0); t.tick(60000);
  assert(t.minuteEvent && t.running && !t.alarm && t.shownSeconds() == 60);
  assert(fresh(100, UINT32_MAX - 99, true, 200));
  assert(!fresh(101, UINT32_MAX - 99, true, 200));
  assert(!fresh(0, 0, false, 1000));
}
