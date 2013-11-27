#ifndef _RADIANT_DM_ALLOC_TRACE_H
#define _RADIANT_DM_ALLOC_TRACE_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class AllocTrace : public std::enable_shared_from_this<AllocTrace>
{
public:
   typedef std::function<void(ObjectPtr)> AllocedCb;
   typedef std::function<void(std::vector<ObjectPtr> const&)> UpdatedCb;

public:
   AllocTrace(Store const& store);

   std::shared_ptr<AllocTrace> OnAlloced(AllocedCb on_alloced);
   std::shared_ptr<AllocTrace> OnUpdated(UpdatedCb on_updated);
   std::shared_ptr<AllocTrace> PushObjectState();

protected:
   friend Store;
   friend TracerSync;
   friend TracerBuffered;

   void SignalAlloc(ObjectPtr obj);
   void SignalUpdated(std::vector<ObjectPtr> const& objs);

private:
   Store const& store_;
   AllocedCb  on_alloced_;
   UpdatedCb  on_updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ALLOC_TRACE_H

