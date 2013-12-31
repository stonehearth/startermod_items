#ifndef _RADIANT_PERFMON_PERFMON_H
#define _RADIANT_PERFMON_PERFMON_H

#include "namespace.h"
#include "core/guard.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class TimelineCounterGuard {
public:
   TimelineCounterGuard(char const* name);
   TimelineCounterGuard(Timeline& timeline, char const* name);

   ~TimelineCounterGuard();
   
private:
   void Start(const char* name);

private:
   bool              disposed_;
   Counter*          last_counter_;
   Timeline*         timeline_;
};

void BeginFrame(bool enabled);
void SwitchToCounter(char const* name);
core::Guard OnFrameEnd(std::function<void(Frame*)>);
uint CounterToMilliseconds(CounterValueType value);
CounterValueType MillisecondsToCounter(uint value);

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_PERFMON_H
