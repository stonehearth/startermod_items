#ifndef _RADIANT_PERFMON_PERFMON_H
#define _RADIANT_PERFMON_PERFMON_H

#include "namespace.h"
#include "core/guard.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class PerfmonThreadGuard {
public:
   PerfmonThreadGuard();
   ~PerfmonThreadGuard();
};

class FrameGuard {
public:
   FrameGuard();
   ~FrameGuard();
};

class TimelineCounterGuard {
public:
   TimelineCounterGuard(char const* name);
   ~TimelineCounterGuard();
   
private:
   Counter*          last_counter_;
};

void SwitchToCounter(char const* name);
core::Guard OnFrameEnd(std::function<void(Frame*)>);
uint CounterToMilliseconds(CounterValueType value);

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_PERFMON_H
