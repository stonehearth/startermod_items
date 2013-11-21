#ifndef _RADIANT_DM_RECORD_TRACE_BUFFERED_H
#define _RADIANT_DM_RECORD_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "record_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class RecordTraceBuffered : public RecordTrace<M>,
                            public TraceBuffered
{
public:
   RecordTraceBuffered(const char* reason, Record const& r, Store const& s, int category) :
      RecordTrace(reason, r, s, category)
   {
   }

   void Flush() override
   {
      ASSERT(changed_);
      if (changed_) {
         SignalModified();
      }
   }

private:
   void NotifyRecordChanged() override
   {
      changed_ = true;
   }

private:
   bool        changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_BUFFERED_H

