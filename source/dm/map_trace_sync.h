#ifndef _RADIANT_DM_MAP_TRACE_SYNC_H
#define _RADIANT_DM_MAP_TRACE_SYNC_H

#include "dm.h"
#include "map_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTraceSync : public MapTrace<M>
{
public:
public:
   MapTraceSync(const char* reason, Object const& o, Store const& store) :
      MapTrace(reason, o, store)
   {
   }

private:
   void NotifyRemoved(Key const& key) override
   {
      SignalRemoved(key);
   }

   void NotifyChanged(Key const& key, Key const& value) override
   {
      SignalChanged(key, value);
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_SYNC_H

