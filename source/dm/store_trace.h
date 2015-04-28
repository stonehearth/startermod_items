#ifndef _RADIANT_DM_STORE_TRACE_H
#define _RADIANT_DM_STORE_TRACE_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class StoreTrace : public std::enable_shared_from_this<StoreTrace>
{
public:
   typedef std::function<void(ObjectPtr const&)> AllocedCb;
   typedef std::function<void(Object const*)> RegisteredCb;
   typedef std::function<void(ObjectId)> ModifiedCb;
   typedef std::function<void(ObjectId, bool dynamic)> DestroyedCb;

public:
   StoreTrace(Store const& store);

   StoreTracePtr OnAlloced(AllocedCb const& on_alloced);
   StoreTracePtr OnRegistered(RegisteredCb const& on_registered);
   StoreTracePtr OnModified(ModifiedCb const& on_modified);
   StoreTracePtr OnDestroyed(DestroyedCb const& on_destroyed);
   StoreTracePtr PushStoreState();

protected:
   friend Store;
   void SignalAllocated(ObjectPtr const& obj);
   void SignalRegistered(Object const* obj);
   void SignalModified(ObjectId id);
   void SignalDestroyed(ObjectId id, bool dynamic);

private:
   Store const&   store_;
   AllocedCb      on_alloced_;
   RegisteredCb   on_registered_;
   ModifiedCb     on_modified_;
   DestroyedCb    on_destroyed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_STORE_TRACE_H

