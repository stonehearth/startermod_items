#ifndef _RADIANT_DM_MAP_TRACKER_SYNC_H
#define _RADIANT_DM_MAP_TRACKER_SYNC_H

#include "namespace.h"
#include "tracker.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTrackerSync : public MapTracker<M> {
public:
   MapTrackerSync(const char* reason) :
      MapTracker(reason)
   {
   }

private:
   void OnMapRemoved(Key const& key) override
   {
      SignalRemoved(key);
   }

   void OnMapChanged(Key const& key, Key const& value) override
   {
      SignalChanged(key, value);
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACKER_SYNC_H

