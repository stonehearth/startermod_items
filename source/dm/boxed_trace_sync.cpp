#include "radiant.h"
#include "boxed.h"
#include "boxed_trace_sync.h"

using namespace radiant;
using namespace radiant::dm;

template <typename B>
BoxedTraceSync<B>::BoxedTraceSync(const char* reason, Object const& o, Store const& store) :
   BoxedTrace(reason, o, store)
{
}

template <typename B>
void BoxedTraceSync<B>::NotifyChanged(Value const& value)
{
   SignalChanged(value);
}

template <typename B>
void BoxedTraceSync<B>::NotifyDestroyed()
{
   SignalDestroyed();
}

#define CREATE_BOXED(B)           template BoxedTraceSync<B>;
#include "types/all_boxed_types.h"
ALL_DM_BOXED_TYPES

