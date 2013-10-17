#ifndef _RADIANT_PERFMON_TIMER_H
#define _RADIANT_PERFMON_TIMER_H

#include "namespace.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class Timer
{
public:
   Timer();
   ~Timer();

   void Start();
   CounterValueType GetElapsed();
   CounterValueType Stop();
   CounterValueType Restart();

   static CounterValueType GetCurrentTime();

private:
   CounterValueType    start_time_;
   CounterValueType    elapsed_;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_TIMER_H
