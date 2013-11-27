import ridl.ridl as ridl
import ridl.std_types as std
import ridl.dm_types as dm
import ridl.c_types as c
from ridl.om_types import *

class CarryBlock(Component):
   carrying = dm.Boxed(EntityRef())
   is_carrying = ridl.Method(c.bool()).const
