#include "radiant.h"
#include "receiver.h"
#include "object.h"
#include "store.h"

using namespace radiant;
using namespace radiant::dm;

#define RECEIVER_LOG(level)      LOG(dm.receiver, level)

Receiver::Receiver(Store& store) :
   store_(store)
{
}

void Receiver::ProcessAlloc(tesseract::protocol::AllocObjects const& msg)
{
   RECEIVER_LOG(5) << "received alloc msg (" << msg.objects_size() << " objects)";
   for (const tesseract::protocol::AllocObjects::Entry& entry : msg.objects()) {
      ObjectId id = entry.object_id();
      ASSERT(!store_.FetchStaticObject(id));

      RECEIVER_LOG(5) << "allocating object " << id << " of type " << entry.object_type();
      dm::ObjectPtr obj = store_.AllocObject(entry.object_type(), id);
      objects_[id] = obj;
      RECEIVER_LOG(3) << "finished allocating object " << id << " of type " << obj->GetObjectClassNameLower();
   }
}

void Receiver::ProcessUpdate(tesseract::protocol::UpdateObject const& msg) 
{
   const auto& update = msg.object();
   ObjectId id = update.object_id();
   ObjectType type = update.object_type();
   RECEIVER_LOG(5) << "loading object " << id << " type " << type;

   Object* obj = store_.FetchStaticObject(id);
   ASSERT(obj);
   ASSERT(type == obj->GetObjectType());

   obj->LoadObject(REMOTING, update);
}

void Receiver::ProcessRemove(tesseract::protocol::RemoveObjects const& msg)
{
   for (int id : msg.objects()) {
      auto i = objects_.find(id);

      ASSERT(i != objects_.end());
      if (i != objects_.end()) {
         dm::ObjectRef o = i->second;
         RECEIVER_LOG(3) << "destroying object " << id << " " << o.lock()->GetObjectClassNameLower();
         objects_.erase(i);

         if (o.use_count() != 0) {
            dm::ObjectPtr obj = o.lock();
            dm::ObjectId id = obj->GetObjectId();
            std::string name = obj->GetObjectClassNameLower();
            obj = nullptr;

            RECEIVER_LOG(0) << "destroy request from streamer failed to release last reference to " << name << " " << id << "(use_count: "
               << o.use_count() << ")";
         }
      } else {
         RECEIVER_LOG(3) << "* received remove msg for unknown object " << id;
      }
   }
}

void Receiver::Shutdown()
{
   objects_.clear();
}
