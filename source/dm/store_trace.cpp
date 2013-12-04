#include "radiant.h"
#include "store_trace.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

StoreTrace::StoreTrace(Store const& store) : 
   store_(store)
{
}

StoreTracePtr StoreTrace::OnAlloced(AllocedCb on_alloced)
{
   on_alloced_ = on_alloced;
   return shared_from_this();
}

StoreTracePtr StoreTrace::OnRegistered(RegisteredCb on_registered)
{
   on_registered_ = on_registered;
   return shared_from_this();
}

StoreTracePtr StoreTrace::OnModified(ModifiedCb on_modified)
{
   on_modified_ = on_modified;
   return shared_from_this();
}

StoreTracePtr StoreTrace::OnDestroyed(DestroyedCb on_destroyed)
{
   on_destroyed_ = on_destroyed;
   return shared_from_this();
}

StoreTracePtr StoreTrace::PushStoreState()
{
   store_.PushStoreState(*this);
   return shared_from_this();
}

void StoreTrace::SignalAllocated(ObjectPtr obj)
{
   if (on_alloced_) {
      on_alloced_(obj);
   }
}

void StoreTrace::SignalRegistered(Object const* obj)
{
   if (on_registered_) {
      on_registered_(obj);
   }
}

void StoreTrace::SignalModified(ObjectId id)
{
   if (on_modified_) {
      on_modified_(id);
   }
}

void StoreTrace::SignalDestroyed(ObjectId id, bool dynamic)
{
   if (on_destroyed_) {
      on_destroyed_(id, dynamic);
   }
}
