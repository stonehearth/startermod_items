#ifndef _RADIANT_DM_OBJECT_TRACE_H
#define _RADIANT_DM_OBJECT_TRACE_H

#include "dm.h"
#include "trace_impl.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ObjectTrace : public TraceImpl<ObjectTrace<T>>
{
public:
   ObjectTrace(const char* reason, Object const& o, Store const& store);
   std::shared_ptr<ObjectTrace> PushObjectState();

private:
   friend Store;
   virtual void NotifyObjectState();
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_TRACE_H

