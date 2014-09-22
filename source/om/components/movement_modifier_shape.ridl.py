from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class MovementModifierShape(Component):
   region = dm.Boxed(Region3fBoxedPtr(), trace='deep_region')
   modifier = dm.Boxed(c.float())

   _generate_construct_object = True

   _includes = [
      "om/region.h",
      "dm/dm_save_impl.h",
   ]
