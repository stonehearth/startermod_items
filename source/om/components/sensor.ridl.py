import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Sensor(dm.Record):
   entity = dm.Boxed(EntityRef())
   name = dm.Boxed(std.string())
   cube = dm.Boxed(csg.Cube3())
   contents = dm.Map(dm.ObjectId(), std.weak_ptr(Entity()), iterate='define', add=None, remove=None)

   _lua_weak_ref = True

   _includes = [
      "csg/cube.h",
      "physics/sensor_tracker.h"
   ]

   _protected = \
   """
   friend phys::SensorTracker;
   friend SensorList;

   void Construct(om::EntityRef entity, std::string name, const csg::Cube3& cube) {
      entity_ = entity;
      name_ = name;
      cube_ = cube;
   }

   dm::Map<dm::ObjectId, std::weak_ptr<Entity>>& GetContainer();

   void UpdateIntersection(std::unordered_map<dm::ObjectId, om::EntityRef> const&);

   """
