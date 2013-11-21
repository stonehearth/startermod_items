from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std

class Destination(Component):

   region = dm.Boxed(Region3BoxedPtr())
   reserved = dm.Boxed(Region3BoxedPtr())
   adjacent = dm.Boxed(Region3BoxedPtr(), set='declare')
   auto_update_adjacent = dm.Boxed(c.bool(), set='declare')
   
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
