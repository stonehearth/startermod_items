#ifndef _RADIANT_PERFMON_FRAME_H
#define _RADIANT_PERFMON_FRAME_H

#include "namespace.h"
#include "timer.h"
#include "counter.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class Frame {
   public:
      Frame();
      ~Frame();

      Counter* GetCounter(char const* name);
      CounterValueType GetCounterSum() const;
      std::vector<Counter*> const& GetCounters() const;

   private:
      std::vector<Counter*>   counters_;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_FRAME_H
