#ifndef _RADIANT_DM_RECORD_TRACE_SYNC_H
#define _RADIANT_DM_RECORD_TRACE_SYNC_H

#include "dm.h"
#include "record_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class RecordTraceSync : public RecordTrace<M>                  
{
public:
   RecordTraceSync(const char* reason, Record const& r, Store& s, Tracer* tracer);

private:
   void NotifyRecordChanged() override;
   void NotifyDestroyed() override;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_SYNC_H

