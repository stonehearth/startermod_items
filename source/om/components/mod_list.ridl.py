from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm

class DataStore(dm.Record):
   name = 'DataStore'

class ModList(Component):
   mods = dm.Map(std.string(), std.shared_ptr(DataStore()), singular_name='mod', add='define', remove=None, iterate='define')

   _includes = [ "om/components/data_store.ridl.h" ]

