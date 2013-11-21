#ifndef _RADIANT_DM_BOXED_TRACE_BUFFERED_H
#define _RADIANT_DM_BOXED_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "boxed_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class BoxedTraceBuffered : public TraceBuffered,
                           public BoxedTrace<M> {
public:
   BoxedTraceBuffered(const char* reason) :
      TraceBuffered(reason),
      value_(nullptr)
   {
   }

   void Flush()
   {
      ASSERT(value_);
      if (value_) {
         // xxx: this might be an old value since it points back into the box (ug!)
         SignalChanged(*value_);
         value_ = nullptr;
      }
   }

private:
   void NotifyChanged(Value const& value) override
   {
      value_ = &value;
   }

   void NotifyObjectState(Value const& value) override
   {
      value_ = nullptr;
      BoxedTrace<M>::PushObjectState(value);
   }

private:
   Value const*      value_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_BUFFERED_H

