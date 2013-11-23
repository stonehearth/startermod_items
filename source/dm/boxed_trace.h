#ifndef _RADIANT_DM_BOXED_TRACE_H
#define _RADIANT_DM_BOXED_TRACE_H

#include "dm.h"
#include "trace_impl.h"


BEGIN_RADIANT_DM_NAMESPACE

template <typename BoxedType>
class BoxedTrace : public TraceImpl<BoxedTrace<BoxedType>>
{
public:
   typedef typename BoxedType::Value   Value;
   typedef std::function<void(Value const& v)> ChangedCb;

public:
   BoxedTrace(const char* reason, Object const& o, Store const& store);
   std::shared_ptr<BoxedTrace> OnChanged(ChangedCb changed);
   std::shared_ptr<BoxedTrace> PushObjectState();

protected:
   void SignalChanged(Value const& value);

protected:
   friend Store;
   virtual void NotifyChanged(Value const& value) = 0;
   virtual void NotifyObjectState(Value const& value);

private:
   ChangedCb      changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_H

