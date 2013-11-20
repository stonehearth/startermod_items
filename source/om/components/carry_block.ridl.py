import ridl.ridl as ridl
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class CarryBlock(Component):
   carrying = dm.Boxed(EntityRef())
