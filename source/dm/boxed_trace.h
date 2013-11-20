#ifndef _RADIANT_DM_BOXED_TRACE_H
#define _RADIANT_DM_BOXED_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class BoxedTrace : public Trace
{
public:
   typedef typename T   Value;
   typedef std::function<void(Value const& v)> ChangedCb;

public:
   void OnChanged(ChangedCb changed)
   {
      changed_ = changed;
   }

protected:
   void SignalChanged(Value const& value) {
      if (changed) {
         changed_(value)
      }
   }

private:
   friend Store;
   virtual void NotifyChanged(Value const& value);

private:
   ChangedCb      changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_H

