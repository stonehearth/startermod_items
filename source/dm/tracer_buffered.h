#ifndef _RADIANT_DM_TRACER_BUFFERED_H
#define _RADIANT_DM_TRACER_BUFFERED_H

#include "radiant_stdutil.h"
#include "tracer.h"
#include "map_trace_buffered.h"

BEGIN_RADIANT_DM_NAMESPACE

class TracerBuffered : public Tracer
{
public:
   virtual ~TracerBuffered();

   TracerType GetType() const
   {
      return TRACER_BUFFERED;
   }

   template <typename C>
   std::shared_ptr<MapTrace<C>> TraceMapChanges(const char* reason, C const& map)
   {
      std::shared_ptr<MapTraceBuffered> trace = std::make_shared<MapTraceBuffered<M>>(reason);
      traces_[id].push_back(trace);
      return trace;
   }

   void OnObjectChanged(ObjectId id) override;
   void Flush();

private:
   typedef std::vector<TraceBufferedRef> TraceList;
   typedef std::unordered_map<ObjectId, TraceList> TraceMap;

private:
   std::vector<ObjectId>   modified_objects_;
   TraceMap                traces_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_BUFFERED_H

