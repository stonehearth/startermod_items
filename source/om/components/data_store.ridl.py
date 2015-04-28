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
   void RestoreController(DataStorePtr self, bool isCppManaged = false);
   void RestoreControllerData();
   void RemoveKeepAliveReferences();
   luabind::object CreateController(DataStorePtr self, std::string const& type, std::string const& alias);
   void SetController(luabind::object controller);
   void OnLoadObject(dm::SerializationType r) override;
   dm::Boxed<lua::DataObject> const& GetDataObjectBox() const;

   static int LuaDestructorHook(lua_State* L);
   void PatchLuaDestructor();
   void Destroy();
   void CallLuaDestructor();
   """

   _private = \
   """
   void RestoreControllerDataRecursive(luabind::object o, luabind::object& visited);
   std::string GetControllerUri();
   bool GetIsCppManaged() const;
   luabind::object _weakTable;
   luabind::object _controllerKeepAliveObject;
   luabind::object _controllerDestructor;
   bool _needsRestoration;
   bool _isCppManaged;
   dm::TracePtr _dataObjTrace;
   """

   _includes = [
      "dm/record.h",
      "lib/lua/data_object.h",
   ]
