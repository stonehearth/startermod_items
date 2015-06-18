from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.lua_types as lua

class DataStore(dm.Record):
   controller_type = dm.Boxed(std.string())
   controller_name = dm.Boxed(std.string())
   flags = dm.Boxed(c.int())
   destroyed = dm.Boxed(c.bool())
   data_object = dm.Boxed(lua.DataObject(), get=None, set=None)

   _no_lua = True
   _declare_constructor = True
   _generate_construct_object = True

   _public = \
   """
   enum DataStoreFlags {
      IS_CPP_MANAGED = (1 << 0)
   };

   void MarkDataChanged();
   luabind::object GetController() const;
   luabind::object GetData() const;
   void SetData(luabind::object o);
   void RestoreController(DataStorePtr self, int flags);
   void RestoreControllerData();
   void RemoveKeepAliveReferences();
   luabind::object CreateController(DataStorePtr self, std::string const& type, std::string const& alias, int flags);
   void SetController(luabind::object controller);
   void OnLoadObject(dm::SerializationType r) override;
   dm::Boxed<lua::DataObject> const& GetDataObjectBox() const;

   static int LuaDestructorHook(lua_State* L);
   void PatchLuaDestructor();
   void Destroy();
   void CallLuaDestructor();
   bool IsDestroyed() const;
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
   dm::TracePtr _dataObjTrace;
   """

   _includes = [
      "dm/record.h",
      "lib/lua/data_object.h",
   ]
