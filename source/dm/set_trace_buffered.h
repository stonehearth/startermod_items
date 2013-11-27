#ifndef _RADIANT_DM_SET_TRACE_BUFFERED_H
#define _RADIANT_DM_SET_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "set_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename S>
class SetTraceBuffered : public TraceBuffered,
                         public SetTrace<S> {
public:
   SetTraceBuffered(const char* reason, S const& set);

   void Flush();
   void SaveObjectDelta(Protocol::Value* value) override;

private:
   void ClearCachedState() override;
   void NotifyAdded(Value const& value) override;
   void NotifyRemoved(Value const& value) override;

private:
   ValueList      added_;
   ValueList      removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_BUFFERED_H

