#ifndef _RADIANT_DM_SET_TRACE_SYNC_H
#define _RADIANT_DM_SET_TRACE_SYNC_H

#include "dm.h"
#include "set_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class SetTraceSync : public SetTrace<M>
{
public:
   SetTraceSync(const char* reason) :
      SetTrace(reason)
   {
   }

private:
   void OnAdded(Value const& value) override
   {
      SignalAdded(value);
   }

   void OnRemoved(Value const& value) override
   {
      SignalRemoved(value);
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_SYNC_H

