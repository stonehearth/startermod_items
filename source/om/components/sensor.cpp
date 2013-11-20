#include "pch.h"
#include "sensor.ridl.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

#if 0
std::ostream& om::operator<<(std::ostream& os, const Sensor& o)
{
   os << "[Sensor " << o.GetObjectId() << " name:" << o.GetName() << "]";
   return os;
}
#endif

void Sensor::UpdateIntersection(std::vector<EntityId> intersection)
{
   // xxx: move this whole routine into the dm::Set class?  It would
   // facilitiate optimization...

   EntityRef e = GetEntity();
   EntityPtr entity = e.lock();
   if (entity) {
      int entity_id = entity->GetObjectId();

      // removed...
      std::vector<EntityId> missing;
      for (EntityId id : contents_) {
         if (!stdutil::contains(intersection, id)) {
            missing.push_back(id);
         }
      }
      for (EntityId id : missing) {
         contents_.Remove(id);
      }

      // added...
      std::vector<EntityId> added;
      for (EntityId id : contents_) {
         stdutil::UniqueRemove(intersection, id);
      }
      for (EntityId id : intersection) {
         if (entity_id != id) {
            LOG(WARNING) << "adding entity " << id << " to sensor for entity " << entity_id;
            contents_.Insert(id);
         }
      }
   }
}
