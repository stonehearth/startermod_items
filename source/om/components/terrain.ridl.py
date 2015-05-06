import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class Terrain(Component):
   config_file_name = dm.Boxed(std.string(), set='declare')
   bounds = dm.Boxed(csg.Cube3f(), get=None, set=None, no_lua_impl = True)
   tiles = dm.Map(csg.Point3(), Region3BoxedPtr(), singular_name='tile', add=None, remove=None, get=None, contains=None, num=None)
   interior_tiles = dm.Map(csg.Point3(), Region3BoxedPtr(), singular_name='interior_tile', add=None, remove=None, get=None, contains=None, num=None)
   delta_region = dm.Boxed(csg.Region3f(), get=None, set=None)

   get_water_tight_region = ridl.Method(Region3TiledPtr())
   water_tight_region_delta = dm.Boxed(csg.Region3f(), get=None, set=None)
   clear_water_tight_region_delta = ridl.Method(c.void())

   mined_region_tiles = dm.Map(csg.Point3(), Region3BoxedPtr(), singular_name='mined_region_tile', add=None, remove=None, get=None, contains=None, num=None)

   is_empty = ridl.Method(c.bool())
   get_bounds = ridl.Method(csg.Cube3f())
   add_tile = ridl.Method(c.void(), ('region', csg.Region3f().const.ref))
   get_tiles = ridl.Method(Region3BoxedTiledPtr())
   get_interior_tiles = ridl.Method(Region3BoxedTiledPtr())
   get_mined_region = ridl.Method(Region3BoxedTiledPtr())
   get_point_on_terrain =  ridl.Method(csg.Point3f(), ('pt', csg.Point3f().const.ref))
   get_terrain_ring_tesselator = ridl.Method(TerrainRingTesselatorPtr()).const

   _declare_constructor = True
   _generate_construct_object = True

   _includes = [
      "om/components/terrain_ring_tesselator.h",
      "om/region.h",
      "om/tiled_region.h",
      "csg/util.h",
      "csg/point.h",
      "simulation/simulation.h"
   ]

   _public = \
   """
   dm::Boxed<csg::Region3f>* GetWaterTightRegionDelta();
   csg::Point3 const& GetTileSize() const;
   void OnLoadObject(dm::SerializationType r) override;
   """

   _private = \
   """
   void SetBounds(csg::Cube3f const& bounds);
   void GrowBounds(csg::Cube3f const& cube);
   Region3BoxedTiledPtr CreateTileAccessor(Region3BoxedPtrMap& tiles);
   void ReadConfigFile();
   void DeferredInitialize();

   Region3BoxedTiledPtr tile_accessor_;
   Region3BoxedTiledPtr interior_tile_accessor_;
   Region3BoxedTiledPtr mined_region_accessor_;
   TerrainRingTesselatorPtr terrainRingTesselator_;

   Region3PtrMap water_tight_region_tiles_;
   Region3TiledPtr water_tight_region_;
   """
