#ifndef _RADIANT_DM_SET_TRACE_BUFFERED_H
#define _RADIANT_DM_SET_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "set_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class SetTraceBuffered : public TraceBuffered,
                         public SetTrace<M> {
public:
   void Flush()
   {
      SignalUpdated(added_, removed_);
      added_.clear();
      removed_.clear();
   }

private:
   void OnAdded(Value const& value) override
   {
      stdutil::FastRemove(removed_, value);
      added_.push_back(value);
   }

   void OnRemoved(Value const& value) override
   {
      stdutil::FastRemove(added_, value);
      removed_.push_back(value);
   }

private:
   ValueList  added_;
   ValueList  removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_BUFFERED_H

