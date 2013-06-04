#include "pch.h"
#include "sensor_list.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

luabind::scope Sensor::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;

   std::string contentsName = std::string(name) + "Contents";

   return
      class_<Sensor, SensorPtr>(name)
         .def("set_name",                 &om::Sensor::SetName)
         .def("get_name",                 &om::Sensor::GetName)
         .def("get_contents",             &om::Sensor::GetContents)
      ;
}

luabind::scope SensorList::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<SensorList, Component, std::weak_ptr<Component>>(name)
         .def("add_sensor",               &om::SensorList::AddSensor)
         .def("get_sensor",               &om::SensorList::GetSensor)
         .def("remove_sensor",            &om::SensorList::RemoveSensor)
      ;
}

void SensorList::ExtendObject(json::ConstJsonObject const& obj)
{
   for (auto const& e : obj.GetNode()["sensors"]) {
      AddSensor(e.name(), e["radius"].as_int());
   }
}

void Sensor::UpdateIntersection(std::vector<EntityId> intersection)
{
   // xxx: move this whole routine into the dm::Set class?  It would
   // facilitiate optimization...

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
      contains_.Insert(id);
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
   SensorPtr sensor = GetStore().AllocObject<Sensor>();

   csg::Cube3 box(csg::Point3(-radius, -radius, -radius), csg::Point3(radius, radius, radius));
   sensor->SetCube(box);
   sensor->SetEntity(GetEntityRef());

   sensors_[name] = sensor;
   return sensor;
}

void SensorList::RemoveSensor(std::string name)
{
   sensors_.Remove(name);
}
