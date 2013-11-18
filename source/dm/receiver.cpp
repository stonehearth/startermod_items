#include "radiant.h"
#include "receiver.h"
#include "object.h"
#include "store.h"
#include "map_loader.h"

using namespace radiant;
using namespace radiant::dm;

Receiver::Receiver(Store& store) :
   store_(store)
{
}

void Receiver::ProcessAlloc(tesseract::protocol::AllocObjects const& msg)
{
   for (const tesseract::protocol::AllocObjects::Entry& entry : msg.objects()) {
      ObjectId id = entry.object_id();
      ASSERT(!store_.FetchStaticObject(id));

      objects_[id] = store_.AllocSlaveObject(entry.object_type(), id);
   }
}

void Receiver::ProcessUpdate(tesseract::protocol::UpdateObject const& msg) 
{
   const auto& update = msg.object();
   ObjectId id = update.object_id();

   Object* obj = store_.FetchStaticObject(id);
   ASSERT(obj);
   ASSERT(update.object_type() == obj->GetObjectType());

   obj->LoadObject(update);
}

void Receiver::ProcessRemove(tesseract::protocol::RemoveObjects const& msg)
{
   for (int id : msg.objects()) {
      auto i = objects_.find(id);

      ASSERT(i != objects_.end());
      if (i != objects_.end()) {
         objects_.erase(i);
      }
   }
}
