#ifndef _RADIANT_DM_RECORD_TRACE_H
#define _RADIANT_DM_RECORD_TRACE_H

#include "dm.h"
#include "trace_impl.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class RecordTrace : public TraceImpl<RecordTrace<T>>
{
public:
   RecordTrace(const char* reason, Record const& r, Tracer& tracer);

public:
   virtual void NotifyRecordChanged() = 0;

private:
   void SignalObjectState() override;

protected:
   ObjectId GetObjectId() const;
   Record const& GetRecord() const;

private:
   Record const&           record_;
   std::vector<TracePtr>   field_traces_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_H

