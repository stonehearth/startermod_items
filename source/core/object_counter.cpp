#include "radiant.h"
#include "platform/utils.h"
#include "object_counter.h"
#include <mutex>
#include <tbb/spin_mutex.h>

using namespace ::radiant;
using namespace ::radiant::core;

#if defined(ENABLE_OBJECT_COUNTER)

tbb::spin_mutex ObjectCounterBase::__lock;
ObjectCounterBase::ObjectMap ObjectCounterBase::__objects;

typedef std::map<int, std::type_index, std::greater<int>> SortedCounters;


/*
 * -- GetObjects
 *
 * Return the lifetime map of all objects since TrackObjectLifetime(true) was
 * called, keyed by the object pointer (which is USELESS, since it's most likely
 * an ObjectCounter<T> and not the base class... unless you favor std::dynamic_cast...).
 * The value is a pair() containing when the object was allocated (in ms) and the
 * typeinfo for that object.
 */

ObjectCounterBase::ObjectMap const& ObjectCounterBase::GetObjects()
{
   return __objects;
}

void ObjectCounterBase::TrackObjectLifetime(bool enable)
{
   __objects.clear();
}

/*
 * -- ObjectCounterBase::IncrementObjectCount
 *
 * Add one to the count of the object type.
 */

void ObjectCounterBase::IncrementObjectCount(ObjectCounterBase *that, std::type_info const& t)
{
   tbb::spin_mutex::scoped_lock lock(__lock);
   std::type_index ti(t);
   auto i = __objects.find(ti);
   if (i == __objects.end()) {
      i = __objects.emplace(std::make_pair(ti, AllocationMap(t))).first;
   }
   AllocationMap &am = i->second;
   am.allocs[that] = perfmon::Timer::GetCurrentCounterValueType();
}


/*
 * -- ObjectCounterBase::IncrementObjectCount
 *
 * Subtract one to the count of the object type.
 */

void ObjectCounterBase::DecrementObjectCount(ObjectCounterBase *that, std::type_info const& t)
{
   tbb::spin_mutex::scoped_lock lock(__lock);
   auto i = __objects.find(t);
   if (i != __objects.end()) {
      AllocationMap& m = i->second;
      m.allocs.erase(that);
      if (m.allocs.empty()) {
         __objects.erase(t);
      }
   }
}

#endif

