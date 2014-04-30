#ifndef _RADIANT_DM_RECORD_TRACE_SYNC_H
#define _RADIANT_DM_RECORD_TRACE_SYNC_H

#include "dm.h"
#include "record_trace.h"
#include "core/object_counter.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename R>
class RecordTraceSync : public RecordTrace<R>,
                        public core::ObjectCounter<RecordTraceSync<R>>

{
public:
   RecordTraceSync(const char* reason, Record const& r, Tracer&);

private:
   void NotifyRecordChanged() override;
   void NotifyDestroyed() override;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_SYNC_H

