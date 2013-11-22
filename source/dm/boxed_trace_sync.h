#ifndef _RADIANT_DM_BOXED_TRACE_SYNC_H
#define _RADIANT_DM_BOXED_TRACE_SYNC_H

#include "dm.h"
#include "destroy_trace.h"
#include "boxed_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class BoxedTraceSync : public DestroyTrace,
                       public BoxedTrace<M>
{
public:
public:
   BoxedTraceSync(const char* reason, Object const& o, Store const& store) :
      BoxedTrace(reason, o, store)
   {
   }

private:
   void NotifyChanged(Value const& value) override
   {
      SignalChanged(value);
   }

   void NotifyDestroyed() override
   {
      SignalDestroyed();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_SYNC_H

