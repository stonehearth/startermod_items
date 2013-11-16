#ifndef _RADIANT_DM_MARSHALL_H
#define _RADIANT_DM_MARSHALL_H

#include "namespace.h"
#include "dm_save_impl.h"
#include "map_tracker_buffered.h"

BEGIN_RADIANT_DM_NAMESPACE

class Marshall
{
public:
   Marshall(const char* reason) :
      reason_(reason)
   {
   }

   template <class K, class V, class Hash>
   void AddObject(Map<K, V, Hash> const &map) {
      typedef MapTrackerBuffered<Map<K, V, Hash>> MBT;

      auto encode = [](MTB::EntryList const& changed, MTB::KeyList const& removed) {

         for (const auto& entry : changed) {
            Protocol::Map::Entry* submsg = msg->add_added();
            SaveImpl<K>::SaveValue(store, submsg->mutable_key(), entry.first);
            SaveImpl<V>::SaveValue(store, submsg->mutable_value(), entry.second);
         }

         for (const auto& key : removed) {
            SaveImpl<K>::SaveValue(store, msg->add_removed(), key);
         }
      };
      ObjectId id = map.GetObjectId();
      trackers_[id] = new MTB(reason_, map, encode);
   }

   void Flush(Protocol::Object* msg)
   {
      Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);
   }

private:
   const char* reason;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MARSHALL_H

