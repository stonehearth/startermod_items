#ifndef _RADIANT_DM_MAP_MARSHALL_H
#define _RADIANT_DM_MAP_MARSHALL_H

#include "namespace.h"
#include "dm_save_impl.h"
#include "map_tracker_buffered.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapMarshall
{
public:
   typedef std::vector<typename M::Key>               KeyList;
   typedef std::vector<std::pair<M::Key, M::Value>>   EntryList;

   MapMarshall(MapTrackerBuffered<M>& tracker) :
      tracker_(tracker)
   {
   }

   void Encode(Protocol::Value* value) {
      tracker_.Flush([this](EntryList const& changed, KeyList const& removed) {
         Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

         for (const auto& entry : changed) {
            Protocol::Map::Entry* submsg = msg->add_added();
            SaveImpl<K>::SaveValue(store, submsg->mutable_key(), entry.first);
            SaveImpl<V>::SaveValue(store, submsg->mutable_value(), entry.second);
         }

         for (const auto& key : removed) {
            SaveImpl<K>::SaveValue(store, msg->add_removed(), key);
         }
      });
   }

private:
   MapTrackerBuffered<M>& tracker;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_MARSHALL_H

