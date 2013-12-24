from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.csg_types as csg
import ridl.std_types as std

class Destination(Component):

   region = dm.Boxed(Region3BoxedPtr(), trace='deep_region', trace_category='LUA_SYNC_TRACES')
   reserved = dm.Boxed(Region3BoxedPtr(), trace='deep_region', trace_category='LUA_SYNC_TRACES')
   adjacent = dm.Boxed(Region3BoxedPtr(), set='declare', trace='deep_region')
   auto_update_adjacent = dm.Boxed(c.bool(), set='declare')
   allow_diagonal_adjacency = dm.Boxed(c.bool(), set='declare')
   get_point_of_interest = ridl.Method(csg.Point3(), ('pt', csg.Point3().const.ref)).const
   _includes = [
      "om/region.h"
   ]
   _generate_construct_object = True

   _private = \
   """
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);

private:
   DeepRegionGuardPtr      region_trace_;
   DeepRegionGuardPtr      reserved_trace_;
   """
