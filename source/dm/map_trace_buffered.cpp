#include "radiant.h"
#include "radiant_stdutil.h"
#include "map.h"
#include "map_trace_buffered.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

#define TRACE_LOG(level)            LOG_CATEGORY(dm.trace.map, level, "buf map<" << GetShortTypeName<M::Key>() << "," << GetShortTypeName<M::Value>() << "> " << GetReason())
#define TRACE_LOG_ENABLED(level)    LOG_IS_ENABLED(dm.trace.map, level)

template <typename M>
MapTraceBuffered<M>::MapTraceBuffered(const char* reason, M const& m) :
   MapTrace(reason, m),
   firing_(false)
{
   TRACE_LOG(5) << "creating trace for object " << GetObjectId();
}

template <typename M>
void MapTraceBuffered<M>::Flush()
{
   if (TRACE_LOG_ENABLED(9)) {
      TRACE_LOG(5) << "flushing trace for object " << GetObjectId();
      for (const auto& entry : changed_) {
         TRACE_LOG(9) << "  changed: " << entry.first;
      }
      for (const auto& key: removed_) {
         TRACE_LOG(9) << "  removed: " << key;
      }
   }

   firing_ = true;
   if (!changed_.empty() || !removed_.empty()) {
      SignalUpdated(changed_, removed_);
      ClearCachedState();
   }
   firing_ = false;
}

template <typename M>
bool MapTraceBuffered<M>::SaveObjectDelta(Protocol::Value* value)
{
   Store const& store = GetStore();
   Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

   TRACE_LOG(5) << "saving trace for object " << GetObjectId();

   if (!changed_.empty() || !removed_.empty()) {
      for (auto const& entry : changed_) {
         Protocol::Map::Entry* submsg = msg->add_added();
         SaveImpl<Key>::SaveValue(store, submsg->mutable_key(), entry.first);
         SaveImpl<Value>::SaveValue(store, submsg->mutable_value(), entry.second);
      }
      for (auto const& key : removed_) {
         SaveImpl<Key>::SaveValue(store, msg->add_removed(), key);
      }
      return true;
   }
   return false;
}

template <typename M>
void MapTraceBuffered<M>::NotifyRemoved(Key const& key)
{
   ASSERT(!firing_);
   TRACE_LOG(5) << "removing " << key << " from changed set";
   if (TRACE_LOG_ENABLED(9)) {
      for (const auto& entry : changed_) {
         TRACE_LOG(9) << "  changed: " << entry.first;
      }
      for (const auto& key: removed_) {
         TRACE_LOG(9) << "  removed: " << key;
      }
   }

   changed_.erase(key);
   stdutil::UniqueInsert(removed_, key);

}

template <typename M>
void MapTraceBuffered<M>::NotifyChanged(Key const& key, Value const& value)
{
   ASSERT(!firing_);
   TRACE_LOG(5) << "adding " << key << " to changed set";
   if (TRACE_LOG_ENABLED(9)) {
      for (const auto& entry : changed_) {
         TRACE_LOG(9) << "  changed: " << entry.first;
      }
      for (const auto& key: removed_) {
         TRACE_LOG(9) << "  removed: " << key;
      }
   }

   stdutil::FastRemove(removed_, key);
   changed_[key] = value;
}

template <typename M>
void MapTraceBuffered<M>::ClearCachedState()
{
   changed_.clear();
   removed_.clear();
}

#define CREATE_MAP(M)  template MapTraceBuffered<M>;
#include "types/all_loader_types.h"
#include "types/all_map_types.h"
ALL_DM_MAP_TYPES
