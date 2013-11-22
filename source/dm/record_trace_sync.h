#ifndef _RADIANT_DM_RECORD_TRACE_SYNC_H
#define _RADIANT_DM_RECORD_TRACE_SYNC_H

#include "dm.h"
#include "destroy_trace.h"
#include "record_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class RecordTraceSync : public DestroyTrace,
                        public RecordTrace<M>                  
{
public:
   RecordTraceSync(const char* reason, Record const& r, Store const& s, int category) :
      RecordTrace(reason, r, s, category) { }

private:
   void NotifyRecordChanged() override
   {
      SignalModified();
   }

   void NotifyDestroyed() override
   {
      SignalDestroyed();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_SYNC_H

