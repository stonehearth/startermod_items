#ifndef _RADIANT_PERFMON_TIMER_H
#define _RADIANT_PERFMON_TIMER_H

#include "namespace.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

void Timer_Init();

class Timer
{
public:
   Timer();
   ~Timer();

   void Start();
   CounterValueType GetElapsed() const;
   CounterValueType Stop();
   CounterValueType Restart();
   void Reset();

   static CounterValueType GetCurrentCounterValueType();
   static uint64 GetCurrentTimeMs();

private:
   bool            started_;
   CounterValueType    start_time_;
   CounterValueType    elapsed_;
};

uint CounterToMilliseconds(CounterValueType value);
CounterValueType MillisecondsToCounter(uint value);


END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_TIMER_H
