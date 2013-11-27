#include "radiant.h"
#include "dm_log.h"
#include "streamer.h"
#include "object.h"
#include "store.h"
#include "alloc_trace.h"
#include "protocol.h"
#include "tracer_buffered.h"

#define STREAMER_LOG(level)      DM_LOG("streamer", level)

using namespace radiant;
using namespace radiant::dm;

namespace proto = ::radiant::tesseract::protocol;

Streamer::Streamer(Store& store, int category, protocol::SendQueue* queue) :
   queue_(queue),
   category_(category)
{
   tracer_ = std::make_shared<TracerBuffered>("streamer");
   store.AddTracer(tracer_, category);

   alloc_trace_ = store.TraceAlloced("streamer", category)
                           ->OnAlloced([this](ObjectPtr object) {
                              alloced_[object->GetObjectId()] = object;
                           })
                           ->PushObjectState();
}

void Streamer::Flush()
{
   STREAMER_LOG(5) << "flushing streamer";
   tracer_->Flush();
   QueueAlloced();
   QueueUpdates();
   QueueDestroyed();
   STREAMER_LOG(5) << "finished flushing streamer";

   ASSERT(alloced_.empty());
   ASSERT(updated_.empty());
   ASSERT(destroyed_.empty());
}

void Streamer::QueueAlloced()
{
   if (!alloced_.empty()) {
      QueueAllocs();
      QueueAllocUpdates();
      alloced_.clear();
   }
}

void Streamer::QueueAllocs()
{
   proto::Update update;
   update.set_type(proto::Update::AllocObjects);
   auto msg = update.MutableExtension(proto::AllocObjects::extension);

   auto i = alloced_.begin();
   while (i != alloced_.end()) {
      ObjectId id = i->first;
      ObjectPtr obj = i->second.lock();
      if (obj) {
         ObjectType type = obj->GetObjectType();

         STREAMER_LOG(5) << "adding object " << id << " to alloced message.";
         auto allocMsg = msg->add_objects();
         allocMsg->set_object_id(id);
         allocMsg->set_object_type(type);

         i++;
      } else {
         STREAMER_LOG(5) << "skipping object " << id << " when constructing alloc msg (already destroyed).";
         i = alloced_.erase(i);
      }
   }

   STREAMER_LOG(5) << "queueing alloc msg (" << alloced_.size() << " objects)";
   QueueUpdate(update);
}

void Streamer::QueueAllocUpdates()
{
   for (const auto& entry : alloced_) {
      ObjectRef o = entry.second;
      ObjectPtr obj = o.lock();
      if (obj) {
         auto trace = obj->TraceObjectChanges("streamer", category_);
         TraceBufferedRef t = std::dynamic_pointer_cast<TraceBuffered>(trace);
         ObjectId id = entry.first;

         trace->OnModified_([this, t, o]() {
            updated_.push_back(std::make_pair(t, o));
         });
         trace->OnDestroyed_([this, id]() {
            destroyed_.push_back(id);
         });
         dtor_traces_[id] = trace;

         STREAMER_LOG(5) << "queueing initial update msg for object " << id;

         proto::Update update;
         update.set_type(proto::Update::UpdateObject);
         Protocol::Object* msg = update.MutableExtension(proto::UpdateObject::extension)->mutable_object();
         obj->SaveObject(msg);
         QueueUpdate(update);
      }
   }
}

void Streamer::QueueUpdates()
{
   for (const auto& entry : updated_) {
      TraceBufferedPtr trace = entry.first.lock();
      ObjectPtr obj = entry.second.lock();
      if (trace && obj) {
         proto::Update update;

         update.set_type(proto::Update::UpdateObject);
         Protocol::Object* msg = update.MutableExtension(proto::UpdateObject::extension)->mutable_object();
         msg->set_object_id(obj->GetObjectId());
         msg->set_object_type(obj->GetObjectType());
         msg->set_timestamp(obj->GetLastModified());
         trace->SaveObjectDelta(msg->mutable_value());

         STREAMER_LOG(5) << "queueing update msg  (object: " << obj->GetObjectId() << ")";
         QueueUpdate(update);
      }
   }
   updated_.clear();
}

void Streamer::QueueDestroyed()
{
   if (!destroyed_.empty()) {
      tesseract::protocol::Update update;
      update.set_type(proto::Update::RemoveObjects);
      auto msg = update.MutableExtension(proto::RemoveObjects::extension);
      for (ObjectId id : destroyed_) {
         STREAMER_LOG(5) << "adding object to destroyed set (object: " << id << ")";
         msg->add_objects(id);
      }
      STREAMER_LOG(5) << "queueing destroyed msg (" << destroyed_.size() << " objects)";
      QueueUpdate(update);
      destroyed_.clear();
   }
}

void Streamer::QueueUpdate(proto::Update const& update)
{
   queue_->Push(protocol::Encode(update));
}
