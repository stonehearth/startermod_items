#ifndef _RADIANT_DM_MAP_TRACKER_BUFFERED_H
#define _RADIANT_DM_MAP_TRACKER_BUFFERED_H

#include "namespace.h"
#include "map_tracker.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTrackerBuffered : public MapTracker<M> {
public:
   void Flush()
   {
      for (const auto& entry : changed_) {
         SignalChanged(entry.first, entry.second);
      }

      for (const auto& key : removed_) {
         SignalRemoved(key);
      }
      changed_.clear();
      removed_.clear();
   }

private:
   void OnMapRemoved(Key const& key) override
   {
      stdutil::FastRemove(changed_, key);
      removed_.push_back(key);
   }

   void OnMapChanged(Key const& key, Key const& value) override
   {
      stdutil::FastRemove(removed_, key);
      changed_.push_back(key);
   }

private:
   EntryList      changed_;
   KeyList        removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACKER_BUFFERED_H

