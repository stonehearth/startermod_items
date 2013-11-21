#ifndef _RADIANT_DM_TRACER_BUFFERED_H
#define _RADIANT_DM_TRACER_BUFFERED_H

#include "radiant_stdutil.h"
#include "tracer.h"
#include "map_trace_buffered.h"
#include "record_trace_buffered.h"
#include "object_trace_buffered.h"
#include "set_trace_buffered.h"
#include "boxed_trace_buffered.h"
#include "array_trace_buffered.h"
#include "alloc_trace_buffered.h"

BEGIN_RADIANT_DM_NAMESPACE

class TracerBuffered : public Tracer
{
public:
   TracerBuffered(int category) : Tracer(category) {}
   virtual ~TracerBuffered();

   TracerType GetType() const { return BUFFERED; }
   
#define DEFINE_TRACE_CLS_CHANGES(Cls, ctor_sig) \
   template <typename C> \
   std::shared_ptr<Cls ## Trace<C>> Trace ## Cls ## Changes(const char* reason, Store const& store, C const& object) \
   { \
      ObjectId id = object.GetObjectId(); \
      std::shared_ptr<Cls ## TraceBuffered<C>> trace = std::make_shared<Cls ## TraceBuffered<C>> ctor_sig; \
      traces_[id].push_back(trace); \
      return trace; \
   }

   DEFINE_TRACE_CLS_CHANGES(Object, (reason, object, store))
   DEFINE_TRACE_CLS_CHANGES(Record, (reason, object, store, GetCategory()))
   DEFINE_TRACE_CLS_CHANGES(Map, (reason, object, store))
   DEFINE_TRACE_CLS_CHANGES(Boxed, (reason, object, store))
   DEFINE_TRACE_CLS_CHANGES(Set, (reason, object, store))
   DEFINE_TRACE_CLS_CHANGES(Array, (reason, object, store))

#undef DEFINE_TRACE_CLS_CHANGES

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

