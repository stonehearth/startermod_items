#ifndef _RADIANT_DM_MAP_TRACKER_SYNC_H
#define _RADIANT_DM_MAP_TRACKER_SYNC_H

#include "namespace.h"
#include "tracker.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapStreamer {
public:
   MapStreamer(const char* reason, M const& map) {
      tracker_ = map.GetStore().TrackMapChanges(map);
   }

   void Flush(Protocol::Object* msg)
   {
      msg->set_object_id(id_.id);
      msg->set_object_type(GetObjectType());
      msg->set_timestamp(timestamp_);

      Protocol::Value* value = msg->mutable_value();
      Protocol::Map::Update* msg = value->MutableExtension(Protocol::Map::extension);

      auto changed = [msg](Key const& k, Value const& v) {
         Protocol::Map::Entry* submsg = msg->add_added();
         SaveImpl<K>::SaveValue(store, submsg->mutable_key(), k);
         SaveImpl<V>::SaveValue(store, submsg->mutable_value(), v);
      };
      auto removed = [msg](Key const& k) {
         SaveImpl<K>::SaveValue(store, msg->add_removed(), k);
      };

      tracker_->Flush(changed, removed);
   }

private:
   MapTrackerBuffered   *tracker_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACKER_SYNC_H

