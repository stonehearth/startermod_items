from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class RegionCollisionShape(Component):
   region = dm.Boxed(Region3BoxedPtr(), trace='deep_region')

   _includes = [
      "om/region.h"
   ]

