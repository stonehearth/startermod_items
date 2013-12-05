from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std

class Item(Component):
   stacks = dm.Boxed(c.int())
   max_stacks = dm.Boxed(c.int())
   category = dm.Boxed(std.string())

   _generate_construct_object = True

