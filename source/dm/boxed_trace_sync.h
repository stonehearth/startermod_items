#ifndef _RADIANT_DM_BOXED_TRACE_SYNC_H
#define _RADIANT_DM_BOXED_TRACE_SYNC_H

#include "dm.h"
#include "boxed_trace.h"
#include "core/object_counter.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename BoxedType>
class BoxedTraceSync : public BoxedTrace<BoxedType>,
                       public core::ObjectCounter<BoxedTraceSync<BoxedType>>
{
public:
public:
   BoxedTraceSync(const char* reason, BoxedType const& b);

private:
   void NotifyChanged(Value const& value) override;
   void NotifyDestroyed() override;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_SYNC_H

