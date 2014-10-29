import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Terrain(Component):
   block_types = ridl.Enum('Terrain', 'BlockTypes',
      Null          = 0,
      Hidden        = 1,
      Bedrock       = 100,
      RockLayer1    = 101,
      RockLayer2    = 102,
      RockLayer3    = 103,
      RockLayer4    = 104,
      RockLayer5    = 105,
      RockLayer6    = 106,
      SoilLight     = 200,
      SoilDark      = 201,
      Grass         = 300,
      GrassEdge1    = 301,
      GrassEdge2    = 302,
      Dirt          = 400,
      DirtEdge1     = 401,
    )

   tiles = dm.Map(csg.Point3(), Region3BoxedPtr(), singular_name='tile', add=None, remove=None, get=None)

   bounds = dm.Boxed(csg.Cube3(), get=None, set=None, no_lua_impl = True)
   get_bounds = ridl.Method(csg.Cube3f()).const
   add_tile = ridl.Method(c.void(),
                          ('region', csg.Region3f().const.ref))
   add_tile_clipped = ridl.Method(c.void(),
                          ('region', csg.Region3f().const.ref),
                          ('clipper', csg.Rect2f().const.ref))
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
   csg::Point3 const& GetTileSize() const;
   """

   _private = \
   """
   Region3BoxedPtr GetTile(csg::Point3 const& index);
   void AddTileWorker(csg::Region3f const& region, csg::Rect2 const* clipper);

   TerrainTesselator terrainTesselator_;
   """
