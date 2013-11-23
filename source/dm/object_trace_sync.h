#ifndef _RADIANT_DM_OBJECT_TRACE_SYNC_H
#define _RADIANT_DM_OBJECT_TRACE_SYNC_H

#include "dm.h"
#include "object_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ObjectTraceSync : public ObjectTrace<T>
{
public:
   ObjectTraceSync(const char* reason, Object const& o, Store const& store) :
      ObjectTrace(reason, o, store)
   {
   }

   void NotifyDestroyed() override
   {
      SignalDestroyed();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_TRACE_SYNC_H

