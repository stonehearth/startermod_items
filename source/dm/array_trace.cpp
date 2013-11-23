#include "radiant.h"
#include "array.h"
#include "array_trace.h"
#include "store.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename A>
ArrayTrace<A>::ArrayTrace(const char* reason, Object const& o, Store const& store) :
   TraceImpl(reason, o, store)
{
}

template <typename A>
std::shared_ptr<ArrayTrace<A>> ArrayTrace<A>::OnChanged(ChangedCb changed)
{
   changed_ = changed;
   return shared_from_this();
}


template <typename A>
std::shared_ptr<ArrayTrace<A>> ArrayTrace<A>::OnUpdated(UpdatedCb updated)
{
   updated_ = updated;
   return shared_from_this();
}

template <typename A>
void ArrayTrace<A>::SignalChanged(uint i, Value const& value) 
{
   SignalModified();
   if (changed) {
      changed_(i, value)
   } else if (updated_) {
      NOT_YET_IMPLEMENTED();
   }
}


template <typename A>
void ArrayTrace<A>::SignalUpdated(ChangeMap const& updated)
{
   SignalModified();
   if (updated_) {
      updated_(updates);
   } else {
      for (const auto& entry : updated) {
         SignalChanged(entry.first, entry.second);
      }
   }
}
