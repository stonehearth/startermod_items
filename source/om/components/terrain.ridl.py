import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Terrain(Component):
   config_file_name = dm.Boxed(std.string())
   tiles = dm.Map(csg.Point3(), Region3BoxedPtr(), singular_name='tile', add=None, remove=None, get=None)
   bounds = dm.Boxed(csg.Cube3(), get=None, set=None, no_lua_impl = True)
   interior_region = dm.Boxed(Region3fBoxedPtr(), trace='deep_region')

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

   _generate_construct_object = True

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
   void AddTileClipped(csg::Region3f const& region, csg::Rect2 const* clipper);

   TerrainTesselator terrainTesselator_;
   dm::TracePtr config_file_name_trace_;
   """
