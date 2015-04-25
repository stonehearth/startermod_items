#include "radiant.h"
#include "receiver.h"
#include "object.h"
#include "store.h"
#include "client/client.h"
#include "om/components/data_store.ridl.h"

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

void Receiver::ProcessRemove(tesseract::protocol::RemoveObjects const& msg, std::function<void(dm::ObjectPtr)> const& onDestroyCb)
{
   for (int id : msg.objects()) {
      auto i = objects_.find(id);

      ASSERT(i != objects_.end());
      if (i != objects_.end()) {
         dm::ObjectRef o = i->second;
         std::string className, controllerName;
         {
            dm::ObjectPtr obj = o.lock();
            if (onDestroyCb) {
               onDestroyCb(obj);
            }
            className = obj->GetObjectClassNameLower();
            if (className == "data_store") {
               controllerName = std::static_pointer_cast<om::DataStore>(obj)->GetControllerName();
            }
         }
         objects_.erase(i);

         int c = o.use_count();
         if (c != 0) {
            RECEIVER_LOG(0) << "destroy request from streamer failed to release last reference to " << id << " " << className << " " << id << "(use_count: "
                            << c << "  controller?:" << controllerName << ")";
         } else {
            RECEIVER_LOG(3) << "destroying object " << id << " " << className << " " << id << "(use_count: "
                            << c << "  controller?:" << controllerName << ")";
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

void Receiver::Audit()
{
   RECEIVER_LOG(0) << "-- Receiver Datastores -----------------------------------";
   {
      std::unordered_map<std::string, std::unordered_map<int, int>> counts;
      for (auto const& entry : objects_) {
         if (entry.second->GetObjectType() == om::DataStore::DmType) {
            om::DataStorePtr ds = std::static_pointer_cast<om::DataStore>(entry.second);
            std::string controllerName = ds->GetControllerName();
            if (controllerName.empty()) {
               controllerName = "unnamed";
            }
            int c = ds.use_count() - 1;
            ++counts[controllerName][c];
         }
      };

      for (auto const& entry : counts) {
         std::string const& name = entry.first;
         for (auto const& e : entry.second) {
            int useCount = e.first;
            int total = e.second;
            RECEIVER_LOG(1) << "     " << std::setw(40) << name << " useCount:" << useCount << "   total:" << total;
         }
      }
   }
}
