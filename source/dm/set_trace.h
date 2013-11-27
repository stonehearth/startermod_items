#ifndef _RADIANT_DM_SET_TRACE_H
#define _RADIANT_DM_SET_TRACE_H

#include "trace_impl.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class SetTrace : public TraceImpl<SetTrace<T>>
{
public:
   typedef typename T::Value     Value;
   typedef std::function<void(Value const& v)> AddedCb;
   typedef std::function<void(Value const& v)> RemovedCb;
   typedef std::vector<Value> ValueList;
   typedef std::function<void(ValueList const& added, ValueList const& removed)> UpdatedCb;
   
public:
   SetTrace(const char* reason, Object const& o, Store const& store);

   std::shared_ptr<SetTrace> OnAdded(AddedCb added);
   std::shared_ptr<SetTrace> OnRemoved(RemovedCb removed);
   std::shared_ptr<SetTrace> OnUpdated(UpdatedCb updated);
   std::shared_ptr<SetTrace> PushObjectState();
   FORWARD_BASE_PUSH_OBJECT_STATE()

protected:
   void SignalRemoved(Value const& value);
   void SignalAdded(Value const& value) ;
   void SignalUpdated(ValueList const& added, ValueList const& removed);

protected:
   friend Store;
   virtual void NotifyRemoved(Value const& value) = 0;
   virtual void NotifyAdded(Value const& value) = 0;
   virtual void NotifyObjectState(typename T::ContainerType const& contents);

private:
   RemovedCb      removed_;
   AddedCb        added_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_H

