#include "radiant.h"
#include "map.h"
#include "map_trace_sync.h"

using namespace radiant;
using namespace radiant::dm;

template <typename M>
MapTraceSync<M>::MapTraceSync(const char* reason, Object const& o, Store const& store) :
   MapTrace(reason, o, store)
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

#undef DM_MAP
#define DM_MAP(k, v)  template MapTraceSync<Map<k, v>>;
#include "all_objects.h"

