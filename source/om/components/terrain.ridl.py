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

   tiles = dm.Map(csg.Point3f(), Region3fBoxedPtr(), singular_name='tile', add=None, remove=None, get=None)
   tile_size = dm.Boxed(c.float())

   cached_bounds = dm.Boxed(csg.Cube3f())
   cached_bounds_is_valid = dm.Boxed(c.bool())

   in_bounds = ridl.Method(c.bool(), ('location', csg.Point3f().const.ref)).const
   get_bounds = ridl.Method(csg.Cube3f()).const
   add_tile = ridl.Method(c.void(),
                          ('tile_offset', csg.Point3f().const.ref),
                          ('region3', Region3fBoxedPtr()))
   get_point_on_terrain =  ridl.Method(csg.Point3f(), ('pt', csg.Point3f().const.ref)).const

   _includes = [
      "om/region.h",
      "csg/util.h",
      "csg/point.h"
   ]

   _public = \
   """
   Region3fBoxedPtr GetTile(csg::Point3f const& location, csg::Point3f& tile_offset) const;
   """

   _private = \
   """
   csg::Cube3f CalculateBounds() const;
   """
