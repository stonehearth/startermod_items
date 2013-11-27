#include "radiant.h"
#include "record.h"
#include "record_trace_buffered.h"
#include "tracer_buffered.h"
#include "store.h"
#include "dm_log.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename R>
RecordTraceBuffered<R>::RecordTraceBuffered(const char* reason, Record const& r, Tracer& tracer) :
   RecordTrace(reason, r, tracer),
   first_save_(true),
   changed_(false)
{
   TRACE_LOG(5) << "creating record trace buffered for object " << GetObjectId();
}

template <typename R>
void RecordTraceBuffered<R>::Flush()
{
   ASSERT(changed_);
   if (changed_) {
      SignalModified();
      changed_ = false;
   }
}

template <typename R>
void RecordTraceBuffered<R>::SaveObjectDelta(Protocol::Value* value)
{
   TRACE_LOG(5) << "saving record trace buffered delta for object " << GetObjectId() << " (first: " << first_save_ <<")";
   if (first_save_) {
      GetRecord().SaveValue(value);
      first_save_ = false;
   }
   // A record gets all it's fields initialized prior to the first save and
   // never changes thereafter.  So there's nothing to do!
}

template <typename R>
void RecordTraceBuffered<R>::NotifyRecordChanged()
{
   changed_ = true;
}

template RecordTraceBuffered<Record>;
