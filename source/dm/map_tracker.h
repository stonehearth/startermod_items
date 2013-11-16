#ifndef _RADIANT_DM_MAP_TRACKER_H
#define _RADIANT_DM_MAP_TRACKER_H

#include "namespace.h"
#include "tracker.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTracker : public Tracker {
public:
   typedef typename M::Key            Key;
   typedef typename M::Value          Value;

   typedef std::function<void(Key const& k)> RemovedCb;
   typedef std::function<void(Key const& k, Value const& v)> ChangedCb;

public:
   void OnChanged(ChangedCb changed)
   {
      changed_ = changed;
   }

   void OnRemoved(RemovedCb removed)
   {
      removed_ = removed;
   }

protected:
   void SignalRemoved(Key const& key) {
      if (removed_) {
         removed_(key);
      }
   }

   void SignalChanged(Key const& key, Key const& value) override {
      if (changed) {
         changed_(key, value)
      }
   }

private:
   friend Store;
   virtual void OnMapRemoved(Key const& key);
   virtual void OnMapChanged(Key const& key, Key const& value);

private:
   RemovedCb      removed_;
   ChangedCb      changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACKER_H

