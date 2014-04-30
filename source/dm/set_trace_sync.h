#ifndef _RADIANT_DM_SET_TRACE_SYNC_H
#define _RADIANT_DM_SET_TRACE_SYNC_H

#include "dm.h"
#include "set_trace.h"
#include "core/object_counter.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename S>
class SetTraceSync : public SetTrace<S>,
                     public core::ObjectCounter<SetTraceSync<S>>
{
public:
   SetTraceSync(const char* reason, S const& set);

private:
   void NotifyAdded(Value const& value) override;
   void NotifyRemoved(Value const& value) override;
   void NotifyDestroyed() override;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_SYNC_H

