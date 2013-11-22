#include "radiant.h"
#include "alloc_trace.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

AllocTrace::AllocTrace(Store const& store) : 
   store_(store)
{
}

std::shared_ptr<AllocTrace> AllocTrace::OnAlloced(AllocedCb on_alloced)
{
   on_alloced_ = on_alloced;
   return shared_from_this();
}

std::shared_ptr<AllocTrace> AllocTrace::OnUpdated(UpdatedCb on_updated)
{
   on_updated_ = on_updated;
   return shared_from_this();
}

std::shared_ptr<AllocTrace> AllocTrace::PushObjectState()
{
   store_.PushAllocState(*this);
   return shared_from_this();
}

void AllocTrace::SignalAlloc(ObjectPtr obj)
{
   if (on_alloced_) {
      on_alloced_(obj);
   } else if (on_updated_) {
      std::vector<ObjectPtr> objs;
      objs.push_back(obj);
      on_updated_(objs);
   }
}

void AllocTrace::SignalUpdated(std::vector<ObjectPtr> const& objs)
{
   if (on_updated_) {
      on_updated_(objs);
   } else if (on_alloced_) {
      for (ObjectRef o : objs) {
         on_alloced_(o);
      }
   }
}
