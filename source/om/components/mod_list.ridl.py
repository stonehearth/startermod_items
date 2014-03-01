from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm
import ridl.luabind_types as luabind

class DataStore(dm.Record):
   name = 'DataStore'

class ModList(Component):
   mods = dm.Map(std.string(), luabind.object(), singular_name='mod', add='define', remove=None, iterate='define')

