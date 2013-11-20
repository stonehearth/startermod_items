import ridl.ridl as ridl
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class UnitInfo(Component):
   name = dm.Boxed(std.string())
   description = dm.Boxed(std.string())
   faction = dm.Boxed(std.string())
   icon = dm.Boxed(std.string())
