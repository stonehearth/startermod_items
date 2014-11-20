#include "radiant.h"
#include "record.h"
#include "record_trace.h"
#include "store.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

#define TRACE_LOG(level) LOG(dm.trace.record, level) << "[record trace '" << GetReason() << "' id:" << record_.GetObjectId() << "] "

template <typename R>
RecordTrace<R>::RecordTrace(const char* reason, Record const& r, Tracer& tracer) :
   TraceImpl(reason),
   record_(r)
{
   TRACE_LOG(7) << "installing record trace";
}

template <typename R>
ObjectId RecordTrace<R>::GetObjectId() const
{
   return record_.GetObjectId();
}

template <typename R>
Record const& RecordTrace<R>::GetRecord() const
{
   return record_;
}

template <typename R>
void RecordTrace<R>::SignalObjectState()
{
   // PushObjectState() of a record needs to do something!
   SignalModified();
}

template RecordTrace<Record>;
