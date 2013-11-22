#include "radiant.h"
#include "streamer.h"
#include "store.h"
#include "alloc_trace.h"
#include "protocol.h"
#include "tracer_buffered.h"

using namespace radiant;
using namespace radiant::dm;

namespace proto = ::radiant::tesseract::protocol;

Streamer::Streamer(Store& store, int category, protocol::SendQueue* queue) :
   store_(store),
   queue_(queue),
   category_(category)
{
   tracer_ = std::make_shared<TracerBuffered>(store, category);
   store.AddTracer(tracer_, category);

   alloc_trace_ = store.TraceAlloced("streamer", category)
                           ->OnUpdated([this](std::vector<ObjectPtr> const& objects) {
                              OnAlloced(objects);
                           })
                           ->PushObjectState();
}

void Streamer::Flush(int now)
{
   SetServerTick(now);
   
   tracer_->Flush();

   if (destroyed_msg_) {
      FinishUpdate(destroyed_update_);
      destroyed_msg_ = nullptr;
   }
}

void Streamer::SetServerTick(int now)
{
   proto::Update update;

   update.set_type(proto::Update::SetServerTick);
   auto msg = update.MutableExtension(proto::SetServerTick::extension);
   msg->set_now(now);
   queue_->Push(protocol::Encode(update));
}

void Streamer::OnAlloced(std::vector<ObjectPtr> const& objects)
{
   ASSERT(!objects.empty());

   proto::Update update;
   update.set_type(proto::Update::AllocObjects);
   auto msg = update.MutableExtension(proto::AllocObjects::extension);
   for (ObjectPtr obj : objects) {
      ObjectId id = obj->GetObjectId();
      ObjectType type = obj->GetObjectType();
      ObjectRef o = obj;

      auto allocMsg = msg->add_objects();
      allocMsg->set_object_id(id);
      allocMsg->set_object_type(type);

      auto trace = obj->TraceObjectChanges("streamer", category_);
      TraceBufferedRef t = std::dynamic_pointer_cast<TraceBuffered>(trace);
      trace->OnModified([this, t, o]() {
         OnModified(t, o);
      })
      ->OnDestroyed([this, id]() {
         OnDestroyed(id);
      });
      dtor_traces_[id] = trace;
   }
   queue_->Push(protocol::Encode(update));
   // todo: Push the current state of all objects, right?
}

void Streamer::OnModified(TraceBufferedRef t, ObjectRef o)
{
   ObjectPtr obj = o.lock();
   TraceBufferedPtr trace = t.lock();
   if (trace && obj) {
      proto::Update update;

      update.set_type(proto::Update::UpdateObject);
      Protocol::Object* msg = update.MutableExtension(proto::UpdateObject::extension)->mutable_object();
      msg->set_object_id(obj->GetObjectId());
      msg->set_object_type(obj->GetObjectType());
      msg->set_timestamp(obj->GetLastModified());
      trace->SaveObjectDelta(obj.get(), msg->mutable_value());

      FinishUpdate(update);
   }
}

void Streamer::OnDestroyed(ObjectId id)
{
   if (!destroyed_msg_) {
      destroyed_update_.Clear();
      destroyed_update_.set_type(proto::Update::RemoveObjects);
      destroyed_msg_ = destroyed_update_.MutableExtension(proto::RemoveObjects::extension);
   }
   destroyed_msg_->add_objects(id);
}

Protocol::Object* Streamer::StartUpdate(proto::Update& update, ObjectId id)
{
   namespace proto = ::radiant::tesseract::protocol;
   Object* obj = store_.FetchStaticObject(id);
   if (!obj) {
      return nullptr;
   }

   update.Clear();
   update.set_type(proto::Update::UpdateObject);

   Protocol::Object* msg = update.MutableExtension(proto::UpdateObject::extension)->mutable_object();
   msg->set_object_id(id);
   msg->set_object_type(obj->GetObjectType());
   msg->set_timestamp(obj->GetLastModified());

   return msg;
}

void Streamer::FinishUpdate(proto::Update const& update)
{
   queue_->Push(protocol::Encode(update));
}

#if 0

Streamer::Streamer(Store& store, int category) :
   TracerBuffered(category),
   store_(store),
   queue_(nullptr)
{
}

Tracer::TracerType Streamer::GetType() const
{
   return STREAMER;
}

// xxx: this should move to the constructor once the arbiter is smart
// enough to let the simulation handle queue lifetime.
void Streamer::Flush(protocol::SendQueue* queue)
{
   queue_ = queue;
   TracerBuffered::Flush();
   queue_ = nullptr;
}


#endif
