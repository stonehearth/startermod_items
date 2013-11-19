#ifndef _RADIANT_DM_ARRAY_TRACE_H
#define _RADIANT_DM_ARRAY_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ArrayTrace : public Trace
{
public:
   typedef typename T::Value     Value;
   typedef std::unordered_map<uint, Value> ChangeMap;

   typedef std::function<void(uint i, Value const& v)> ChangedCb;
   typedef std::function<void(ChangeMap const& v)> UpdatedCb;
   
public:
   void OnChanged(ChangedCb changed)
   {
      changed_ = changed;
   }

   void OnUpdated(UpdatedCb updated)
   {
      updated_ = updated;
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
   virtual void OnChanged(uint i, Value const& value);

private:
   ChangedCb      changed_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ARRAY_TRACE_H

