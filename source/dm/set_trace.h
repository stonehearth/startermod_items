#ifndef _RADIANT_DM_SET_TRACE_H
#define _RADIANT_DM_SET_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class SetTrace : public Trace
{
public:
   typedef typename T::Value     Value;
   typedef std::function<void(Value const& v)> AddedCb;
   typedef std::function<void(Value const& v)> RemovedCb;
   typedef std::vector<Value> ValueList;
   typedef std::function<void(ValueList const& added, ValueList const& removed)> UpdatedCb;
   
public:
   void OnAdded(AddedCb added)
   {
      added_ = added;
   }

   void OnRemoved(RemovedCb removed)
   {
      removed_ = removed;
   }

   void OnUpdated(UpdatedCb updated)
   {
      updated_ = updated;
   }

protected:
   void SignalRemoved(Value const& value)
   {
      if (removed_) {
         removed_(value);
      } else if (updated_) {
         NOT_YET_IMPLEMENTED();
      }
   }

   void SignalAdded(Value const& value) 
   {
      if (added) {
         added_(value)
      } else if (updated_) {
         NOT_YET_IMPLEMENTED();
      }
   }

   void SignalUpdated(ValueList const& added, ValueList const& removed)
   {
      if (updated_) {
         update_(added, removed);
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

private:
   RemovedCb      removed_;
   AddedCb        added_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_H

