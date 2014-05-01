#include "radiant.h"
#include "platform/utils.h"
#include "object_counter.h"
#include <mutex>

using namespace ::radiant;
using namespace ::radiant::core;

#if defined(ENABLE_OBJECT_COUNTER)

bool ObjectCounterBase::__track_objects;
boost::detail::spinlock ObjectCounterBase::__lock;
ObjectCounterBase::CounterMap ObjectCounterBase::__counters;
ObjectCounterBase::ObjectMap ObjectCounterBase::__objects;

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
 * -- GetObjects
 *
 * Return the lifetime map of all objects since TrackObjectLifetime(true) was
 * called, keyed by the object pointer (which is USELESS, since it's most likely
 * an ObjectCounter<T> and not the base class... unless you favor std::dynamic_cast...).
 * The value is a pair() containing when the object was allocated (in ms) and the
 * typeinfo for that object.
 */

ObjectCounterBase::ObjectMap ObjectCounterBase::GetObjects()
{
   return __objects;
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


void ObjectCounterBase::TrackObjectLifetime(bool enable)
{
   __track_objects = enable;
   __objects.clear();
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

void ObjectCounterBase::IncrementObjectCount(ObjectCounterBase *that, std::type_info const& t)
{
   std::lock_guard<boost::detail::spinlock> lock(__lock);

   auto i = __counters.find(std::type_index(t));
   if (i != __counters.end()) {
      i->second++;
   } else {
      __counters[std::type_index(t)] = 1;
   }
   if (__track_objects) {
      // std::type_index() has no default constructor, which is why this line of code is so... soooo
      // ugly.  
      __objects.insert(std::make_pair(that, std::make_pair(platform::get_current_time_in_ms(), std::type_index(t))));
   }
}


/*
 * -- ObjectCounterBase::IncrementObjectCount
 *
 * Subtract one to the count of the object type.
 */

void ObjectCounterBase::DecrementObjectCount(ObjectCounterBase *that, std::type_info const& t)
{
   std::lock_guard<boost::detail::spinlock> lock(__lock);
   __counters[std::type_index(t)]--;
   if (__track_objects) {
      __objects.erase(that);
   }
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

