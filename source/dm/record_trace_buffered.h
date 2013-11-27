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
   RecordTraceBuffered(const char* reason, Record const& r, Tracer&);

   void Flush() override;
   void SaveObjectDelta(Protocol::Value* value) override;

private:
   void NotifyRecordChanged() override;

private:
   bool        changed_;
   bool        first_save_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_BUFFERED_H

