from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.lua_types as lua

class DataStore(dm.Record):
   controller_type = dm.Boxed(std.string())
   controller_name = dm.Boxed(std.string())
   data = dm.Boxed(lua.DataObject(), get=None, set=None)

   _no_lua = True
   _public = \
   """
   void MarkDataChanged();
   luabind::object GetController() const;
   luabind::object GetData() const;
   void SetData(luabind::object o);
   luabind::object RestoreController(DataStorePtr self);
   luabind::object CreateController(DataStorePtr self, std::string type, std::string const& alias);
   void SetController(luabind::object controller);
   void DestroyController();
   """

   _private = \
   """
   luabind::object RestoreControllerRecursive(DataStorePtr self, std::unordered_map<dm::ObjectId, luabind::object>& visited);
   void RestoreContainedDatastores(luabind::object o, std::unordered_map<dm::ObjectId, luabind::object>& visited);
   std::string GetControllerUri();
   luabind::object _controllerObject;
   """

   _includes = [
      "dm/record.h",
      "lib/lua/data_object.h",
   ]
