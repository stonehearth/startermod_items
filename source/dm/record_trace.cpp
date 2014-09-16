#include "radiant.h"
#include "record.h"
#include "record_trace.h"
#include "store.h"
#include "dm_save_impl.h"
#include "map_util.h" // xxx: for CStringKeyTransform, which should move to core or something!
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

static SafeCStringTable safeCStrings;

#define TRACE_LOG(level) LOG(dm.trace.record, level) << "[record trace '" << GetReason() << "' id:" << record_.GetObjectId() << "] "

template <typename R>
RecordTrace<R>::RecordTrace(const char* reason, Record const& r, Tracer& tracer) :
   TraceImpl(reason),
   record_(r)
{
   TRACE_LOG(7) << "installing record trace";

   for (const auto& field : r.GetFields()) {
      const char* name = field.first.c_str();
      const char* freason = safeCStrings(BUILD_STRING(reason << "(tracing field " << name << ")").c_str());

      auto t = r.GetStore().FetchStaticObject(field.second)->TraceObjectChanges(freason, &tracer);
      t->OnModified_([this, name]() {
         TRACE_LOG(9) << "field " << name << " changed.  notifying record changed, too!";

         // First, signal that the record itself has actually change, too.  This will add all existing
         // buffered record traces to their respective tracers.
         record_.GetStore().OnRecordFieldChanged(record_);
         
         // Now notify that the record has changed.  This will either call the record trace OnModified
         // callback (for sync traces) or set the changed bit so they will get called at Flush() time
         // (for buffered ones).
         NotifyRecordChanged();
      });
      field_traces_.push_back(t);
   }
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
