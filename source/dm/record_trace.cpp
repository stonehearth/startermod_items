#include "radiant.h"
#include "record.h"
#include "record_trace.h"
#include "store.h"
#include "dm_save_impl.h"
#include "object_trace.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename R>
RecordTrace<R>::RecordTrace(const char* reason, Record const& r, Store& s, Tracer* tracer) :
   TraceImpl(reason, r, s)
{
   for (const auto& field : r.GetFields()) {
      auto t = s.FetchStaticObject(field.second)->TraceObjectChanges(reason, tracer);
      t->OnModified([=]() {
         NotifyRecordChanged();
      });
      field_traces_.push_back(t);
   }
}

template RecordTrace<Record>;
