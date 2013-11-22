#ifndef _RADIANT_DM_SET_TRACE_BUFFERED_H
#define _RADIANT_DM_SET_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "set_trace.h"
#include "set_loader.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class SetTraceBuffered : public TraceBuffered,
                         public SetTrace<M> {
public:
   SetTraceBuffered(const char* reason, Object const& o, Store const& store) :
      SetTrace(reason, o, store)
   {
   }

   void Flush()
   {
      SignalUpdated(added_, removed_);
      added_.clear();
      removed_.clear();
   }

   void SaveObjectDelta(Object* obj, Protocol::Value* value) override
   {
      SaveObjectDelta(added_, removed_ value);
   }


private:
   void NotifyAdded(Value const& value) override
   {
      stdutil::FastRemove(removed_, value);
      added_.push_back(value);
   }

   void NotifyRemoved(Value const& value) override
   {
      stdutil::FastRemove(added_, value);
      removed_.push_back(value);
   }

   void NotifyObjectState(typename M::ContainerType const& contents) override
   {
      added_.clear();
      removed_.clear();
      SetTrace<M>::PushObjectState(contents);
   }

private:
   ValueList   added_;
   ValueList   removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_BUFFERED_H

