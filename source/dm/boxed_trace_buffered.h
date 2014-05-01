#ifndef _RADIANT_DM_BOXED_TRACE_BUFFERED_H
#define _RADIANT_DM_BOXED_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "boxed_trace.h"
#include "core/object_counter.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename BoxedType>
class BoxedTraceBuffered : public core::ObjectCounter<BoxedTraceBuffered<BoxedType>>,
                           public TraceBuffered,
                           public BoxedTrace<BoxedType>
                           
{
public:
   BoxedTraceBuffered(const char* reason, BoxedType const& b);
   
   void Flush();
   bool SaveObjectDelta(SerializationType r, Protocol::Value* value) override;

private:
   void ClearCachedState() override;
   void NotifyChanged(Value const& value) override;

private:
   Value const*      value_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_BUFFERED_H

