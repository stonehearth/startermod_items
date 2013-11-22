#ifndef _RADIANT_DM_SET_TRACE_H
#define _RADIANT_DM_SET_TRACE_H

#include "dm.h"
#include "trace.h"

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
   SetTrace(const char* reason, Object const& o, Store const& store) :
      TraceImpl(reason, o, store)
   {
   }

   std::shared_ptr<SetTrace> OnAdded(AddedCb added)
   {
      added_ = added;
      return shared_from_this();
   }

   std::shared_ptr<SetTrace> OnRemoved(RemovedCb removed)
   {
      removed_ = removed;
      return shared_from_this();
   }

   std::shared_ptr<SetTrace> OnUpdated(UpdatedCb updated)
   {
      updated_ = updated;
      return shared_from_this();
   }

   std::shared_ptr<SetTrace> PushObjectState()
   {
      GetStore().PushSetState<T>(*this, GetObjectId());
      return shared_from_this();
   }

protected:
   void SignalRemoved(Value const& value)
   {
      SignalModified();
      if (removed_) {
         removed_(value);
      } else if (updated_) {
         NOT_YET_IMPLEMENTED();
      }
   }

   void SignalAdded(Value const& value) 
   {
      SignalModified();
      if (added_) {
         added_(value);
      } else if (updated_) {
         NOT_YET_IMPLEMENTED();
      }
   }

   void SignalUpdated(ValueList const& added, ValueList const& removed)
   {
      SignalModified();
      if (updated_) {
         updated_(added, removed);
      } else {
         for (const auto& value : added) {
            SignalAdded(value);
         }

         for (const auto& value : removed) {
            SignalRemoved(value);
         }
      }
   }

private:
   friend Store;
   virtual void NotifyRemoved(Value const& value);
   virtual void NotifyAdded(Value const& value);
   void NotifyObjectState(typename T::ContainerType const& contents)
   {
      SignalUpdated(contents, ValueList());
   }

private:
   RemovedCb      removed_;
   AddedCb        added_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_H

