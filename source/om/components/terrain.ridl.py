import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Terrain(Component):
   block_types = ridl.Enum('Terrain', 'BlockTypes',
      Null          = 0,
      Bedrock       = 1,
      RockLayer1    = 100,
      RockLayer2    = 101,
      RockLayer3    = 102,
      RockLayer4    = 103,
      RockLayer5    = 104,
      RockLayer6    = 105,
      SoilLight     = 200,
      SoilDark      = 201,
      Grass         = 300,
      GrassEdge1    = 301,
      GrassEdge2    = 302,
      Dirt          = 400,
      DirtEdge1     = 401,
    )

   tiles = dm.Map(csg.Point3f(), Region3fBoxedPtr(), singular_name='tile', add=None, remove=None, get=None)
   tile_size = dm.Boxed(c.float())

   cached_bounds = dm.Boxed(csg.Cube3f(), no_lua_impl = True)
   origin_offset = dm.Boxed(csg.Point3f())
   in_bounds = ridl.Method(c.bool(), ('location', csg.Point3f().const.ref)).const
   get_bounds = ridl.Method(csg.Cube3f()).const
   add_tile = ridl.Method(c.void(),
                          ('tile_offset', csg.Point3f().const.ref),
                          ('region', csg.Region3f().const.ref))
   get_point_on_terrain =  ridl.Method(csg.Point3f(), ('pt', csg.Point3f().const.ref)).const

   add_cube = ridl.Method(c.void(), ('cube', csg.Cube3f().const.ref))
   add_region = ridl.Method(c.void(), ('region', csg.Region3f().const.ref))
   subtract_cube = ridl.Method(c.void(), ('cube', csg.Cube3f().const.ref))
   subtract_region = ridl.Method(c.void(), ('region', csg.Region3f().const.ref))
   intersect_cube = ridl.Method(csg.Region3f(), ('cube', csg.Cube3f().const.ref))
   intersect_region = ridl.Method(csg.Region3f(), ('region', csg.Region3f().const.ref))

   _includes = [
      "om/components/terrain_tesselator.h",
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
   typedef std::function<void(csg::Point3f tile_offset, csg::Region3f& tile_region, csg::Region3f& chopped_region)> ApplyRegionToTileCb;
   TerrainTesselator terrainTesselator_;
   csg::Cube3f CalculateBounds() const;
   csg::Cube3f GetTileBounds(csg::Point3f const& tile_offset) const;
   void ApplyRegionToTiles(csg::Region3f const& region, ApplyRegionToTileCb const& operation);
   """
