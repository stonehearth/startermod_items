#include "radiant.h"
#include "map.h"
#include "map_trace_sync.h"
#include "store.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename M>
MapTraceBuffered<M>::MapTraceBuffered(const char* reason, Object const& o, Store const& store) :
   MapTrace(reason, o, store)
{
}

template <typename M>
void MapTraceBuffered<M>::Flush()
{
   SignalUpdated(changed_, removed_);
   changed_.clear();
   removed_.clear();
}

template <typename M>
void MapTraceBuffered<M>::SaveObjectDelta(Object* obj, Protocol::Value* value)
{
   Store const& store = GetStore();
   Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

   for (auto const& entry : changed_) {
      Protocol::Map::Entry* submsg = msg->add_added();
      SaveImpl<Key>::SaveValue(store, submsg->mutable_key(), entry.first);
      SaveImpl<Value>::SaveValue(store, submsg->mutable_value(), entry.second);
   }
   for (auto const& key : removed_) {
      SaveImpl<Key>::SaveValue(store, msg->add_removed(), key);
   }
}

template <typename M>
void MapTraceBuffered<M>::NotifyRemoved(Key const& key)
{
   changed_.erase(key);
   removed_.push_back(key);
}

template <typename M>
void MapTraceBuffered<M>::NotifyChanged(Key const& key, Value const& value)
{
   stdutil::FastRemove(removed_, key);
   changed_[key] = value;
}

template <typename M>
void MapTraceBuffered<M>::NotifyObjectState(typename M::ContainerType const& contents)
{
   changed_.clear();
   removed_.clear();
   MapTrace<M>::PushObjectState();
}

#define DM_MAP(k, v)  template MapTraceBuffered<Map<k, v>>;
#include "defs/maps.h"
