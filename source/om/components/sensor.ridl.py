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
   contents = dm.Map(dm.ObjectId(), std.weak_ptr(Entity()), iterate='define')

   _lua_weak_ref = True

   _includes = [
      "csg/cube.h"
   ]

   _public = \
   """
   void UpdateIntersection(std::unordered_map<dm::ObjectId, om::EntityRef> const&);
   dm::Map<dm::ObjectId, std::weak_ptr<Entity>>& GetContainer();
   """

   _protected = \
   """
   friend SensorList;

   void Construct(om::EntityRef entity, std::string name, const csg::Cube3& cube) {
      entity_ = entity;
      name_ = name;
      cube_ = cube;
   }
   """
