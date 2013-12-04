#ifndef _RADIANT_DM_MAP_TRACE_SYNC_H
#define _RADIANT_DM_MAP_TRACE_SYNC_H

#include "map_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTraceSync : public MapTrace<M>
{
public:
public:
   MapTraceSync(const char* reason, M const& m);

private:
   void NotifyRemoved(Key const& key) override;
   void NotifyChanged(Key const& key, Value const& value) override;
   void NotifyDestroyed() override;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_SYNC_H

