#ifndef _RADIANT_DM_BOXED_TRACE_H
#define _RADIANT_DM_BOXED_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class BoxedTrace : public TraceImpl<BoxedTrace<T>>
{
public:
   typedef typename T   Value;
   typedef std::function<void(Value const& v)> ChangedCb;

public:
   BoxedTrace(const char* reason, Object const& o, Store const& store) :
      TraceImpl(reason, o, store)
   {
   }

   std::shared_ptr<BoxedTrace> OnChanged(ChangedCb changed)
   {
      changed_ = changed;
      return std::static_pointer_cast<BoxedTrace>(shared_from_this());
   }

   std::shared_ptr<BoxedTrace> PushObjectState()
   {
      GetStore().PushBoxedState<T>(*this, GetObjectId());
      return shared_from_this();
   }

protected:
   void SignalChanged(Value const& value)
   {
      if (changed_) {
         changed_(value);
      }
   }

private:
   friend Store;
   virtual void NotifyChanged(Value const& value);
   virtual void NotifyObjectState(Value const& value)
   {
      SignalChanged(value);
   }

private:
   ChangedCb      changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_H

