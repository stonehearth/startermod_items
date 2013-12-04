from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class Clock(Component):
   _generate_construct_object = True

   time = dm.Boxed(c.int())
