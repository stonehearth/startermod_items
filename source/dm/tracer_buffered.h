#ifndef _RADIANT_DM_TRACER_BUFFERED_H
#define _RADIANT_DM_TRACER_BUFFERED_H

#include "radiant_stdutil.h"
#include "tracer.h"
#include "map_trace_buffered.h"
#include "record_trace_buffered.h"
#include "set_trace_buffered.h"
#include "boxed_trace_buffered.h"

BEGIN_RADIANT_DM_NAMESPACE

class TracerBuffered : public Tracer
{
public:
   TracerBuffered(std::string const&, Store& store);
   virtual ~TracerBuffered();

   TracerType GetType() const { return BUFFERED; }
   
#define DEFINE_TRACE_CLS_CHANGES(Cls, ctor_sig) \
   template <typename C> \
   std::shared_ptr<Cls ## Trace<C>> Trace ## Cls ## Changes(const char* reason, Store& store, C const& object) \
   { \
      ObjectId id = object.GetObjectId(); \
      std::shared_ptr<Cls ## TraceBuffered<C>> trace = std::make_shared<Cls ## TraceBuffered<C>> ctor_sig; \
      traces_[id].push_back(trace); \
      buffered_traces_[id].push_back(trace); \
      return trace; \
   }

   DEFINE_TRACE_CLS_CHANGES(Record, (reason, object, *this))
   DEFINE_TRACE_CLS_CHANGES(Map, (reason, object))
   DEFINE_TRACE_CLS_CHANGES(Boxed, (reason, object))
   DEFINE_TRACE_CLS_CHANGES(Set, (reason, object))

#undef DEFINE_TRACE_CLS_CHANGES

   void Flush();

private:
   void FlushOnce(std::vector<ObjectId>& last_modified,
                  std::vector<ObjectId>& last_destroyed);
   void OnObjectDestroyed(ObjectId id);
   void OnObjectModified(ObjectId id);

private:
   typedef std::vector<TraceBufferedRef> TraceBufferedList;
   typedef std::unordered_map<ObjectId, TraceBufferedList> TraceBufferedMap;

   typedef std::vector<TraceRef> TraceList;
   typedef std::unordered_map<ObjectId, TraceList> TraceMap;

private:
   std::vector<ObjectId>   modified_objects_;
   std::vector<ObjectId>   destroyed_objects_;
   StoreTracePtr           store_trace_;
   TraceMap                traces_;
   TraceBufferedMap        buffered_traces_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_BUFFERED_H

