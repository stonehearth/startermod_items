#include "radiant.h"
#include "lib/json/macros.h"
#include "lib/lua/register.h"
#include "lib/lua/dm/trace_wrapper.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "om/components/data_store.ridl.h"
#include "om/components/data_store_ref_wrapper.h"
#include "lua_data_store.h"
#include "dm/trace_categories.h"
#include <luabind/adopt_policy.hpp>
#include <luabind/dependency_policy.hpp>

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

IMPLEMENT_TRIVIAL_TOSTRING(lua::DataObject);
IMPLEMENT_TRIVIAL_TOSTRING(DataStoreRefWrapper);
DEFINE_INVALID_JSON_CONVERSION(std::shared_ptr<lua::DataObject>);

bool StrongDataStore_IsValid(DataStorePtr ds)
{
   return ds != nullptr;
}

static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
StrongDataStore_Trace(DataStorePtr ds, const char* reason, dm::TraceCategories category)
{
   if (ds) {
      auto trace = ds->TraceDataObject(reason, category);
      return std::make_shared<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>(trace);
   }
   return nullptr;
}

static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
StrongDataStore_TraceAsync(DataStorePtr ds, const char* reason)
{
   return StrongDataStore_Trace(ds, reason, dm::LUA_ASYNC_TRACES);
}

DataStorePtr
StrongDataStore_SetData(DataStorePtr ds, object data)
{
   if (ds) {
      ds->SetData(data);
   }
   return ds;
}

object
StrongDataStore_GetData(DataStorePtr ds)
{
   if (ds) {
      return ds->GetData();
   }
   return object();
}

int
StrongDataStore_GetDataObjectId(DataStorePtr ds)
{
   if (ds) {
      return ds->GetDataObjectBox().GetObjectId();
   }
   return 0;
}


static void
StrongDataStore_CallDataCb(lua_State* L, DataStorePtr ds, luabind::object cb)
{
   if (ds) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
      try {
         object safe_cb(cb_thread, cb);
         safe_cb(ds->GetData());
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(L, e);
      }
   }
}

void
StrongDataStore_ModifyData(lua_State* L, DataStorePtr ds, luabind::object cb)
{
   if (ds) {
      StrongDataStore_CallDataCb(L, ds, cb);
      ds->MarkDataChanged();
   }
}

void
StrongDataStore_Destroy(DataStorePtr ds)
{
   if (ds) {
      ds->Destroy();
   }
}

DataStorePtr
StrongDataStore_MarkChanged(DataStorePtr ds)
{
   if (ds) {
      ds->MarkDataChanged();
   }
   return ds;
}

DataStorePtr
StrongDataStore_SetController(DataStorePtr ds, luabind::object obj)
{
   if (ds) {
      ds->SetController(obj);
   }
   return ds;
}

luabind::object
StrongDataStore_CreateController(DataStorePtr ds, std::string const& name)
{
   luabind::object controller;

   if (ds) {
      controller = ds->CreateController(ds, "controllers", name);
   }
   return controller;
}

static void
StrongDataStore_Restore(DataStoreRef data_store)
{
   auto ds = data_store.lock();
   if (ds) {
      ds->RestoreController(ds);
      ds->RestoreControllerData();
      ds->RemoveKeepAliveReferences();
   }
}


// -- weak versions --

bool WeakDataStore_IsValid(DataStoreRefWrapper ds)
{
   return StrongDataStore_IsValid(ds.lock());
}

static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
WeakDataStore_Trace(DataStoreRefWrapper ds, const char* reason, dm::TraceCategories category)
{
   return StrongDataStore_Trace(ds.lock(), reason, category);
}

static std::shared_ptr<lua::BoxedTraceWrapper<dm::BoxedTrace<dm::Boxed<lua::DataObject>>>>
WeakDataStore_TraceAsync(DataStoreRefWrapper ds, const char* reason)
{
   return StrongDataStore_TraceAsync(ds.lock(), reason);
}

DataStoreRefWrapper
WeakDataStore_SetData(DataStoreRefWrapper ds, object data)
{
   return StrongDataStore_SetData(ds.lock(), data);
}

object
WeakDataStore_GetData(DataStoreRefWrapper ds)
{
   return StrongDataStore_GetData(ds.lock());
}

int
WeakDataStore_GetDataObjectId(DataStoreRefWrapper ds)
{
   return StrongDataStore_GetDataObjectId(ds.lock());
}


static void
WeakDataStore_CallDataCb(lua_State* L, DataStoreRefWrapper ds, luabind::object cb)
{
   return StrongDataStore_CallDataCb(L, ds.lock(), cb);
}

void
WeakDataStore_ModifyData(lua_State* L, DataStoreRefWrapper ds, luabind::object cb)
{
   return StrongDataStore_ModifyData(L, ds.lock(), cb);
}

void
WeakDataStore_Destroy(DataStoreRefWrapper ds)
{
   return StrongDataStore_Destroy(ds.lock());
}

DataStoreRefWrapper
WeakDataStore_MarkChanged(DataStoreRefWrapper ds)
{
   return StrongDataStore_MarkChanged(ds.lock());
}

DataStoreRefWrapper
WeakDataStore_SetController(DataStoreRefWrapper ds, luabind::object obj)
{
   return StrongDataStore_SetController(ds.lock(), obj);
}

luabind::object
WeakDataStore_CreateController(DataStoreRefWrapper ds, std::string const& name)
{
   return StrongDataStore_CreateController(ds.lock(), name);
}

