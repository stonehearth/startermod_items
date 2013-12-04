#include "pch.h"
#include "sensor.ridl.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, const Sensor& o)
{
   os << "[Sensor " << o.GetObjectId() << " name:" << o.GetName() << "]";
   return os;
}

void Sensor::UpdateIntersection(std::unordered_map<dm::ObjectId, om::EntityRef> const& intersection)
{
   // xxx: move this whole routine into the dm::Set class?  It would
   // facilitiate optimization...

   EntityRef e = GetEntity();
   EntityPtr entity = e.lock();
   if (entity) {
      int entity_id = entity->GetObjectId();

      // removed...
      std::vector<dm::ObjectId> removed;
      for (const auto& entry : contents_) {
         auto i = intersection.find(entry.first);
         if (i == intersection.end()) {
            removed.push_back(entry.first);
         }
      }
      for (dm::ObjectId id : removed) {
         contents_.Remove(id);
      }

      // added...
      for (const auto& entry : intersection) {
         auto i = contents_.find(entry.first);
         if (i == contents_.end()) {
            contents_.Add(entry.first, entry.second);
         }
      }
   }
}
