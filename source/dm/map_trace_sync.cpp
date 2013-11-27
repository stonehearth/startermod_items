#include "radiant.h"
#include "map.h"
#include "map_trace_sync.h"

using namespace radiant;
using namespace radiant::dm;

template <typename M>
MapTraceSync<M>::MapTraceSync(const char* reason, M const& m) :
   MapTrace(reason, m)
{
}

template <typename M>
void MapTraceSync<M>::NotifyRemoved(Key const& key)
{
   SignalRemoved(key);
}

template <typename M>
void MapTraceSync<M>::NotifyChanged(Key const& key, Value const& value)
{
   SignalChanged(key, value);
}

template <typename M>
void MapTraceSync<M>::NotifyDestroyed()
{
   SignalDestroyed();
}

#define CREATE_MAP(M)  template MapTraceSync<M>;
#include "types/all_map_types.h"
ALL_DM_MAP_TYPES
