#ifndef _RADIANT_DM_TRACER_SYNC_H
#define _RADIANT_DM_TRACER_SYNC_H

#include "tracer.h"
#include "map_trace_sync.h"

BEGIN_RADIANT_DM_NAMESPACE

class TracerSync : public Tracer
{
public:
   virtual ~TracerSync();

   TracerType GetType() const override
   {
      return TRACER_SYNC;
   }

   template <typename C>
   std::shared_ptr<MapTrace<C>> TraceMapChanges(const char* reason, C const& map)
   {
      return std::make_shared<MapTraceSync<M>>(reason);
   }

   void OnObjectChanged(ObjectId id) override
   {
      // Nothing to do.
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_SYNC_H

