#include "radiant.h"
#include "frame.h"

using namespace radiant;
using namespace radiant::perfmon;

Frame::Frame()
{
}

Frame::~Frame()
{
   for (Counter* c : counters_) {
      delete c;
   }
   counters_.clear();
}

Counter* Frame::GetCounter(char const* counter_name)
{
   for (Counter* c : counters_) {
      if (c->GetName() == counter_name) {
         return c;
      }
   }
   Counter* c = new Counter(counter_name);
   counters_.push_back(c);
   return c;
}

CounterValueType Frame::GetCounterSum() const
{
   CounterValueType sum = 0;
   for (Counter* c : counters_) {
      sum += c->GetValue();
   }
   return sum;
}

std::vector<Counter*> const& Frame::GetCounters() const
{
   return counters_;
}
