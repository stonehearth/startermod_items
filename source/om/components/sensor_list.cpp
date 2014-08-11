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

int GetRadius(SensorPtr sensor)
{
   csg::Cube3 cube = sensor->GetCube();
   int radius = -cube.min.x;

   for (int i = 0; i < 3; i++) {
      if (cube.min[i] != -radius || cube.max[i] != radius+1) {
         throw core::Exception(BUILD_STRING("Cannot determine radius for sensor: " << sensor->GetName()));
      }
   }

   return radius;
}

void SensorList::LoadFromJson(json::Node const& obj)
{
   for (auto const& node : obj.get_node("sensors")) {
      AddSensor(node.name(), node.get<int>("radius"));
   }
}

void SensorList::SerializeToJson(json::Node& obj) const
{
   Component::SerializeToJson(obj);

   for (auto const& entry : sensors_) {
      SensorPtr sensor = entry.second;
      json::Node node;

      int radius = GetRadius(sensor);
      node.set("radius", radius);
      obj.set(entry.first, node);
   }
}

SensorRef SensorList::AddSensor(std::string const& name, int radius)
{
   // Remember the +1 on max because we are centering the box around the origin voxel.
   // 
   // Extended discussion:
   // Consider the case where we have an entity with sensor radius 1 at world_grid_location (0,0,0)
   // and an oak_log at world_grid_location (1,0,0). The oak_log is distance 1 away from the entity,
   // so we expect that a sensor of radius 1 should detect it. By this definition, a sensor of radius 1
   // should extend 1 voxel out around the origin voxel. The origin voxel is defined in local coordinates
   // as (0,0,0) to (1,1,1), so centering around it should yield a cube of coordinates
   // (0-radius,0-radius,0-radius) to (1+radius,1+radius,1+radius).
   //
   // This implementation was confirmed in testing.
   csg::Cube3 cube(
      csg::Point3(0-radius, 0-radius, 0-radius),
      csg::Point3(1+radius, 1+radius, 1+radius)
   );

   SensorPtr sensor = GetStore().AllocObject<Sensor>();
   sensor->Construct(GetEntityRef(), name, cube);

   sensors_.Add(name, sensor);
   return sensor;
}
