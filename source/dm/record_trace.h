#ifndef _RADIANT_DM_RECORD_TRACE_H
#define _RADIANT_DM_RECORD_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class RecordTrace : public Trace
{
public:
   RecordTrace(const char* reason, Record const& r, int category) :
      Trace(reason)
   {
      for (const auto& field : r.GetFields()) {
         TracePtr t = r.GetStore().FetchStaticObject(field.second)->TraceObjectChanges(reason, category);
         t->OnChanged([=]() {
            NotifyObjectChanged();
         });
         field_traces_.push_back(t);
      }
   }
private:
   virtual void NotifyObjectChanged() = 0;

private:
   std::vector<TracePtr>   field_traces_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_H

