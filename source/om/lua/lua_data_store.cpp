#include "radiant.h"
#include "lib/json/macros.h"
#include "lib/lua/register.h"
#include "lib/lua/dm/trace_wrapper.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "om/components/data_store.ridl.h"
#include "lua_data_store.h"
#include "dm/trace_categories.h"
#include <luabind/adopt_policy.hpp>
#include <luabind/dependency_policy.hpp>

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

IMPLEMENT_TRIVIAL_TOSTRING(lua::DataObject);
DEFINE_INVALID_JSON_CONVERSION(std::shared_ptr<lua::DataObject>);


static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
DataStore_Trace(DataStoreRef data_store, const char* reason, dm::TraceCategories category)
{
   auto ds = data_store.lock();
   if (ds) {
      auto trace = ds->TraceDataObject(reason, category);
      return std::make_shared<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>(trace);
   }
   return nullptr;
}

static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
DataStore_TraceAsync(DataStoreRef data_store, const char* reason)
{
   return DataStore_Trace(data_store, reason, dm::LUA_ASYNC_TRACES);
}

static void
DataStore_Restore(DataStoreRef data_store)
{
   auto ds = data_store.lock();
   if (ds) {
      std::vector<luabind::object> visitedTables;
      ds->RestoreController(ds);
      ds->RestoreControllerData(visitedTables);
   }
}

DataStoreRef
DataStore_SetData(DataStoreRef data_store, object data)
{
   auto ds = data_store.lock();
   if (ds) {
      ds->SetData(data);
   }
   return data_store;
}

object
DataStore_GetData(DataStoreRef data_store)
{
   auto ds = data_store.lock();
   if (ds) {
      return ds->GetData();
   }
   return object();
}

int
DataStore_GetDataObjectId(DataStoreRef data_store)
{
   auto ds = data_store.lock();
   if (ds) {
      return ds->GetDataObjectBox().GetObjectId();
   }
   return 0;
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
DataStore_ModifyData(lua_State* L, DataStoreRef data_store, luabind::object cb)
{
   auto ds = data_store.lock();
   if (ds) {
      DataStore_CallDataCb(L, ds, cb);
      ds->MarkDataChanged();
   }
}

void
DataStore_ReadData(lua_State* L, DataStoreRef data_store, luabind::object cb)
{
   auto ds = data_store.lock();
   if (ds) {
      DataStore_CallDataCb(L, ds, cb);
   }
}

DataStoreRef
DataStore_MarkChanged(DataStoreRef data_store)
{
   auto ds = data_store.lock();
   if (ds) {
      ds->MarkDataChanged();
   }
   return data_store;
}

DataStoreRef
DataStore_SetController(DataStoreRef data_store, luabind::object obj)
{
   auto ds = data_store.lock();
   if (ds) {
      ds->SetController(obj);
   }
   return data_store;
}

luabind::object
DataStore_CreateController(DataStoreRef data_store, std::string const& type, std::string const& name)
{
   auto ds = data_store.lock();
   luabind::object controller;
   if (ds) {
      controller = ds->CreateController(ds, type, name);
   }
   return controller;
}

void DataStore_DestroyController(DataStoreRef data_store)
{
   auto ds = data_store.lock();
   if (ds) {
      ds->DestroyController();
   }
}

luabind::object
DataStore_GetController(DataStoreRef data_store)
{
   luabind::object controller;
   auto ds = data_store.lock();
   if (ds) {
      controller = ds->GetController();
   }
   return controller;
}

scope LuaDataStore::RegisterLuaTypes(lua_State* L)
{
   return
      // references to DataStore's are used where lua should not be able to keep objects alive, e.g. component data
      lua::RegisterWeakGameObject<DataStore>(L, "DataStore")
         .def("set_data",       &DataStore_SetData) // xxx: don't we need to adopt(_2) here?
         .def("get_data",       &DataStore_GetData) // xxx: don't we need dependency(_1, _2) here?
         .def("get_data_object_id", &DataStore_GetDataObjectId) // xxx: don't we need dependency(_1, _2) here?
         .def("modify_data",    &DataStore_ModifyData) // xxx: don't we need dependency(_1, _2) here?
         .def("read_data",      &DataStore_ReadData) // xxx: don't we need dependency(_1, _2) here?
         .def("trace_data",     &DataStore_Trace)
         .def("trace_data",     &DataStore_TraceAsync)
         .def("restore",        &DataStore_Restore)
         .def("set_controller", &DataStore_SetController)
         .def("get_controller", &DataStore_GetController)
         .def("create_controller", &DataStore_CreateController)
         .def("destroy_controller", &DataStore_DestroyController)
         .def("mark_changed",   &DataStore_MarkChanged)
      ,
      lua::RegisterTypePtr<lua::DataObject>("DataObject")
      ;
}
