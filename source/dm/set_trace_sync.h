#ifndef _RADIANT_DM_SET_TRACE_SYNC_H
#define _RADIANT_DM_SET_TRACE_SYNC_H

#include "dm.h"
#include "set_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class SetTraceSync : public SetTrace<M>
{
public:
   SetTraceSync(const char* reason, Object const& o, Store const& store);

private:
   void NotifyAdded(Value const& value) override;
   void NotifyRemoved(Value const& value) override;
   void NotifyDestroyed() override;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_SYNC_H

