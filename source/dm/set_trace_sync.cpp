#include "radiant.h"
#include "set.h"
#include "set_trace_sync.h"

using namespace radiant;
using namespace radiant::dm;

template <typename S>
SetTraceSync<S>::SetTraceSync(const char* reason, S const& set) :
   SetTrace(reason, set)
{
}

template <typename S>
void SetTraceSync<S>::NotifyAdded(Value const& value)
{
   SignalAdded(value);
}

template <typename S>
void SetTraceSync<S>::NotifyRemoved(Value const& value)
{
   SignalRemoved(value);
}

template <typename S>
void SetTraceSync<S>::NotifyDestroyed()
{
   SignalDestroyed();
}

#define CREATE_SET(S)  template SetTraceSync<S>;
#include "types/all_set_types.h"
ALL_DM_SET_TYPES
