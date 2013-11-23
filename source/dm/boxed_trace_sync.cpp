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

#define DM_BOXED(v) template BoxedTraceSync<Boxed<v>>;
#include "defs/boxes.h"
