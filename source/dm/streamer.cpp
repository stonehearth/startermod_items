#include "radiant.h"
#include "boxed.h"
#include "streamer.h"
#include "object.h"
#include "store.h"
#include "store_trace.h"
#include "protocol.h"
#include "tracer_buffered.h"
#include "protocols/store.pb.h"
#include "protocols/tesseract.pb.h"

#define STREAMER_LOG(level)      LOG(dm.streamer, level)

using namespace radiant;
using namespace radiant::dm;

namespace proto = ::radiant::tesseract::protocol;

Streamer::Streamer(Store& store, int category, protocol::SendQueue* queue) :
   store_(store),
   queue_(queue),
   category_(category)
{
   // Add a buffered tracer to the store to accumulate changes to the container
   // classes.  See OnAlloced() and TraceObject() for more info.
   tracer_ = std::make_shared<TracerBuffered>("streamer", store);
   store.AddTracer(tracer_, category);

   // Add ourselves to the store so we can receive direct notifiation of changes
   // from objects without going through the StoreTrace infrasturcture.  This is
   // a huge performance win for value-type objects (like Boxed<T>), which don't
   // support any kind of delta encoding.
   store.AddStreamer(this);
}

Streamer::~Streamer()
{
   store_.RemoveStreamer(this);
}

void Streamer::Flush()
{
   STREAMER_LOG(5) << "flushing streamer";

   tracer_->Flush();
   QueueAllocated();
   QueueUpdateInfo();
   QueueUnsavedObjects();
   QueueModifiedObjects();
   QueueDestroyedObjects();

   STREAMER_LOG(5) << "finished flushing streamer";
}

void Streamer::QueueAllocated()
{
   if (!allocated_objects_.empty()) {
      tesseract::protocol::Update update;
      update.set_type(proto::Update::AllocObjects);
      auto allocated_msg = update.MutableExtension(proto::AllocObjects::extension);

      for (const auto& entry : allocated_objects_) {
         ObjectPtr obj = entry.second.lock();
         if (obj) {
            auto msg = allocated_msg->add_objects();
            msg->set_object_id(entry.first);
            msg->set_object_type(obj->GetObjectType());
            STREAMER_LOG(5) << "sending allocated msg for object " << entry.first;
         }
      }
      QueueUpdate(update);
      allocated_objects_.clear();
   }
}


void Streamer::QueueUnsavedObjects()
{
   if (!unsaved_objects_.empty()) {
      proto::Update update;
      for (const auto& entry : unsaved_objects_) {
         STREAMER_LOG(5) << "sending new object state for object " << entry.first;
         update.Clear();
         update.set_type(proto::Update::UpdateObject);
         Protocol::Object* msg = update.MutableExtension(proto::UpdateObject::extension)->mutable_object();
         entry.second->SaveObject(REMOTING, msg);
         QueueUpdate(update);
      }
      unsaved_objects_.clear();
   }
}

void Streamer::QueueModifiedObjects()
{
   for (const auto &entry: object_updates_) {
      for (const auto &update : entry.second) {
         QueueUpdate(update);
      }
   }
   object_updates_.clear();
}

void Streamer::QueueDestroyedObjects()
{
   if (!destroyed_objects_.empty()) {
      proto::Update update;
      update.set_type(proto::Update::RemoveObjects);
      auto destroyed_msg = update.MutableExtension(proto::RemoveObjects::extension);

      for (const auto& entry : destroyed_objects_) {
         STREAMER_LOG(5) << "sending destroyed msg for object " << entry.first;
         destroyed_msg->add_objects(entry.first);
      }

      QueueUpdate(update);
      destroyed_objects_.clear();
   }
}

void Streamer::OnAlloced(ObjectPtr obj)
{
   ObjectId id = obj->GetObjectId();
   STREAMER_LOG(5) << "adding object " << id << " to alloced set.";

   allocated_objects_[id] = obj;
}

