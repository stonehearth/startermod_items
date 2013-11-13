#include "pch.h"
#include "sensor_list.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& os, const Sensor& o)
{
   os << "[Sensor " << o.GetObjectId() << " name:" << o.GetName() << "]";
   return os;
}

void SensorList::ExtendObject(json::Node const& obj)
{
   for (auto const& e : obj.GetNode()["sensors"]) {
      AddSensor(e.name(), e["radius"].as_int());
   }
}

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
      for (EntityId id : contains_) {
         if (!stdutil::contains(intersection, id)) {
            missing.push_back(id);
         }
      }
      for (EntityId id : missing) {
         contains_.Remove(id);
      }

      // added...
      std::vector<EntityId> added;
      for (EntityId id : contains_) {
         stdutil::UniqueRemove(intersection, id);
      }
      for (EntityId id : intersection) {
         if (entity_id != id) {
            LOG(WARNING) << "adding entity " << id << " to sensor for entity " << entity_id;
            contains_.Insert(id);
         }
      }
   }
}

void SensorList::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("sensors", sensors_);
}

SensorPtr SensorList::GetSensor(std::string name)
{
   return sensors_.Lookup(name, nullptr);
}

SensorPtr SensorList::AddSensor(std::string name, int radius)
{
   float r = (float)radius;
   SensorPtr sensor = GetStore().AllocObject<Sensor>();

   csg::Cube3f box(csg::Point3f(-r, -r, -r), csg::Point3f(r, r, r));
   sensor->SetCube(box);
   sensor->SetEntity(GetEntityRef());

   sensors_[name] = sensor;
   return sensor;
}

void SensorList::RemoveSensor(std::string name)
{
   sensors_.Remove(name);
}
