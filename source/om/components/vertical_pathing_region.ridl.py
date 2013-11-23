import ridl.ridl as ridl
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
from ridl.om_types import *

class VerticalPathingRegion(Component):
   region = dm.Boxed(Region3BoxedPtr(), trace='deep_region')
   normal = dm.Boxed(csg.Point3())

   _includes = [
      "om/region.h"
   ]
