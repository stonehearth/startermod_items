#include "radiant.h"
#include "store.h"
#include "map.h"
#include "map_trace.h"
#include "streamer.h"
#include "dbg_indenter.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"
#include "streamer.h"

using namespace radiant;
using namespace radiant::dm;

#define MAP_LOG(level)  LOG(dm.map, level) << "[map <" << GetShortTypeName<K>() << "," << GetShortTypeName<V>() << "> id:" << GetObjectId() << "] "

template <class K, class V, class H, class MH>
std::shared_ptr<MapTrace<Map<K, V, H, MH>>> Map<K, V, H, MH>::TraceChanges(const char* reason, int category) const
{
   return GetStore().TraceMapChanges(reason, *this, category);
}

template <class K, class V, class H, class MH>
TracePtr Map<K, V, H, MH>::TraceObjectChanges(const char* reason, int category) const
{
   return GetStore().TraceMapChanges(reason, *this, category);
}

template <class K, class V, class H, class MH>
TracePtr Map<K, V, H, MH>::TraceObjectChanges(const char* reason, Tracer* tracer) const
{
   return GetStore().TraceMapChanges(reason, *this, tracer);
}

template <class K, class V, class H, class MH>
void Map<K, V, H, MH>::LoadValue(SerializationType r, Protocol::Value const& value)
{
   auto msg = value.GetExtension(Protocol::Map::extension);
   K key;
   V val;

   for (Protocol::Map::Entry const& entry : msg.added()) {
      SaveImpl<K>::LoadValue(GetStore(), r, entry.key(), key);
      SaveImpl<V>::LoadValue(GetStore(), r, entry.value(), val);
      Add(key, val);
   }
   for (Protocol::Value const& removed : msg.removed()) {
      SaveImpl<K>::LoadValue(GetStore(), r, removed, key);
      Remove(key);
   }
}

template <class K, class V, class H, class MH>
void Map<K, V, H, MH>::SaveValue(SerializationType r, Protocol::Value* value) const
{
   Store const& store = GetStore();
   Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

   for (auto const& entry : items_) {
      Protocol::Map::Entry* submsg = msg->add_added();
      SaveImpl<Key>::SaveValue(store, r, submsg->mutable_key(), entry.first);
      SaveImpl<Value>::SaveValue(store, r, submsg->mutable_value(), entry.second);
   }
}

template <class K, class V, class H, class MH>
void Map<K, V, H, MH>::GetDbgInfo(DbgInfo &info) const {
   if (WriteDbgInfoHeader(info)) {
      info.os << " {" << std::endl;
      {
         Indenter indent(info.os);
         auto i = items_.begin(), end = items_.end();
         while (i != end) {
            SaveImpl<K>::GetDbgInfo(i->first, info);
            info.os << " : ";
            SaveImpl<V>::GetDbgInfo(i->second, info);
            if (++i != end) {
               info.os << ",";
            }
            info.os << std::endl;
         }
      }
      info.os << "}";
   }
}


template <class K, class V, class H, class MH>
int Map<K, V, H, MH>::GetSize() const
{
   return items_.size();
}

template <class K, class V, class H, class MH>
bool Map<K, V, H, MH>::IsEmpty() const
{
   return items_.empty();
}

template <typename T> bool Equals(T const& lhs, T const& rhs) {
   return lhs == rhs;
}

template <typename T> bool Equals(std::weak_ptr<T> const& lhs, std::weak_ptr<T> const& rhs) {
   return lhs.lock() == rhs.lock();
}

template <class K, class V, class H, class MH>
void Map<K, V, H, MH>::Add(K const& key, V const& value) {
   auto i = items_.find(MH()(key));
   if (i != items_.end()) {
      i->second = value;
      MAP_LOG(7) << "modifying " << key << " (size: " << items_.size() << ")";
      GetStore().OnMapChanged(*this, key, value);
   } else {
      items_[MH()(key)] = value;
      lastErasedIterator_ = items_.end();
      MAP_LOG(7) << "adding " << key << " (size: " << items_.size() << ")";
      GetStore().OnMapChanged(*this, key, value);
   }
}

template <class K, class V, class H, class MH>
void Map<K, V, H, MH>::Remove(const K& key) {
   auto i = items_.find(MH()(key));
   if (i != items_.end()) {
      MAP_LOG(7) << "removing " << key;
      lastErasedIterator_ = items_.erase(i);
      GetStore().OnMapRemoved(*this, key);
   }
}

template <class K, class V, class H, class MH>
bool Map<K, V, H, MH>::Contains(const K& key) const {
   return items_.find(MH()(key)) != items_.end();
}

template <class K, class V, class H, class MH>
V Map<K, V, H, MH>::Get(const K& key, V default) const {
   auto i = items_.find(MH()(key));
   return i != items_.end() ? i->second : default;
}

template <class K, class V, class H, class MH>
void Map<K, V, H, MH>::Clear() {
   MAP_LOG(7) << "clearing";
   while (!items_.empty()) {
      Remove(items_.begin()->first);
   }
}

template <class K, class V, class H, class MH>
typename Map<K, V, H, MH>::ContainerType const& Map<K, V, H, MH>::GetContainer() const
{
   MAP_LOG(7) << "returning container size " << items_.size();
   return items_;
}

template <class K, class V, class H, class MH>
typename Map<K, V, H, MH>::Iterator Map<K, V, H, MH>::begin() const
{
   return Iterator(*this, items_.begin());
}

template <class K, class V, class H, class MH>
typename Map<K, V, H, MH>::Iterator Map<K, V, H, MH>::end() const
{
   return Iterator(*this, items_.end());
}

template <class K, class V, class H, class MH>
typename Map<K, V, H, MH>::Iterator Map<K, V, H, MH>::find(K const& key) const
{
   return Iterator(*this, items_.find(MH()(key)));
}

template <class K, class V, class H, class MH>
typename Map<K, V, H, MH>::ContainerType::const_iterator const& Map<K, V, H, MH>::GetLastErasedIterator() const
{
   return lastErasedIterator_;
}

template <class K, class V, class H, class MH>
void Map<K, V, H, MH>::RegisterWithStreamer(Streamer* streamer) const
{
   // Ask the streamer to install a trace on us for object remoting.  See
   // Streamer::TraceObject for more details.
   streamer->TraceObject<Map<K, V, H, MH>>(this);
}

#define CREATE_MAP(M)    template M;
#include "types/all_map_types.h"
#include "types/all_loader_types.h"
ALL_DM_MAP_TYPES
