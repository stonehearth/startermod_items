#ifndef _RADIANT_PERFMON_STORE_H
#define _RADIANT_PERFMON_STORE_H

#include <unordered_map>
#include "namespace.h"
#include "counter.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class Store {
   public:
      typedef std::unordered_map<std::string, Counter> CounterMap;

   public:
      Store();

      Timeline* GetTimeline();
      Counter& GetCounter(const char* name);
      CounterMap const& GetCounters() const;
      void SetCounter(const char* name, CounterValueType value);

   private:
      std::unique_ptr<Timeline>  timeline_;
      CounterMap                 counters_;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_STORE_H
