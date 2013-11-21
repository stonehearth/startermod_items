#ifndef _RADIANT_DM_RECORD_TRACE_SYNC_H
#define _RADIANT_DM_RECORD_TRACE_SYNC_H

#include "dm.h"
#include "trace_sync.h"
#include "record_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class RecordTraceSync : public RecordTrace<M>,
                        public TraceSync                        
{
public:
   RecordTraceSync(const char* reason, Record const& r, Store const& s, int category) :
      RecordTrace(reason, r, s, category) { }

private:
   void NotifyRecordChanged() override
   {
      SignalModified();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_SYNC_H