//
// -- Streamer::TraceObject
//
// Request by a dm::Object at creation time to figure out what needs to be remoted
// via our buffered tracer.  This is quite expensive and should only be used when
// the cost of using OnModifiedObject() everytime the object changes outweighs the
// cost of the Trace.  Generally, this is only true for container types (e.g. Map),
// that support streaming deltas and and grow quite large.
//
template <typename T>
void Streamer::TraceObject(T const* obj)
{
   // The first push must be the full state of the object.
   AddUnsavedObject(obj);

   // Create a trace for the object to encode the delta state right before the
   // push.
   auto trace = obj->TraceObjectChanges("streamer", category_);
   TraceBufferedPtr trace_buffered = std::dynamic_pointer_cast<TraceBuffered>(trace);
   TraceBufferedRef t = trace_buffered;
   trace->OnModified_([this, t, obj]() {
      SaveDeltaState(t, obj);
   });

   dm::ObjectId id = obj->GetObjectId();
   traces_.insert(std::make_pair(id, trace_buffered));
}

//
// -- Streamer::OnModified
//
// Request by a dm::Object at modification time to encode its state at the next
// push.  This will use the Object::SaveObject method, so is discouraged if the
// object supports an efficient delta encoding (see Streamer::TraceObject).
// In practice, this method is only used for Boxed and Record types (which, btw,
// are a VAST majority of the objects out there).
//
void Streamer::OnModified(Object const* obj)
{
   AddUnsavedObject(obj);
}


//
// -- Streamer::AddUnsavedObject
//
// Add an object to the `unsaved_objects_` list.  This results in a full encoding
// of the object's state next push.
//
void Streamer::AddUnsavedObject(Object const* obj)
{
   ObjectId id = obj->GetObjectId();
   STREAMER_LOG(5) << "adding object " << id << " to unsaved object set.";
   unsaved_objects_.insert(std::make_pair(id, obj));
}

void Streamer::OnDestroyed(ObjectId id, bool dynamic)
{
   auto i = unsaved_objects_.find(id);
   if (i != unsaved_objects_.end()) {
      if (dynamic) {
         STREAMER_LOG(5) << "removing unsaved object from set rather than adding to destroy set (object: " << id << ")";
      }
      unsaved_objects_.erase(i);
   } else {
      if (dynamic) {
         STREAMER_LOG(5) << "adding object to destroyed set (object: " << id << ")";
         destroyed_objects_[id] = true;
      }
   }


   STREAMER_LOG(7) << "removing from traces (object: " << id << ")";
   auto k = traces_.find(id);
   if (k != traces_.end()) {
      traces_.erase(k);
   }
   
   STREAMER_LOG(7) << "removing updates (object: " << id << ")";
   auto j = object_updates_.find(id);
   if (j != object_updates_.end()) {
      object_updates_.erase(j);
   }
}

void Streamer::SaveDeltaState(TraceBufferedRef t, Object const* obj)
{
   TraceBufferedPtr trace = t.lock();
   if (trace) {
      ObjectId id = obj->GetObjectId();
      if (!stdutil::contains(unsaved_objects_, id)) {
         STREAMER_LOG(5) << "adding object " << id << " " << obj->GetObjectClassNameLower() << " to modified object set.";

         proto::Update update;
         update.set_type(proto::Update::UpdateObject);
         Protocol::Object* msg = update.MutableExtension(proto::UpdateObject::extension)->mutable_object();

         msg->set_object_id(id);
         msg->set_object_type(obj->GetObjectType());
         msg->set_timestamp(obj->GetLastModified());
         if (trace->SaveObjectDelta(REMOTING, msg->mutable_value())) {
            STREAMER_LOG(5) << "queuing object state for object " << id;
            object_updates_[id].emplace_back(update);
         } else {
            STREAMER_LOG(5) << "ignoring changes to object " << id;
         }
      }
   }
}

void Streamer::QueueUpdate(proto::Update const& update)
{
   queue_->Push(update);
}


void Streamer::QueueUpdateInfo()
{
   tesseract::protocol::Update update;
   update.set_type(proto::Update::UpdateObjectsInfo);
   auto info_msg = update.MutableExtension(proto::UpdateObjectsInfo::extension);

   info_msg->set_update_count(unsaved_objects_.size() + object_updates_.size());
   info_msg->set_remove_count(destroyed_objects_.size());
   QueueUpdate(update);
}

#define CREATE_MAP(M)    template void Streamer::TraceObject(M const* obj);
#define CREATE_SET(S)    template void Streamer::TraceObject(S const* obj);

#include "types/all_map_types.h"
#include "types/all_set_types.h"
#include "types/all_loader_types.h"

ALL_DM_MAP_TYPES
ALL_DM_SET_TYPES
