#include "radiant.h"
#include "store.h"
#include "timeline.h"

using namespace radiant;
using namespace radiant::perfmon;

Store::Store() :
   timeline_(new Timeline)
{
}

Timeline* Store::GetTimeline()
{
   return timeline_.get();
}

Counter& Store::GetCounter(const char* name)
{
   auto i = counters_.find(name);
   if (i == counters_.end()) {
      counters_.insert(std::make_pair(name, Counter(name)));
      i = counters_.find(name);
   }
   return i->second;
}

Store::CounterMap const& Store::GetCounters() const
{
   return counters_;
}
