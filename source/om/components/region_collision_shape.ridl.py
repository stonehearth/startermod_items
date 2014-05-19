from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class RegionCollisionShape(Component):
   region_collision_types = ridl.Enum('RegionCollisionShape', 'RegionCollisionTypes',
      NONE = 0,
      SOLID = 1
   )
   region_collision_type = dm.Boxed(region_collision_types)
   region = dm.Boxed(Region3BoxedPtr(), trace='deep_region')

   _includes = [
      "om/region.h",
      "dm/dm_save_impl.h",
   ]

   _global_post = \
   """
IMPLEMENT_DM_ENUM(radiant::om::RegionCollisionShape::RegionCollisionTypes);
   """
