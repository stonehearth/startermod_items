#include "pch.h"
#include "lib/json/macros.h"
#include "lib/lua/register.h"
#include "lib/lua/dm/trace_wrapper.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "om/components/data_store.ridl.h"
#include "lua_data_store.h"
#include <luabind/adopt_policy.hpp>
#include <luabind/dependency_policy.hpp>

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

IMPLEMENT_TRIVIAL_TOSTRING(lua::DataObject);
DEFINE_INVALID_JSON_CONVERSION(std::shared_ptr<lua::DataObject>);

static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
DataStore_Trace(DataStorePtr data_store, const char* reason)
{
   if (data_store) {
      auto trace = data_store->TraceData(reason, dm::LUA_ASYNC_TRACES);
      return std::make_shared<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>(trace);
   }
   return nullptr;
}

DataStorePtr
DataStore_SetData(DataStorePtr data_store, object data)
{
   if (data_store) {
      data_store->SetData(data);
   }
   return data_store;
}

object
DataStore_GetData(DataStorePtr data_store)
{
   if (data_store) {
      return data_store->GetData();
   }
   return object();
}


static void
DataStore_CallDataCb(lua_State* L, DataStorePtr data_store, luabind::object cb)
{
   if (data_store) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
      try {
         object safe_cb(cb_thread, cb);
         safe_cb(data_store->GetData());
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(L, e);
      }
   }
}

void
DataStore_ModifyData(lua_State* L, DataStorePtr data_store, luabind::object cb)
{
   if (data_store) {
      DataStore_CallDataCb(L, data_store, cb);
      data_store->MarkDataChanged();
   }
}

void
DataStore_ReadData(lua_State* L, DataStorePtr data_store, luabind::object cb)
{
   if (data_store) {
      DataStore_CallDataCb(L, data_store, cb);
   }
}

DataStorePtr
DataStore_MarkChanged(DataStorePtr data_store)
{
   if (data_store) {
      data_store->MarkDataChanged();
   }
   return data_store;
}

DataStorePtr
DataStore_SetController(DataStorePtr data_store, luabind::object obj)
{
   if (data_store) {
      data_store->SetController(obj);
   }
   return data_store;
}

luabind::object
DataStore_CreateController(DataStorePtr data_store, std::string const& type, std::string const& name)
{
   luabind::object controller;
   if (data_store) {
      controller = data_store->CreateController(data_store, type, name);
   }
   return controller;
}

scope LuaDataStore::RegisterLuaTypes(lua_State* L)
{
   return
      // references to DataStore's are used where lua should not be able to keep objects alive, e.g. component data
      lua::RegisterStrongGameObject<DataStore>(L, "DataStore")
         .def("set_data",       &DataStore_SetData) // xxx: don't we need to adopt(_2) here?
         .def("get_data",       &DataStore_GetData) // xxx: don't we need dependency(_1, _2) here?
         .def("modify_data",    &DataStore_ModifyData) // xxx: don't we need dependency(_1, _2) here?
         .def("read_data",      &DataStore_ReadData) // xxx: don't we need dependency(_1, _2) here?
         .def("trace_data",     &DataStore_Trace)
         .def("set_controller", &DataStore_SetController)
         .def("create_controller", &DataStore_CreateController)
         .def("mark_changed",   &DataStore_MarkChanged)
      ,
      lua::RegisterTypePtr<lua::DataObject>("DataObject")
      ;
}
