#include "radiant.h"
#include "timer.h"
#include "perfmon.h"

using namespace radiant;
using namespace radiant::perfmon;

Timer::Timer() :
   start_time_(0)
{
}

Timer::~Timer()
{
}

void Timer::Start()
{
   start_time_ = GetCurrentTime();
}

CounterValueType Timer::Stop()
{
   if (!start_time_) {
      throw std::logic_error("call to Timer::Stop while timer's not running");
   }
   CounterValueType elapsed = GetCurrentTime() - start_time_;
   start_time_ = 0;
   return elapsed;
}

CounterValueType Timer::Restart()
{
   if (!start_time_) {
      throw std::logic_error("call to Timer::Restart while timer's not running");
   }
   CounterValueType now = GetCurrentTime();
   CounterValueType elapsed = now - start_time_;
   start_time_ = now;
   return elapsed;
}

CounterValueType Timer::GetElapsed()
{
   if (!start_time_) {
      throw std::logic_error("call to Timer::Restart while timer's not running");
   }
   return GetCurrentTime() - start_time_;
}

CounterValueType Timer::GetCurrentTime()
{
   LARGE_INTEGER value;
   QueryPerformanceCounter(&value);
   return value.QuadPart;
}

uint Timer::GetCurrentTimeMs()
{
   return perfmon::CounterToMilliseconds(GetCurrentTime());
}
