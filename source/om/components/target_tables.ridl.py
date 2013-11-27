import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class TargetTable(dm.Record):
   name = 'TargetTable'

class TargetTableGroup(dm.Record):
   name = 'TargetTableGroup'

class TargetTables(Component):
   tables = dm.Map(std.string(), std.shared_ptr(TargetTableGroup), add=None, singular_name='table')
   add_table = ridl.Method(std.shared_ptr(TargetTable()), ('category', std.string().const.ref))

   _includes = [
      "om/components/target_table.ridl.h",
      "om/components/target_table_group.ridl.h"
   ]
