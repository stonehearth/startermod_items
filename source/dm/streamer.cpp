#include "radiant.h"
#include "streamer.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

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

Protocol::Object* Streamer::StartUpdate(ObjectId id)
{
   ASSERT(queue_); // if this fires, someone probably called Flush() intead of Flush(queue)

   namespace proto = ::radiant::tesseract::protocol;
   Object* obj = store_.FetchStaticObject(id);
   if (!obj) {
      return nullptr;
   }

   update_.Clear();
   update_.set_type(proto::Update::UpdateObject);

   Protocol::Object* msg = update_.MutableExtension(proto::UpdateObject::extension)->mutable_object();
   msg->set_object_id(id);
   msg->set_object_type(obj->GetObjectType());
   msg->set_timestamp(obj->GetLastModified());

   return msg;
}

void Streamer::FinishUpdate(::Protocol::Object* msg)
{
   ASSERT(msg);
   ASSERT(queue_);
   queue_->Push(protocol::Encode(update_));
}

