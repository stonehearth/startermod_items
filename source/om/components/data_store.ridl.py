from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.luabind_types as luabind

class DataStore(dm.Record):
   controller = dm.Boxed(luabind.object, trace=None)
   model = dm.Boxed(luabind.object)
   
   _public = \
   """
   JSONNode GetJsonData() const;
   """

   _generate_construct_object = True
   
   _includes = [
      "lib/lua/bind.h"
   ]

   _private = \
   """
   mutable JSONNode           cached_json_;
   mutable dm::GenerationId   last_encode_;
   """

   _global_post = \
   """
   IMPLEMENT_DM_NOP(luabind::object)
   """