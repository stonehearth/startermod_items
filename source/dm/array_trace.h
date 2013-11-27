#ifndef _RADIANT_DM_ARRAY_TRACE_H
#define _RADIANT_DM_ARRAY_TRACE_H

#include "dm.h"
#include "trace_impl.h"
#include <unordered_map>

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
   ArrayTrace(const char* reason, Object const& o, Store const& store);

   std::shared_ptr<ArrayTrace> OnAdded(ChangedCb changed);
   std::shared_ptr<ArrayTrace> OnUpdated(UpdatedCb updated);

protected:
   void SignalChanged(uint i, Value const& value);
   void SignalUpdated(ChangeMap const& updated);

private:
   friend Store;
   virtual void NotifySet(uint i, Value const& value) = 0;

private:
   ChangedCb      changed_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ARRAY_TRACE_H

