#include "pch.h"
#include "csg/cube.h"
#include "sensor.ridl.h"
#include "sensor_list.ridl.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, const SensorList& o)
{
   os << "[SensorList]";
   return os;
}

void SensorList::LoadFromJson(json::Node const& obj)
{
   for (auto const& e : obj.get_node("sensors")) {
      AddSensor(e.name(), e.get<int>("radius"));
   }
}

void SensorList::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   for (auto const& entry : sensors_) {
      json::Node sensor;
      sensor.set("radius", -1337);
      node.set(stdutil::ToString(entry.first), sensor);
   }
}

SensorRef SensorList::AddSensor(std::string const& name, int radius)
{
   // xxx: use region3's for sensors?
   float r = (float)radius;
   SensorPtr sensor = GetStore().AllocObject<Sensor>();
   csg::Cube3f box(csg::Point3f(-r, -r, -r), csg::Point3f(r, r, r));
   sensor->SetCube(box);
   sensor->SetEntity(GetEntityRef());

   sensors_.Add(name, sensor);
   return sensor;
}
