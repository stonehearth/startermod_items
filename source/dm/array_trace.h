#ifndef _RADIANT_DM_ARRAY_TRACE_H
#define _RADIANT_DM_ARRAY_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ArrayTrace : public TraceImpl<ArrayTrace<T>>
{
public:
   typedef typename T::Value     Value;
   typedef std::unordered_map<uint, Value> ChangeMap;

   typedef std::function<void(uint i, Value const& v)> ChangedCb;
   typedef std::function<void(ChangeMap const& v)> UpdatedCb;
   
public:
   ArrayTrace(const char* reason, Object const& o, Store const& store) :
      TraceImpl(reason, o, store)
   {
   }

   std::shared_ptr<ArrayTrace> OnChanged(ChangedCb changed)
   {
      changed_ = changed;
      return shared_from_this();
   }

   std::shared_ptr<ArrayTrace> OnUpdated(UpdatedCb updated)
   {
      updated_ = updated;
      return shared_from_this();
   }

protected:
   void SignalChanged(uint i, Value const& value) 
   {
      if (changed) {
         changed_(i, value)
      } else if (updated_) {
         NOT_YET_IMPLEMENTED();
      }
   }

   void SignalUpdated(ChangeMap const& updated)
   {
      if (updated_) {
         updated_(updates);
      } else {
         for (const auto& entry : updated) {
            SignalChanged(entry.first, entry.second);
         }
      }
   }

private:
   friend Store;
   virtual void NotifySet(uint i, Value const& value);

private:
   ChangedCb      changed_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ARRAY_TRACE_H

