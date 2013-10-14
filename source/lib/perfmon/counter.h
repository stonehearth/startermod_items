#ifndef _RADIANT_PERFMON_COUNTER_H
#define _RADIANT_PERFMON_COUNTER_H

#include "namespace.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class Counter
{
public:
   Counter(char const* name);
   ~Counter();

   std::string GetName() const;
   CounterValueType GetValue() const;
   void Increment(CounterValueType amount);

private:
   std::string       name_;
   CounterValueType  value_;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_COUNTER_H
