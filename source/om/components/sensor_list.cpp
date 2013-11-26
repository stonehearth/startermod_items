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

void SensorList::ExtendObject(json::Node const& obj)
{
   for (auto const& e : obj.GetNode()["sensors"]) {
      AddSensor(e.name(), e["radius"].as_int());
   }
}

SensorPtr SensorList::AddSensor(std::string const& name, int radius)
{
   float r = (float)radius;
   SensorPtr sensor = GetStore().AllocObject<Sensor>();

   csg::Cube3f box(csg::Point3f(-r, -r, -r), csg::Point3f(r, r, r));
   sensor->SetCube(box);
   sensor->SetEntity(GetEntityRef());

   sensors_.Insert(name, sensor);
   return sensor;
}
