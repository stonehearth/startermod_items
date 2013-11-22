#include "radiant.h"
#include "radiant_stdutil.h"
#include "tracer_buffered.h"
#include "alloc_trace.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

TracerBuffered::~TracerBuffered()
{
}

void TracerBuffered::OnObjectChanged(ObjectId id)
{
   stdutil::UniqueInsert(modified_objects_, id);
}

void TracerBuffered::OnObjectAlloced(ObjectPtr obj)
{
   alloced_objects_.push_back(obj);
}

void TracerBuffered::OnObjectDestroyed(ObjectId id)
{
   destroyed_objects_.push_back(id);
}

void TracerBuffered::Flush()
{
   std::vector<ObjectPtr> alloced;
   stdutil::ForEachPrune<Object>(alloced_objects_, [&alloced](ObjectPtr obj) {
      alloced.push_back(obj);
   });
   alloced_objects_.clear();

   stdutil::ForEachPrune<AllocTrace>(alloc_traces_, [&alloced](AllocTracePtr trace) {
      trace->SignalUpdated(alloced);
   });

   for (ObjectId id : modified_objects_) {
      auto i = buffered_traces_.find(id);
      if (i != buffered_traces_.end()) {
         stdutil::ForEachPrune<TraceBuffered>(i->second, [](TraceBufferedPtr t) {
            t->Flush();
         });
         if (i->second.empty()) {
            buffered_traces_.erase(i);
         }
      }
   }

   for (ObjectId id : destroyed_objects_) {
      auto i = traces_.find(id);
      if (i != traces_.end()) {
         stdutil::ForEachPrune<Trace>(i->second, [](TracePtr t) {
            t->SignalDestroyed();
         });
         traces_.erase(i);
      }
   };
   destroyed_objects_.clear();
}

