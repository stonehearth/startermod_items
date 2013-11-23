#include "radiant.h"
#include "record.h"
#include "record_trace_buffered.h"
#include "store.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename R>
RecordTraceBuffered<R>::RecordTraceBuffered(const char* reason, Record const& r, Store& s, Tracer* tracer) :
   RecordTrace(reason, r, s, tracer),
   first_save_(true),
   changed_(false)
{
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
void RecordTraceBuffered<R>::SaveObjectDelta(Object* obj, Protocol::Value* value)
{
   if (first_save_) {
      static_cast<Record*>(obj)->SaveValue(value);
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
