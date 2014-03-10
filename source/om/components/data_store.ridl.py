from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.lua_types as lua

class DataStore(dm.Record):
   controller = dm.Boxed(lua.ControllerObject())
   data = dm.Boxed(lua.DataObject(), get=None, set=None)

   _no_lua = True
   _public = \
   """
   void MarkDataChanged();
   luabind::object GetData() const;
   void SetData(luabind::object o);
   """

   _includes = [
      "dm/record.h",
      "lib/lua/controller_object.h",
      "lib/lua/data_object.h",
   ]
