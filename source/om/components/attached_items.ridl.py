import ridl.ridl as ridl
import ridl.std_types as std
import ridl.dm_types as dm
import ridl.c_types as c
from ridl.om_types import *

class AttachedItems(Component):
   items = dm.Map(std.string(), EntityRef(), singular_name='item', add='declare', iterate='define', num=None)
   ContainsItem = ridl.Method(c.bool(), ('key', std.string().const.ref)).const
