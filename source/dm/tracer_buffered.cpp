#include "radiant.h"
#include "radiant_stdutil.h"
#include "tracer_buffered.h"
#include "alloc_trace.h"
#include "object.h"
#include "object_macros.h"
#include "dm_log.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

#define TB_LOG(level)  DM_LOG("tracer buffered " << GetName(), 5)

TracerBuffered::TracerBuffered(std::string const& name) :
   Tracer(name)
{
}

TracerBuffered::~TracerBuffered()
{
}

void TracerBuffered::NotifyChanged(ObjectId id)
{
   TB_LOG(5) << "adding object " << id << " to modified set";
   stdutil::UniqueInsert(modified_objects_, id);
}

void TracerBuffered::OnObjectAlloced(ObjectPtr obj)
{
   TB_LOG(5) << "adding object " << obj->GetObjectId() << " to alloced set";
   alloced_objects_.push_back(obj);
}

void TracerBuffered::OnObjectDestroyed(ObjectId id)
{
   TB_LOG(5) << "adding object " << id << " to destroyed set";
   destroyed_objects_.push_back(id);
}

void TracerBuffered::Flush()
{
   TB_LOG(5) << "starting flush...";

   bool finished;
   std::vector<ObjectRef> alloced;
   std::vector<ObjectId> modified;
   std::vector<ObjectId> destroyed;

   do {
      alloced = std::move(alloced_objects_);
      modified = std::move(modified_objects_);
      destroyed = std::move(destroyed_objects_);

      TB_LOG(5) << "flushing -> ("
                << "alloced: " << alloced.size() << " "
                << "modified: " << modified.size() << " "
                << "destroyed: " << destroyed.size() << ")";     
      FlushOnce(alloced, modified, destroyed);

      finished = alloced_objects_.empty() && modified_objects_.empty() && destroyed_objects_.empty();
   } while (!finished);

   TB_LOG(5) << "finished flush";
}

void TracerBuffered::FlushOnce(std::vector<ObjectRef>& last_alloced,
                               std::vector<ObjectId>& last_modified,
                               std::vector<ObjectId>& last_destroyed)
{
   std::vector<ObjectPtr> alloced;
   stdutil::ForEachPrune<Object>(last_alloced, [&alloced](ObjectPtr obj) {
      alloced.push_back(obj);
   });
   if (!alloced.empty()) {
      stdutil::ForEachPrune<AllocTrace>(alloc_traces_, [&alloced](AllocTracePtr trace) {
         trace->SignalUpdated(alloced);
      });
   }

   for (ObjectId id : last_modified) {
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

   for (ObjectId id : last_destroyed) {
      auto i = traces_.find(id);
      if (i != traces_.end()) {
         stdutil::ForEachPrune<Trace>(i->second, [](TracePtr t) {
            t->SignalDestroyed();
         });
         traces_.erase(i);
      }
   };
}

