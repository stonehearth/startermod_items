#ifndef _RADIANT_DM_OBJECT_TRACE_H
#define _RADIANT_DM_OBJECT_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ObjectTrace : public TraceImpl<ObjectTrace<T>>
{
public:
   ObjectTrace(const char* reason, Object const& o, Store const& store) :
      TraceImpl(reason, o, store)
   {
   }

   std::shared_ptr<ObjectTrace> PushObjectState()
   {
      GetStore().PushObjectState<T>(*this, GetObjectId());
      return shared_from_this();
   }

private:
   friend Store;
   virtual void NotifyObjectState()
   {
      SignalModified();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_TRACE_H

