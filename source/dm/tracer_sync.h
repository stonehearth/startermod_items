#ifndef _RADIANT_DM_TRACER_SYNC_H
#define _RADIANT_DM_TRACER_SYNC_H

#include "tracer.h"
#include "map_trace_sync.h"
#include "record_trace_sync.h"
#include "set_trace_sync.h"
#include "boxed_trace_sync.h"

BEGIN_RADIANT_DM_NAMESPACE

class TracerSync : public Tracer
{
public:
   TracerSync(std::string const&);
   virtual ~TracerSync();

private:
   friend Store;

   TracerType GetType() const override { return SYNC; }

#define DEFINE_TRACE_CLS_CHANGES(Cls, ctor_sig) \
   template <typename C> \
   std::shared_ptr<Cls ## Trace<C>> Trace ## Cls ## Changes(const char* reason, Store& store, C const& object) \
   { \
      return std::make_shared<Cls ## TraceSync<C>> ctor_sig; \
   } \

   DEFINE_TRACE_CLS_CHANGES(Record, (reason, object, *this))
   DEFINE_TRACE_CLS_CHANGES(Map, (reason, object))
   DEFINE_TRACE_CLS_CHANGES(Boxed, (reason, object))
   DEFINE_TRACE_CLS_CHANGES(Set, (reason, object))

#undef DEFINE_TRACE_CLS_CHANGES

   void OnObjectAlloced(ObjectPtr obj) override;
   void OnObjectDestroyed(ObjectId id) override;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_SYNC_H

