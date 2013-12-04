from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class EntityContainer(Component):
   children = dm.Map(dm.ObjectId(), EntityRef(), singular_name='child', add=None, remove='declare', iterate='define')
   add_child = ridl.Method(c.void(), ('child', EntityRef()))
