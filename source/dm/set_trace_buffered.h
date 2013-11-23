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
   SetTraceBuffered(const char* reason, Object const& o, Store const& store);

   void Flush();
   void SaveObjectDelta(Object* obj, Protocol::Value* value) override;

private:
   void NotifyAdded(Value const& value) override;
   void NotifyRemoved(Value const& value) override;
   void NotifyObjectState(typename M::ContainerType const& contents) override;

private:
   ValueList   added_;
   ValueList   removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_TRACE_BUFFERED_H

