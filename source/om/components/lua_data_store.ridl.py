from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.luabind_types as luabind

class DataStore(dm.Record):
   controller = dm.Boxed(luabind.object, trace=None)
   model = dm.Boxed(luabind.object)
   
   _public_metehods = """
   JSONNode GetJsonData() const;
   """

   _private = """
   mutable JSONNode           cached_json_;
   mutable dm::GenerationId   last_encode_;
   """
