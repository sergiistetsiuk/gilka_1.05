#include "SectionLogic.h"
#include <assert.h>

int main()
{
  using namespace SectionLogic;
  assert(wrap(-1, 6) == 5 && wrap(6, 6) == 0);
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
  t.click(550001);
  assert(!t.alarm && t.shownSeconds() == 120);
  t.reset(60000);
  t.click(UINT32_MAX - 500);
  t.tick(499);
  assert(t.elapsedMs == 1000);
  t.reset(0, true);
  t.click(0); t.tick(60000);
  assert(t.minuteEvent && t.running && !t.alarm && t.shownSeconds() == 60);
  assert(fresh(100, UINT32_MAX - 99, true, 200));
  assert(!fresh(101, UINT32_MAX - 99, true, 200));
  assert(!fresh(0, 0, false, 1000));
}
