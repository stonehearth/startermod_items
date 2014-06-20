#include "radiant.h"
#include "radiant_stdutil.h"
#include "tracer_buffered.h"
#include "object.h"
#include "object_macros.h"
#include "store.h"
#include "store_trace.h"

using namespace ::radiant;
using namespace ::radiant::dm;

#define TRACE_LOG(level)            LOG_CATEGORY(dm.trace.buffered, level, "buffered tracer: " << GetName())
#define TRACE_LOG_ENABLED(level)    LOG_IS_ENABLED(dm.trace.buffered, level)

TracerBuffered::TracerBuffered(std::string const& name, Store& store) :
   Tracer(name)
{
   store_trace_ = store.TraceStore("tracer buffered")
      ->OnDestroyed([this](ObjectId id, bool dynamic) {
         OnObjectDestroyed(id);
      })
      ->OnModified([this](ObjectId id) {
         OnObjectModified(id);
      });
}

TracerBuffered::~TracerBuffered()
{
}

void TracerBuffered::OnObjectModified(ObjectId id)
{
   TRACE_LOG(5) << "adding object " << id << " to modified set len:" << modified_objects_.size();
   modified_objects_.insert(id);

   if (TRACE_LOG_ENABLED(9)) {
      std::ostringstream buf;
      for (dm::ObjectId id : modified_objects_) {
         buf << " " << id;
      }
      TRACE_LOG(5) << "  now: " << buf.str();
   }
}

void TracerBuffered::OnObjectDestroyed(ObjectId id)
{
   TRACE_LOG(5) << "adding object " << id << " to destroyed set len:" << destroyed_objects_.size();
   destroyed_objects_.push_back(id);
   modified_objects_.erase(id);
}

void TracerBuffered::Flush()
{
   TRACE_LOG(5) << "starting flush...";

   bool finished;
   decltype(modified_objects_) modified;
   decltype(destroyed_objects_) destroyed;

   do {
      modified = std::move(modified_objects_);
      destroyed = std::move(destroyed_objects_);

      TRACE_LOG(5) << "flushing -> ("
                << "modified: " << modified.size() << " "
                << "destroyed: " << destroyed.size() << ")";     
      FlushOnce(modified, destroyed);

      finished = modified_objects_.empty() && destroyed_objects_.empty();
   } while (!finished);

   TRACE_LOG(5) << "finished flush";
}

void TracerBuffered::FlushOnce(ModifiedObjectsSet& last_modified,
                               std::vector<ObjectId>& last_destroyed)
{
   for (ObjectId id : last_modified) {
      auto range = traces_.equal_range(id);
      auto i = range.first;
      while (i != range.second) {
         TraceBufferedPtr t = i->second.lock();
         if (t) {
            t->Flush();
            ++i;
         } else {
            i = traces_.erase(i);
         }
      }
   }

   for (ObjectId id : last_destroyed) {
      auto range = traces_.equal_range(id);
      auto i = range.first;
      while (i != range.second) {
         TraceBufferedPtr t = i->second.lock();
         if (t) {
            t->SignalDestroyed();
            ++i;
         } else {
            i = traces_.erase(i);
         }
      }
   };
}

