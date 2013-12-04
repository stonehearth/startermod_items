import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class TargetTable(dm.Record):
   name = 'TargetTable'

class TargetTableEntry(object):
   name = 'TargetTableEntry'

class TargetTableGroup(dm.Record):
   top = dm.Boxed(std.shared_ptr(TargetTableEntry))
   tables = dm.Set(std.shared_ptr(TargetTable), singular_name='table', add=None)
   category = dm.Boxed(std.string())

   add_table = ridl.Method(std.shared_ptr(TargetTable()))

   _includes = [
      "om/components/target_table.ridl.h"
   ]

   _public = \
   """
   void Update(int now, int interval);
   """


