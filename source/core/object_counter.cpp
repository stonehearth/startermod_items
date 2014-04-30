#include "radiant.h"
#include "object_counter.h"
#include <tuple>

using namespace ::radiant;
using namespace ::radiant::core;

#if defined(ENABLE_OBJECT_COUNTER)

std::mutex ObjectCounterBase::__mutex;
std::unordered_map<std::type_index, size_t> ObjectCounterBase::__counters;

/*
 * -- ObjectCounterBase::ForEachObjectCount
 *
 * Call `cb` passing the type_info and count of all objects in the system.
 * Objects will be passed in descending order according to the total allocation
 * count.  Iteration will be aborted if `cb` returns false.
 */

void ObjectCounterBase::ForEachObjectCount(ForEachObjectCountCb cb)
{
   std::map<size_t, std::type_index, std::greater<size_t>> sorted;

   {
      std::lock_guard<std::mutex> lock(__mutex);
      for (const auto& entry : __counters) {
         sorted.insert(std::make_pair(entry.second, entry.first));
      }
   }
   for (auto const& entry : sorted) {
      if (!cb(entry.second, entry.first)) {
         break;
      }
   }
}


/*
 * -- ObjectCounterBase::IncrementObjectCount
 *
 * Add one to the count of the object type.
 */

void ObjectCounterBase::IncrementObjectCount(std::type_info const& t)
{
   std::lock_guard<std::mutex> lock(__mutex);

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
   std::lock_guard<std::mutex> lock(__mutex);
   __counters[std::type_index(t)]--;
}


/*
 * -- ObjectCounterBase::GetObjectCount
 *
 * Get the count of the object with the specified base.
 */

size_t ObjectCounterBase::GetObjectCount(std::type_info const& t)
{
   std::lock_guard<std::mutex> lock(__mutex);
   auto i = __counters.find(std::type_index(t));
   return i != __counters.end() ? i->second : 0;
}

#endif

