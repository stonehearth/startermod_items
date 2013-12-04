import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class TargetTableEntry(dm.Record):
   target = dm.Boxed(EntityRef())
   value = dm.Boxed(c.int())
   expire_time = dm.Boxed(c.int())

   _generate_construct_object = True
   
   _public = \
   """
   bool Update(int now, int interval);
   """
