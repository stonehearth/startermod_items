#include "radiant.h"
#include "record.h"
#include "record_trace_sync.h"

using namespace radiant;
using namespace radiant::dm;

template <typename R>
RecordTraceSync<R>::RecordTraceSync(const char* reason, Record const& r, Tracer& tracer) :
   RecordTrace(reason, r, tracer)
{
}

template <typename R>
void RecordTraceSync<R>::NotifyRecordChanged()
{
   SignalModified();
}

template <typename R>
void RecordTraceSync<R>::NotifyDestroyed()
{
   SignalDestroyed();
}

template RecordTraceSync<Record>;
