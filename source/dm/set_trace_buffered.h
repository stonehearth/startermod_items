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
   void OnAdded(Key const& key, Key const& value) override
   {
      stdutil::FastRemove(removed_, key);
      added_.push_back(std::make_pair(key, value));
   }

   void OnRemoved(Key const& key) override
   {
      stdutil::FastRemove(added_, key);
      removed_.push_back(key);
   }

private:
   std::vector<T>  added_;
   std::vector<T>  removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_BUFFERED_H

