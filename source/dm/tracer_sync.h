#ifndef _RADIANT_DM_TRACER_SYNC_H
#define _RADIANT_DM_TRACER_SYNC_H

#include "tracer.h"
#include "map_trace_sync.h"
#include "record_trace_sync.h"
#include "object_trace_sync.h"
#include "set_trace_sync.h"
#include "boxed_trace_sync.h"
#include "array_trace_sync.h"

BEGIN_RADIANT_DM_NAMESPACE

class TracerSync : public Tracer
{
public:
   virtual ~TracerSync();

   TracerType GetType() const override { return SYNC; }

#define DEFINE_TRACE_CLS_CHANGES(Cls, ctor_sig) \
   template <typename C> \
   std::shared_ptr<Cls ## Trace<C>> Trace ## Cls ## Changes(const char* reason, C const& object) \
   { \
      return std::make_shared<Cls ## TraceSync<C>> ctor_sig; \
   } \

   DEFINE_TRACE_CLS_CHANGES(Object, (reason))
   DEFINE_TRACE_CLS_CHANGES(Record, (reason, object, GetCategory()))
   DEFINE_TRACE_CLS_CHANGES(Map, (reason))
   DEFINE_TRACE_CLS_CHANGES(Boxed, (reason))
   DEFINE_TRACE_CLS_CHANGES(Set, (reason))
   DEFINE_TRACE_CLS_CHANGES(Array, (reason))

#undef DEFINE_TRACE_CLS_CHANGES


   void OnObjectChanged(ObjectId id) override
   {
      // Nothing to do.
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_SYNC_H

