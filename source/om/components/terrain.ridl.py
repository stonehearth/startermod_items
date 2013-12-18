import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Terrain(Component):
   block_types = ridl.Enum('Terrain', 'BlockTypes',
      Null          = 0,
      RockLayer1    = 1,
      RockLayer2    = 2,
      RockLayer3    = 3,
      Boulder       = 4,
      Soil          = 5,
      DarkGrass     = 6,
      DarkGrassDark = 7,
      LightGrass    = 8,
      DarkWood      = 9, 
      DirtRoad      = 10, 
      Magma         = 11,
   )

   zones = dm.Map(csg.Point3(), Region3BoxedPtr(), add=None, remove=None, get=None)
   zone_size = dm.Boxed(c.int())

   get_bounds = ridl.Method(csg.Cube3())
   add_zone = ridl.Method(c.void(),
                          ('zone_offset', csg.Point3().const.ref),
                          ('region3', Region3BoxedPtr()))
   create_new = ridl.Method(c.void())
   place_entity = ridl.Method(c.void(), ('e', EntityRef()), ('location', csg.Point3().const.ref))
   
   _includes = [
      "om/region.h",
      "csg/point.h"
   ]

   _public = \
   """
   Region3BoxedPtr GetZone(csg::Point3 const& location, csg::Point3& zone_offset);
   """
