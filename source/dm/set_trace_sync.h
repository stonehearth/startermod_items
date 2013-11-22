#ifndef _RADIANT_DM_SET_TRACE_SYNC_H
#define _RADIANT_DM_SET_TRACE_SYNC_H

#include "dm.h"
#include "destroy_trace.h"
#include "set_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class SetTraceSync : public DestroyTrace,
                     public SetTrace<M>
{
public:
   SetTraceSync(const char* reason, Object const& o, Store const& store) :
      SetTrace(reason, o, store)
   {
   }

private:
   void NotifyAdded(Value const& value) override
   {
      SignalAdded(value);
   }

   void NotifyRemoved(Value const& value) override
   {
      SignalRemoved(value);
   }

   void NotifyDestroyed() override
   {
      SignalDestroyed();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_SYNC_H

