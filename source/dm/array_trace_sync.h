#ifndef _RADIANT_DM_ARRAY_TRACE_SYNC_H
#define _RADIANT_DM_ARRAY_TRACE_SYNC_H

#include "dm.h"
#include "array_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class ArrayTraceSync : public ArrayTrace<M>
{
public:
public:
   ArrayTraceSync(const char* reason, Object const& o, Store const& store) :
      ArrayTrace(reason, o, store)
   {
   }

private:
   void NotifySet(uint i, Value const& value) override
   {
      SignalChanged(i, value);
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ARRAY_TRACE_SYNC_H

