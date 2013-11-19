from ridl.om import *
from ridl.ridl import *

class Destination(Component):
   region = dm.Boxed(Ptr(dm.Boxed(csg.Region3)))
   reserved = dm.Boxed(Ptr(dm.Boxed(csg.Region3)), set=declare)
   adjacent = dm.Boxed(Ptr(dm.Boxed(csg.Region3)), set=declare)
   auto_update_adjacent = dm.Boxed(bool, set=declare)

   _private_methods = """
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);
   """
