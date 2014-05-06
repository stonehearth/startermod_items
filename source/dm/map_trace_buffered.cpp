#include "radiant.h"
#include "radiant_stdutil.h"
#include "map.h"
#include "map_trace_buffered.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

#define TRACE_LOG(level)            LOG(dm.trace.map, level) << "[buf map trace<" << GetShortTypeName<M::Key>() << "," << GetShortTypeName<M::Value>() << "> " << GetReason() << " id:" << GetMapObjectId() << "] "
#define TRACE_LOG_ENABLED(level)    LOG_IS_ENABLED(dm.trace.map, level)

template <typename M>
MapTraceBuffered<M>::MapTraceBuffered(const char* reason, M const& m) :
   MapTrace(reason, m),
   firing_(false)
{
   TRACE_LOG(5) << "creating trace";
}

template <typename M>
MapTraceBuffered<M>::~MapTraceBuffered()
{
}

template <typename M>
void MapTraceBuffered<M>::Flush()
{
   if (TRACE_LOG_ENABLED(9)) {
      TRACE_LOG(5) << "flushing trace";
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
bool MapTraceBuffered<M>::SaveObjectDelta(SerializationType r, Protocol::Value* value)
{
   Store const& store = GetStore();
   Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

   TRACE_LOG(5) << "saving trace";

   if (!changed_.empty() || !removed_.empty()) {
      for (auto const& entry : changed_) {
         Protocol::Map::Entry* submsg = msg->add_added();
         SaveImpl<Key>::SaveValue(store, r, submsg->mutable_key(), entry.first);
         SaveImpl<Value>::SaveValue(store, r, submsg->mutable_value(), entry.second);
      }
      for (auto const& key : removed_) {
         SaveImpl<Key>::SaveValue(store, r, msg->add_removed(), key);
      }
      return true;
   }
   return false;
}

template <typename M>
void MapTraceBuffered<M>::NotifyRemoved(Key const& key)
{
   TRACE_LOG(5) << "removing " << key << " from changed set firing:" << firing_;
   if (TRACE_LOG_ENABLED(9)) {
      for (const auto& entry : changed_) {
         TRACE_LOG(9) << "  changed: " << entry.first;
      }
      for (const auto& key: removed_) {
         TRACE_LOG(9) << "  removed: " << key;
      }
   }

   if (!firing_) {
      changed_.erase(key);
      stdutil::UniqueInsert(removed_, key);
   } else {
      pending_changed_.erase(key);
      stdutil::UniqueInsert(pending_removed_, key);
   }
}

template <typename M>
void MapTraceBuffered<M>::NotifyChanged(Key const& key, Value const& value)
{
   TRACE_LOG(5) << "adding " << key << " to changed set firing: " << firing_;
   if (TRACE_LOG_ENABLED(9)) {
      for (const auto& entry : changed_) {
         TRACE_LOG(9) << "  changed: " << entry.first;
      }
      for (const auto& key: removed_) {
         TRACE_LOG(9) << "  removed: " << key;
      }
   }

   if (!firing_) {
      stdutil::FastRemove(removed_, key);
      changed_[key] = value;
   } else {
      stdutil::FastRemove(pending_removed_, key);
      pending_changed_[key] = value;
   }
}

template <typename M>
void MapTraceBuffered<M>::ClearCachedState()
{
   TRACE_LOG(7) << "clearing cached state";
   changed_ = std::move(pending_changed_);
   removed_ = std::move(pending_removed_);
}

#define CREATE_MAP(M)  template MapTraceBuffered<M>;
#include "types/all_loader_types.h"
#include "types/all_map_types.h"
ALL_DM_MAP_TYPES
