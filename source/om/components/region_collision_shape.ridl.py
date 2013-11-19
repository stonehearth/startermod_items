from ridl.om import *
from ridl.ridl import *

class RegionCollisionShape(Component):
   region = dm.Boxed(Ptr(dm.Boxed(csg.Region3)))
