#ifndef _RADIANT_DM_ARRAY_TRACE_BUFFERED_H
#define _RADIANT_DM_ARRAY_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "array_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class ArrayTraceBuffered : public TraceBuffered,
                           public ArrayTrace<M> {
public:
   ArrayTraceBuffered(const char* reason, Object const& o, Store const& store);

   void Flush();
   void SaveObjectDelta(Object* obj, Protocol::Value* value) override;

private:
   void NotifySet(uint i, Value const& key) override;

private:
   ChangeMap       changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ARRAY_TRACE_BUFFERED_H

