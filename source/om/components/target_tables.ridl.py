import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class TargetTableGroup(dm.Record):
   name = 'TargetTableGroup'

class TargetTables(Component):
   tables = dm.Map(std.string(), std.shared_ptr(TargetTableGroup), singular_name='table')
