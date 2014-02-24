import ridl.ridl as ridl
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class UnitInfo(Component):
   display_name = dm.Boxed(std.string())
   description = dm.Boxed(std.string())
   portrait = dm.Boxed(std.string())
   character_sheet_info = dm.Boxed(std.string())
   faction = dm.Boxed(std.string())
   kingdom = dm.Boxed(std.string())
   icon = dm.Boxed(std.string())
