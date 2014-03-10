#include "radiant.h"
#include "store.h"
#include "map.h"
#include "map_trace.h"
#include "dbg_indenter.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <class K, class V, class H>
std::shared_ptr<MapTrace<Map<K, V, H>>> Map<K, V, H>::TraceChanges(const char* reason, int category) const
{
   return GetStore().TraceMapChanges(reason, *this, category);
}

template <class K, class V, class H>
TracePtr Map<K, V, H>::TraceObjectChanges(const char* reason, int category) const
{
   return GetStore().TraceMapChanges(reason, *this, category);
}

template <class K, class V, class H>
TracePtr Map<K, V, H>::TraceObjectChanges(const char* reason, Tracer* tracer) const
{
   return GetStore().TraceMapChanges(reason, *this, tracer);
}

template <class K, class V, class H>
void Map<K, V, H>::LoadValue(SerializationType r, Protocol::Value const& value)
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

template <class K, class V, class H>
void Map<K, V, H>::SaveValue(SerializationType r, Protocol::Value* value) const
{
   Store const& store = GetStore();
   Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

   for (auto const& entry : items_) {
      Protocol::Map::Entry* submsg = msg->add_added();
      SaveImpl<Key>::SaveValue(store, r, submsg->mutable_key(), entry.first);
      SaveImpl<Value>::SaveValue(store, r, submsg->mutable_value(), entry.second);
   }
}

template <class K, class V, class H>
void Map<K, V, H>::GetDbgInfo(DbgInfo &info) const {
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


template <class K, class V, class H>
int Map<K, V, H>::GetSize() const
{
   return items_.size();
}

template <class K, class V, class H>
bool Map<K, V, H>::IsEmpty() const
{
   return items_.empty();
}

template <class K, class V, class H>
typename Map<K, V, H>::ContainerType const & Map<K, V, H>::GetContents() const
{
   return items_;
}

template <class K, class V, class H>
typename Map<K, V, H>::ContainerType::const_iterator Map<K, V, H>::Remove(typename ContainerType::const_iterator i)
{
   K key = i->first;
   auto result = items_.erase(i);
   GetStore().OnMapRemoved(*this, key);
   return result;
}

template <class K, class V, class H>
void Map<K, V, H>::Add(K const& key, V const& value) {
   auto result = items_.insert(std::make_pair(key, value));
   if (result.second) {
      GetStore().OnMapChanged(*this, key, value);
   }
}

template <class K, class V, class H>
void Map<K, V, H>::Remove(const K& key) {
   auto i = items_.find(key);
   if (i != items_.end()) {
      Remove(i);
   }
}

template <class K, class V, class H>
bool Map<K, V, H>::Contains(const K& key) const {
   return items_.find(key) != items_.end();
}

template <class K, class V, class H>
V Map<K, V, H>::Get(const K& key, V default) const {
   auto i = items_.find(key);
   return i != items_.end() ? i->second : default;
}

template <class K, class V, class H>
void Map<K, V, H>::Clear() {
   while (!items_.empty()) {
      Remove(items_.begin()->first);
   }
}

template <class K, class V, class H>
typename Map<K, V, H>::ContainerType const& Map<K, V, H>::GetContainer() const
{
   return items_;
}

#define CREATE_MAP(M)    template M;
#include "types/all_map_types.h"
#include "types/all_loader_types.h"
ALL_DM_MAP_TYPES
