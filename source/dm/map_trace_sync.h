#ifndef _RADIANT_DM_MAP_TRACE_SYNC_H
#define _RADIANT_DM_MAP_TRACE_SYNC_H

#include "dm.h"
#include "map_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTraceSync : public MapTrace<M>
{
public:
   MapTraceSync(const char* reason) :
      MapTrace(reason)
   {
   }

private:
   void OnRemoved(Key const& key) override
   {
      SignalRemoved(key);
   }

   void OnChanged(Key const& key, Key const& value) override
   {
      SignalChanged(key, value);
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_SYNC_H

