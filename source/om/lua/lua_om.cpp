#include "pch.h"
#include "lua_component.h"
#include "lua_entity.h"
#include "lua_region.h"
#include "lua_data_store.h"
#include "lib/lua/script_host.h"
#include "lib/lua/register.h"
#include "lib/json/core_json.h"
#include "lib/json/dm_json.h"

#include "dm/store.h"
#include "om/all_object_defs.h"
#include "om/all_component_defs.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

DEFINE_INVALID_JSON_CONVERSION(Region2BoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(Region3BoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(DeepRegionGuard)
IMPLEMENT_TRIVIAL_TOSTRING(Region2Boxed)
IMPLEMENT_TRIVIAL_TOSTRING(Region3Boxed)
IMPLEMENT_TRIVIAL_TOSTRING(DeepRegionGuard)

template <typename Boxed>
static void ModifyBoxed(Boxed& boxed, luabind::object cb)
{
   boxed.Modify([cb](typename Boxed::Value& value) {
      try {
         call_function<void>(cb, &value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "error modifying boxed object: " << e.what();
      }
   });
}

#define OM_OBJECT(Cls, cls) scope Register ## Cls(lua_State* L);
OM_ALL_COMPONENTS
#undef OM_OBJECT
scope RegisterEffect(lua_State* L);
scope RegisterSensor(lua_State* L);
scope RegisterTargetTable(lua_State* L);
scope RegisterTargetTableGroup(lua_State* L);
scope RegisterTargetTableEntry(lua_State* L);
scope RegisterModelLayer(lua_State* L);

scope RegisterLuaComponents(lua_State *L)
{
   return scope();
}

void radiant::om::RegisterLuaTypes(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("om") [
            LuaComponent_::RegisterLuaTypes(L),
#define OM_OBJECT(Cls, cls) Register ## Cls(L),
            OM_ALL_COMPONENTS
#undef OM_OBJECT
            RegisterSensor(L),
            RegisterEffect(L),
            RegisterTargetTable(L),
            RegisterTargetTableGroup(L),
            RegisterTargetTableEntry(L),
            RegisterModelLayer(L),
            LuaEntity::RegisterLuaTypes(L),
            LuaDataStore::RegisterLuaTypes(L),
            lua::RegisterStrongGameObject<Region2Boxed>(L, "Region2Boxed")
               .def("get",       &Region2Boxed::Get)
               .def("modify",    &ModifyBoxed<Region2Boxed>)
            ,
            lua::RegisterStrongGameObject<Region3Boxed>(L, "Region3Boxed")
               .def("get",       &Region3Boxed::Get)
               .def("modify",    &ModifyBoxed<Region3Boxed>)
            ,
            luabind::class_<LuaDeepRegionGuard, std::shared_ptr<LuaDeepRegionGuard>>("LuaDeepRegionGuard")
               .def("on_changed",         &LuaDeepRegionGuard::OnChanged)
               .def("push_object_state",  &LuaDeepRegionGuard::PushObjectState)
               .def("destroy",            &LuaDeepRegionGuard::Destroy)
         ]
      ]
   ];
}
