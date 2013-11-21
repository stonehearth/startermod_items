#ifndef _RADIANT_DM_ALLOC_TRACE_H
#define _RADIANT_DM_ALLOC_TRACE_H

#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

class Trace
{
public:
   virtual ~Trace();
};

class AllocTrace : public Trace,
                   public std::enable_shared_from_this<AllocTrace>
{
public:
   typedef std::function<void(ObjectRef)> AllocedCb;
   typedef std::function<void(std::vector<ObjectPtr>)> UpdatedCb;

public:
   AllocTrace(Store const& store) : 
      store_(store)
   {
   }

   std::shared_ptr<AllocTrace> OnAlloc(AllocedCb on_alloced)
   {
      on_alloced_ = on_alloced;
      return shared_from_this();
   }

   std::shared_ptr<AllocTrace> OnUpdated(UpdatedCb on_updated)
   {
      on_updated_ = on_updated;
      return shared_from_this();
   }

   std::shared_ptr<AllocTrace> PushObjectState()
   {
      store_.PushAllocState(*this);
      return shared_from_this();
   }

protected:
   friend Store;

   void SignalAlloc(ObjectPtr obj)
   {
      if (on_alloced_) {
         on_alloced_(obj);
      } else if (on_updated_) {
         std::vector<ObjectPtr> objs;
         objs.push_back(obj);
         on_updated_(objs);
      }
   }

   void NotifyAllocState(std::vector<ObjectPtr> const& objs)
   {
      if (on_updated_) {
         on_updated_(objs);
      } else if (on_alloced_) {
         for (ObjectRef o : objs) {
            on_alloced_(o);
         }
      }
   }

protected:
   virtual void NotifyAlloc(ObjectPtr obj) = 0;

private:
   Store const& store_;
   AllocedCb  on_alloced_;
   UpdatedCb  on_updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ALLOC_TRACE_H

