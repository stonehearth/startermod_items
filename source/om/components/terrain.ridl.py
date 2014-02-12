import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Terrain(Component):
   block_types = ridl.Enum('Terrain', 'BlockTypes',
      Null          = 0,
      Soil          = 1,
      SoilStrata    = 2,
      Dirt          = 4,
      Grass         = 5,
      DarkGrass     = 6,
      Wood          = 9,
      Boulder       = 10,
      RockLayer1    = 11,
      RockLayer2    = 12,
      RockLayer3    = 13,
      RockLayer4    = 14,
      RockLayer5    = 15,
      RockLayer6    = 16
    )

   tiles = dm.Map(csg.Point3(), Region3BoxedPtr(), singular_name='tile', add=None, remove=None, get=None)
   tile_size = dm.Boxed(c.int())

   get_bounds = ridl.Method(csg.Cube3())
   add_tile = ridl.Method(c.void(),
                          ('tile_offset', csg.Point3().const.ref),
                          ('region3', Region3BoxedPtr()))
   create_new = ridl.Method(c.void())
   place_entity = ridl.Method(c.void(), ('e', EntityRef()), ('location', csg.Point3().const.ref))
   get_height =  ridl.Method(c.int(), ('location', csg.Point2().const.ref))

   _includes = [
      "om/region.h",
      "csg/point.h"
   ]

   _public = \
   """
   Region3BoxedPtr GetTile(csg::Point3 const& location, csg::Point3& tile_offset);
   """
