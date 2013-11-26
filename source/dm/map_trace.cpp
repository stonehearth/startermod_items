#include "radiant.h"
#include "map.h"
#include "map_trace.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

template <typename M>
MapTrace<M>::MapTrace(const char* reason, Object const& o, Store const& store) :
   TraceImpl(reason, o, store)
{
}

template <typename M>
std::shared_ptr<MapTrace<M>> MapTrace<M>::OnChanged(ChangedCb changed)
{
   changed_ = changed;
   return shared_from_this();
}

template <typename M>
std::shared_ptr<MapTrace<M>> MapTrace<M>::OnRemoved(RemovedCb removed)
{
   removed_ = removed;
   return shared_from_this();
}

template <typename M>
std::shared_ptr<MapTrace<M>> MapTrace<M>::OnUpdated(UpdatedCb updated)
{
   updated_ = updated;
   return shared_from_this();
}

template <typename M>
std::shared_ptr<MapTrace<M>> MapTrace<M>::PushObjectState()
{
   GetStore().PushMapState(*this, GetObjectId());
   return shared_from_this();
}

template <typename M>
void MapTrace<M>::SignalRemoved(Key const& key)
{
   SignalModified();
   if (removed_) {
      removed_(key);
   } else if (updated_) {
      NOT_YET_IMPLEMENTED();
   }
}

template <typename M>
void MapTrace<M>::SignalChanged(Key const& key, Value const& value) 
{
   SignalModified();
   if (changed_) {
      changed_(key, value);
   } else if (updated_) {
      NOT_YET_IMPLEMENTED();
   }
}

template <typename M>
void MapTrace<M>::SignalUpdated(ChangeMap const& changed, KeyList const& removed)
{
   SignalModified();
   if (updated_) {
      updated_(changed, removed);
   } else {
      if (changed_) {
         for (const auto& entry : changed) {
            changed_(entry.first, entry.second);
         }
      }

      if (removed_) {
         for (const auto& key : removed) {
            removed_(key);
         }
      }
   }
}

template <typename M>
void MapTrace<M>::NotifyObjectState(typename M::ContainerType const& contents)
{
   SignalUpdated(contents, KeyList());
}

#define CREATE_MAP(M)    template MapTrace<M>;
#include "types/all_map_types.h"
ALL_DM_MAP_TYPES