std::string WeakDataStore_ToJson(DataStoreRefWrapper ds)
{
   return lua::StrongGameObjectToJson<DataStore>(ds.lock());
}

static dm::ObjectId WeakDataStore_GetObjectId(DataStoreRefWrapper ds)
{
   return lua::StrongGetObjectId<DataStore>(ds.lock());
}

std::string WeakDataStore_GetObjectAddress(DataStoreRefWrapper ds)
{
   return lua::StrongGetObjectAddress<DataStore>(ds.lock());
}


static std::shared_ptr<lua::TraceWrapper>
WeakDataStore_TraceGameObject(DataStoreRefWrapper ds, const char* reason, dm::TraceCategories c)
{
   return lua::StrongTraceGameObject<DataStore>(ds.lock(), reason, c);
}

static std::shared_ptr<lua::TraceWrapper>
WeakDataStore_TraceGameObjectAsync(DataStoreRefWrapper ds, const char* reason)
{
   return lua::StrongTraceGameObject<DataStore>(ds.lock(), reason, dm::LUA_ASYNC_TRACES);
}

static void
WeakDataStore_Restore(DataStoreRefWrapper ds)
{
   return StrongDataStore_Restore(ds.lock());
}

scope LuaDataStore::RegisterLuaTypes(lua_State* L)
{
   // This is pretty unfortunate.  We need to register the same interface twice: once for
   // the std::shared_ptr DataStore and another for the DataStoreRefWrapper version (which is
   // really just a wrapper around the std::weak_ptr version which luabind cannot coerse to
   // the std::shared_ptr one).  Unfortunately, register.h can't handle this, so we need to
   // copy-pasta a lot of the implementation into the DataStoreRefWrapper one.  If this
   // becomes a "thing", of course we should make register.h smart enough to do the right
   // thing. -tony
   return
      lua::RegisterTypePtr<lua::DataObject>("DataObject")
      ,
      lua::RegisterStrongGameObject<DataStore>(L, "DataStore")
         .def("is_valid",       StrongDataStore_IsValid) // xxx: don't we need dependency(_1, _2) here?
         .def("get_data_object_id", StrongDataStore_GetDataObjectId) // xxx: don't we need dependency(_1, _2) here?
         .def("set",            StrongDataStore_SetData) // xxx: don't we need to adopt(_2) here?
         .def("set_data",       StrongDataStore_SetData) // xxx: don't we need to adopt(_2) here?
         .def("get",            StrongDataStore_GetData) // xxx: don't we need dependency(_1, _2) here?
         .def("get_data",       StrongDataStore_GetData) // xxx: don't we need dependency(_1, _2) here?
         .def("modify",         StrongDataStore_ModifyData) // xxx: don't we need dependency(_1, _2) here?
         .def("modify_data",    StrongDataStore_ModifyData) // xxx: don't we need dependency(_1, _2) here?
         .def("trace_data",     StrongDataStore_Trace)
         .def("trace_data",     StrongDataStore_TraceAsync)
         .def("restore",        StrongDataStore_Restore)
         .def("set_controller", StrongDataStore_SetController)
         .def("create_controller", StrongDataStore_CreateController)
         .def("destroy",        StrongDataStore_Destroy)
         .def("mark_changed",   StrongDataStore_MarkChanged)
      ,
      luabind::class_<DataStoreRefWrapper>("DataStoreRefWrapper")
         .def(tostring(luabind::const_self))
         .def(luabind::self == luabind::self)
         .def("is_valid",       &WeakDataStore_IsValid)
         .def("__get_userdata_type_id", &typeinfo::Type<DataStoreRefWrapper>::GetTypeId)
         .def("__tojson",       &WeakDataStore_ToJson)
         .def("get_id",         &WeakDataStore_GetObjectId)
         .def("get_address",    &WeakDataStore_GetObjectAddress)
         .def("get_type_id",    &lua::GetTypeHashCode<DataStoreRefWrapper>)
         .def("get_type_name",  &GetTypeName<DataStoreRefWrapper>)
         .def("serialize",      &WeakDataStore_ToJson)
         .def("trace",          &WeakDataStore_TraceGameObject)
         .def("trace",          &WeakDataStore_TraceGameObjectAsync)
         .def("get_data_object_id", WeakDataStore_GetDataObjectId) // xxx: don't we need dependency(_1, _2) here?
         .def("set",            WeakDataStore_SetData) // xxx: don't we need to adopt(_2) here?
         .def("set_data",       WeakDataStore_SetData) // xxx: don't we need to adopt(_2) here?
         .def("get",            WeakDataStore_GetData) // xxx: don't we need dependency(_1, _2) here?
         .def("get_data",       WeakDataStore_GetData) // xxx: don't we need dependency(_1, _2) here?
         .def("modify",         WeakDataStore_ModifyData) // xxx: don't we need dependency(_1, _2) here?
         .def("modify_data",    WeakDataStore_ModifyData) // xxx: don't we need dependency(_1, _2) here?
         .def("trace_data",     WeakDataStore_Trace)
         .def("trace_data",     WeakDataStore_TraceAsync)
         .def("restore",        WeakDataStore_Restore)
         .def("set_controller", WeakDataStore_SetController)
         .def("create_controller", WeakDataStore_CreateController)
         .def("destroy",        WeakDataStore_Destroy)
         .def("mark_changed",   WeakDataStore_MarkChanged)
         .scope [
            def("get_type_id",   &lua::GetClassTypeId<DataStoreRefWrapper>),
            def("get_type_name", &lua::GetStaticTypeName<DataStoreRefWrapper>)
         ];
}
