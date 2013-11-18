#ifndef _RADIANT_DM_OBJECT_TRACE_SYNC_H
#define _RADIANT_DM_OBJECT_TRACE_SYNC_H

#include "dm.h"
#include "trace_sync.h"
#include "object_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ObjectTraceSync : public ObjectTrace<T>,
                        public TraceSync
{
public:
   ObjectTraceSync(const char* reason) : ObjectTrace(reason) { }

private:
   void NotifyObjectChanged()
   {
      SignalChanged();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_TRACE_SYNC_H

