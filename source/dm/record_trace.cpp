#include "radiant.h"
#include "record.h"
#include "record_trace.h"
#include "store.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename R>
RecordTrace<R>::RecordTrace(const char* reason, Record const& r, Tracer& tracer) :
   TraceImpl(reason),
   record_(r)
{
   for (const auto& field : r.GetFields()) {
      auto t = r.GetStore().FetchStaticObject(field.second)->TraceObjectChanges(reason, &tracer);
      t->OnModified_([=]() {
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
