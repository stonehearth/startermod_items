#include "radiant.h"
#include "streamer.h"
#include "object.h"
#include "store.h"
#include "store_trace.h"
#include "protocol.h"
#include "tracer_buffered.h"

#define STREAMER_LOG(level)      LOG(dm.streamer, level)

using namespace radiant;
using namespace radiant::dm;

namespace proto = ::radiant::tesseract::protocol;

Streamer::Streamer(Store& store, int category, protocol::SendQueue* queue) :
   store_(store),
   queue_(queue),
   category_(category)
{
   tracer_ = std::make_shared<TracerBuffered>("streamer", store);
   store.AddTracer(tracer_, category);

   store_trace_ = store.TraceStore("streamer")
                           ->OnAlloced([this](ObjectPtr object) {
                              OnAlloced(object);
                           })
                           ->OnRegistered([this](Object const* object) {
                              OnRegistered(object);
                           })
                           ->OnDestroyed([this](ObjectId id, bool dynamic) {
                              OnDestroyed(id, dynamic);
                           })         
                           ->PushStoreState();
}

void Streamer::Flush()
{
   STREAMER_LOG(5) << "flushing streamer";

   tracer_->Flush();
   QueueAllocated();
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
      for (const auto& entry : unsaved_objects_) {
         ObjectId id = entry.first;
         proto::Update update;
         update.set_type(proto::Update::UpdateObject);
         Protocol::Object* msg = update.MutableExtension(proto::UpdateObject::extension)->mutable_object();
         entry.second->SaveObject(REMOTING, msg);
         STREAMER_LOG(5) << "sending new object state for object " << entry.first;
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

void Streamer::OnRegistered(Object const* obj)
{
   ObjectId id = obj->GetObjectId();
   STREAMER_LOG(5) << "adding object " << id << " to unsaved object set.";
   unsaved_objects_[id] = obj;

   auto trace = obj->TraceObjectChanges("streamer", category_);
   TraceBufferedPtr trace_buffered = std::dynamic_pointer_cast<TraceBuffered>(trace);
   TraceBufferedRef t = trace_buffered;
   trace->OnModified_([this, t, obj]() {
      OnModified(t, obj);
   });
   traces_[id] = trace_buffered;
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

void Streamer::OnModified(TraceBufferedRef t, Object const* obj)
{
   TraceBufferedPtr trace = t.lock();
   if (trace) {
      ObjectId id = obj->GetObjectId();
      if (!stdutil::contains(unsaved_objects_, id)) {
         STREAMER_LOG(5) << "adding object " << id << " to modified object set.";

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
   queue_->Push(protocol::Encode(update));
}
