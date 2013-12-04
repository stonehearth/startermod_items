import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class TargetTableEntry(dm.Record):
   name = 'TargetTableEntry'

class TargetTable(dm.Record):
   entries = dm.Map(dm.ObjectId(), std.shared_ptr(TargetTableEntry), singular_name='entry', add=None, num=None)
   name = dm.Boxed(std.string(), set=None)
   category = dm.Boxed(std.string())
   top = dm.Boxed(std.shared_ptr(TargetTableEntry), set=None)
   add_entry = ridl.Method(std.shared_ptr(TargetTableEntry()), ('entity', std.weak_ptr(Entity())))

   _includes = [
      "om/components/target_table_entry.ridl.h"
   ]

   _public = \
   """
   void Update(int now, int interval);
   """
