from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.csg_types as csg
import ridl.std_types as std

class Destination(Component):

   region = dm.Boxed(Region3fBoxedPtr(), trace='deep_region')
   reserved = dm.Boxed(Region3fBoxedPtr(), trace='deep_region')
   adjacent = dm.Boxed(Region3fBoxedPtr(), set='declare', trace='deep_region')
   auto_update_adjacent = dm.Boxed(c.bool(), set='declare')
   allow_diagonal_adjacency = dm.Boxed(c.bool(), set='declare')
   get_point_of_interest = ridl.Method(csg.Point3f(), ('pt', csg.Point3f().const.ref)).const
   _includes = [
      "om/region.h"
   ]
   _generate_construct_object = True

   _private = \
   """
   void OnAutoUpdateAdjacentChanged();
   void Initialize() override;
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3f const& r);
   csg::Point3f GetBestPointOfInterest(csg::Region3f const& r, csg::Point3f const& pt) const;

private:
   DeepRegion3fGuardPtr      region_trace_;
   DeepRegion3fGuardPtr      reserved_trace_;
   """
