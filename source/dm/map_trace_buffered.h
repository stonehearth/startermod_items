#ifndef _RADIANT_DM_MAP_TRACE_BUFFERED_H
#define _RADIANT_DM_MAP_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "map_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTraceBuffered : public TraceBuffered,
                         public MapTrace<M> {
public:
   void Flush()
   {
      SignalUpdated(changed_, removed_);
      changed_.clear();
      removed_.clear();
   }

private:
   void NotifyRemoved(Key const& key) override
   {
      changed_.erase(key);
      removed_.push_back(key);
   }

   void NotifyChanged(Key const& key, Value const& value) override
   {
      stdutil::FastRemove(removed_, key);
      changed_[key] = value;
   }

private:
   ChangeMap            changed_;
   KeyList              removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_BUFFERED_H

