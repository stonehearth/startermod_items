#ifndef _RADIANT_PERFMON_PERFMON_H
#define _RADIANT_PERFMON_PERFMON_H

#include "namespace.h"
#include "core/guard.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class TimelineCounterGuard {
public:
   TimelineCounterGuard(char const* name);
   void Dispose();
   ~TimelineCounterGuard();
   
private:
   bool              disposed_;
   Counter*          last_counter_;
};

void BeginFrame();
void SwitchToCounter(char const* name);
core::Guard OnFrameEnd(std::function<void(Frame*)>);
uint CounterToMilliseconds(CounterValueType value);
CounterValueType MillisecondsToCounter(uint value);

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_PERFMON_H
