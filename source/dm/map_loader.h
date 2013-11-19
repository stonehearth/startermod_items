#ifndef _RADIANT_DM_MAP_LOADER_H
#define _RADIANT_DM_MAP_LOADER_H

#include "dm.h"
#include "protocols/store.pb.h"
#include "protocol.h"
#include "map.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename K, typename V, typename H>
static void Load(Map<K, V, H>& map, Protocol::Value const& value)
{
   const Protocol::Map::Update& msg = value.GetExtension(Protocol::Map::extension);
   K key;
   V value;

   for (const Protocol::Map::Entry& entry : msg.added()) {
      SaveImpl<K>::LoadValue(store, entry.key(), key);
      SaveImpl<V>::LoadValue(store, entry.value(), value);
      map.Insert(key, value);
   }
   for (const Protocol::Value& removed : msg.removed()) {
      SaveImpl<K>::LoadValue(store, removed, key);
      map.Remove(key);
   }
}

template <typename K, typename V, typename H>
void SaveObject(Map<K, V, H> const& map, Protocol::Value* msg)
{
   NOT_YET_IMPLEMENTED();
}

template <typename K, typename V, typename H>
static void SaveObjectDelta(std::unordered_map<K, V, H> const& changed,
                            std::vector<typename Map<K, V, H>::Key> const& removed,
                            Protocol::Value* value)
{
   Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

   for (auto const& entry : changed) {
      Protocol::Map::Entry* submsg = msg->add_added();
      SaveImpl<K>::SaveValue(store, submsg->mutable_key(), k);
      SaveImpl<V>::SaveValue(store, submsg->mutable_value(), v);
   }
   for (auto const& key : removed) {
      SaveImpl<K>::SaveValue(store, msg->add_removed(), k);
   }
}

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_LOADER_H

