#ifndef _RADIANT_DM_BOXED_TRACE_BUFFERED_H
#define _RADIANT_DM_BOXED_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "boxed_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class BoxedTraceBuffered : public TraceBuffered,
                           public BoxedTrace<M> {
public:
   BoxedTraceBuffered(const char* reason, Object const& o, Store const& store);
   
   void Flush();
   void SaveObjectDelta(Object* obj, Protocol::Value* value) override;

private:
   void NotifyChanged(Value const& value) override;
   void NotifyObjectState(Value const& value) override;

private:
   Value const*      value_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_BUFFERED_H

