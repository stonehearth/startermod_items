#ifndef _RADIANT_DM_SET_TRACE_H
#define _RADIANT_DM_SET_TRACE_H

#include "trace_impl.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename SetType>
class SetTrace : public TraceImpl<SetTrace<SetType>>
{
public:
   typedef typename SetType::Value     Value;
   typedef std::function<void(Value const& v)> AddedCb;
   typedef std::function<void(Value const& v)> RemovedCb;
   typedef std::vector<Value> ValueList;
   typedef std::function<void(ValueList const& added, ValueList const& removed)> UpdatedCb;
   
public:
   SetTrace(const char* reason, SetType const& set);

   std::shared_ptr<SetTrace> OnAdded(AddedCb added);
   std::shared_ptr<SetTrace> OnRemoved(RemovedCb removed);
   std::shared_ptr<SetTrace> OnUpdated(UpdatedCb updated);

protected:
   ObjectId GetObjectId() const;
   Store const& GetStore() const;
   void SignalRemoved(Value const& value);
   void SignalAdded(Value const& value) ;
   void SignalUpdated(ValueList const& added, ValueList const& removed);
   void SignalObjectState() override;

protected:
   friend Store;
   virtual void NotifyRemoved(Value const& value) = 0;
   virtual void NotifyAdded(Value const& value) = 0;

private:
   SetType const& set_;
   RemovedCb      removed_;
   AddedCb        added_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_H

