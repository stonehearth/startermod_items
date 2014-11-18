from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.lua_types as lua

class DataStore(dm.Record):
   controller_type = dm.Boxed(std.string())
   controller_name = dm.Boxed(std.string())
   data_object = dm.Boxed(lua.DataObject(), get=None, set=None)

   _no_lua = True
   _declare_constructor = True

   _public = \
   """
   void MarkDataChanged();
   luabind::object GetController() const;
   luabind::object GetData() const;
   void SetData(luabind::object o);
   void RestoreController(DataStoreRef self);
   void RestoreControllerData(std::vector<luabind::object>& visitedTables);
   luabind::object CreateController(DataStoreRef self, std::string const& type, std::string const& alias);
   void SetController(luabind::object controller);
   void DestroyController();
   void OnLoadObject(dm::SerializationType r) override;
   dm::Boxed<lua::DataObject> const& GetDataObjectBox() const;
   """

   _private = \
   """
   void RestoreControllerDataRecursive(luabind::object o, std::vector<luabind::object>& visitedTables);
   std::string GetControllerUri();
   luabind::object _controllerObject;
   bool _needsRestoration;
   dm::TracePtr _dataObjTrace;
   """

   _includes = [
      "dm/record.h",
      "lib/lua/data_object.h",
   ]
