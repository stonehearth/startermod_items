#ifndef _RADIANT_DM_ARRAY_TRACE_BUFFERED_H
#define _RADIANT_DM_ARRAY_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "array_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class ArrayTraceBuffered : public TraceBuffered,
                           public ArrayTrace<M> {
public:
   void Flush()
   {
      SignalUpdated(changed_);
      changed_.clear();
   }

private:
   void OnChanged(uint i, Key const& key) override
   {
      changed_[i] = key;
   }

private:
   ChangeMap       changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ARRAY_TRACE_BUFFERED_H

