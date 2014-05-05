#include "radiant.h"
#include "map.h"
#include "map_trace.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

#define TRACE_LOG(level)            LOG(dm.trace.map, level) << "[map trace <" << GetShortTypeName<M::Key>() << "," << GetShortTypeName<M::Value>() << "> " << GetReason() << " id:" << GetMapObjectId() << "] "

template <typename M>
MapTrace<M>::MapTrace(const char* reason, M const& m) :
   TraceImpl(reason),
   map_(m)
{
}

template <typename M>
std::shared_ptr<MapTrace<M>> MapTrace<M>::OnAdded(ChangedCb changed)
{
   changed_ = changed;
   return shared_from_this();
}


template <typename M>
ObjectId MapTrace<M>::GetObjectId() const
{
   return map_.GetObjectId();
}

template <typename M>
Store const& MapTrace<M>::GetStore() const
{
   return map_.GetStore();
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
void MapTrace<M>::SignalObjectState()
{
   TRACE_LOG(9) << "signaling object state of " << map_.Size() << " entries";
   SignalUpdated(map_.GetContainer(), KeyList());
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
ObjectId MapTrace<M>::GetMapObjectId() const
{
   return map_.GetObjectId();
}

#define CREATE_MAP(M)    template MapTrace<M>;
#include "types/all_map_types.h"
ALL_DM_MAP_TYPES
