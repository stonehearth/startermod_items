#include "pch.h"
#include "lib/json/macros.h"
#include "lib/lua/register.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "om/components/data_store.ridl.h"
#include "lua_data_store.h"
#include <luabind/adopt_policy.hpp>
#include <luabind/dependency_policy.hpp>

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

IMPLEMENT_TRIVIAL_TOSTRING(lua::DataObject);
DEFINE_INVALID_JSON_CONVERSION(lua::DataObject);

static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
DataStore_Trace(std::shared_ptr<DataStore> data_store, const char* reason)
{
   if (data_store) {
      auto trace = data_store->TraceData(reason, dm::LUA_ASYNC_TRACES);
      return std::make_shared<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>(trace);
   }
   return nullptr;
}

DataStorePtr
DataStore_SetData(std::shared_ptr<DataStore> data_store, object data)
{
   if (data_store) {
      data_store->SetData(data);
   }
   return data_store;
}


DataStorePtr
DataStore_SetController(std::shared_ptr<DataStore> data_store, object controller)
{
   if (data_store) {
      data_store->SetController(lua::ControllerObject(controller));
   }
   return data_store;
}

object
DataStore_GetData(std::shared_ptr<DataStore> data_store)
{
   if (data_store) {
      return data_store->GetData();
   }
   return object();
}

DataStorePtr
DataStore_MarkChanged(std::shared_ptr<DataStore> data_store)
{
   if (data_store) {
      data_store->MarkDataChanged();
   }
   return data_store;
}

scope LuaDataStore::RegisterLuaTypes(lua_State* L)
{
   return
      // references to DataStore's are used where lua should not be able to keep objects alive, e.g. component data
      lua::RegisterStrongGameObject<DataStore>()
         .def("update",         &DataStore_SetData) // xxx: don't we need to adopt(_2) here?
         .def("get_data",       &DataStore_GetData) // xxx: don't we need dependency(_1, _2) here?
         .def("trace_data",     &DataStore_Trace)
         .def("mark_changed",   &DataStore_MarkChanged)
         .def("set_controller", &DataStore_SetController)
      ,
      luabind::class_<lua::DataObject, std::shared_ptr<lua::DataObject>>(GetShortTypeName<lua::DataObject>())
      ;
}
