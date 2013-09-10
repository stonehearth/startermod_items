#ifndef _RADIANT_CLIENT_TRACE_OBJECT_DEFERRED_H
#define _RADIANT_CLIENT_TRACE_OBJECT_DEFERRED_H

#include "namespace.h"
#include "core/deferred.h"
#include "dm/object.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class TraceObjectDeferred : public core::Deferred1<dm::ObjectRef>
{
public:
   TraceObjectDeferred(dm::ObjectPtr obj, const char* reason);
   virtual ~TraceObjectDeferred();

   void Reset(dm::ObjectPtr obj);

private:
   std::string    reason_;
   dm::Guard      guard_;
};

typedef std::shared_ptr<TraceObjectDeferred> TraceObjectDeferredPtr;
typedef std::weak_ptr<TraceObjectDeferred> TraceObjectDeferredRef;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_TRACE_OBJECT_DEFERRED_H
