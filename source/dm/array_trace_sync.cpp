#include "radiant.h"
#include "array.h"
#include "array_trace_sync.h"

using namespace radiant;
using namespace radiant::dm;

template <typename A>
ArrayTraceSync<A>::ArrayTraceSync(const char* reason, Object const& o, Store const& store) :
   ArrayTrace(reason, o, store)
{
}

template <typename A>
void ArrayTraceSync<A>::NotifySet(uint i, Value const& value)
{
   SignalChanged(i, value);
}

template <typename A>
void ArrayTraceSync<A>::NotifyDestroyed()
{
   SignalDestroyed();
}