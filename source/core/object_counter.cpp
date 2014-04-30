#include "radiant.h"
#include "object_counter.h"
#include <mutex>

using namespace ::radiant;
using namespace ::radiant::core;

#if defined(ENABLE_OBJECT_COUNTER)

boost::detail::spinlock ObjectCounterBase::__lock;
std::unordered_map<std::type_index, int> ObjectCounterBase::__counters;

typedef std::map<int, std::type_index, std::greater<int>> SortedCounters;


/*
 * -- GetObjectCounts
 *
 * Return a map of all the object counts allocated right now, indexed by
 * the typeinfo for those objects.
 */

ObjectCounterBase::CounterMap ObjectCounterBase::GetObjectCounts()
{
   return __counters;
}


/*
 * -- ReportCounts
 *
 * Call the `cb` for each result in the SortedCounters list, stopping when cb returns
 * false.
 */

static void ReportCounts(SortedCounters const& sorted, ObjectCounterBase::ForEachObjectCountCb cb)
{
   for (auto const& entry : sorted) {
      if (!cb(entry.second, entry.first)) {
         break;
      }
   }
}


/*
 * -- ForEachObjectDeltaCount
 *
 * Call `cb` passing the type_info and how the count of all objects in the system
 * has changed since the last `checkpoint` (see GetObjectCounts).
 *
 * Objects will be passed in descending order according to the total allocation
 * count.  Iteration will be aborted if `cb` returns false.
 */

void ObjectCounterBase::ForEachObjectDeltaCount(CounterMap const& checkpoint, ForEachObjectCountCb cb)
{
   SortedCounters sorted;

   {
      std::lock_guard<boost::detail::spinlock> lock(__lock);

      auto checkpoint_end = checkpoint.end();
      for (const auto& entry : __counters) {
         int count = entry.second;
         auto i = checkpoint.find(entry.first);
         if (i != checkpoint_end) {
            count -= i->second;
         }
         sorted.insert(std::make_pair(count, entry.first));
      }
   }
   ReportCounts(sorted, cb);
}

/*
 * -- ObjectCounterBase::ForEachObjectCount
 *
 * Call `cb` passing the type_info and count of all objects in the system.
 * Objects will be passed in descending order according to the total allocation
 * count.  Iteration will be aborted if `cb` returns false.
 */

void ObjectCounterBase::ForEachObjectCount(ForEachObjectCountCb cb)
{
   SortedCounters sorted;

   {
      std::lock_guard<boost::detail::spinlock> lock(__lock);
      for (const auto& entry : __counters) {
         sorted.insert(std::make_pair(entry.second, entry.first));
      }
   }
   ReportCounts(sorted, cb);
}


/*
 * -- ObjectCounterBase::IncrementObjectCount
 *
 * Add one to the count of the object type.
 */

void ObjectCounterBase::IncrementObjectCount(std::type_info const& t)
{
   std::lock_guard<boost::detail::spinlock> lock(__lock);

   auto i = __counters.find(std::type_index(t));
   if (i != __counters.end()) {
      i->second++;
   } else {
      __counters[std::type_index(t)] = 1;
   }
}


/*
 * -- ObjectCounterBase::IncrementObjectCount
 *
 * Subtract one to the count of the object type.
 */

void ObjectCounterBase::DecrementObjectCount(std::type_info const& t)
{
   std::lock_guard<boost::detail::spinlock> lock(__lock);
   __counters[std::type_index(t)]--;
}


/*
 * -- ObjectCounterBase::GetObjectCount
 *
 * Get the count of the object with the specified base.
 */

int ObjectCounterBase::GetObjectCount(std::type_info const& t)
{
   std::lock_guard<boost::detail::spinlock> lock(__lock);
   auto i = __counters.find(std::type_index(t));
   return i != __counters.end() ? i->second : 0;
}

#endif

