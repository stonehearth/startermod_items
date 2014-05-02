#ifndef _RADIANT_DM_RECORD_TRACE_BUFFERED_H
#define _RADIANT_DM_RECORD_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "record_trace.h"
#include "core/object_counter.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename R>
class RecordTraceBuffered : public core::ObjectCounter<RecordTraceBuffered<R>>,
                            public RecordTrace<R>,
                            public TraceBuffered
                            
{
public:
   RecordTraceBuffered(const char* reason, Record const& r, Tracer&);

   void Flush() override;
   bool SaveObjectDelta(SerializationType r, Protocol::Value* value) override;

private:
   void NotifyRecordChanged() override;

private:
   bool        changed_;
   bool        first_save_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_BUFFERED_H

