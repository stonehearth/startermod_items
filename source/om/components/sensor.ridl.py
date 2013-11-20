import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Sensor(dm.Record):
   entity = dm.Boxed(EntityRef())
   name = dm.Boxed(std.string())
   cube = dm.Boxed(csg.Cube3f())
   contents = dm.Set(dm.ObjectId())

   _includes = [
      "csg/cube.h"
   ]

   _public = \
   """
   void UpdateIntersection(std::vector<EntityId> intersection);
   """

   _protected = \
   """
   friend SensorList;

   void Construct(om::EntityRef entity, std::string name, const csg::Cube3f& cube) {
      entity_ = entity;
      name_ = name;
      cube_ = cube;
   }
   """
