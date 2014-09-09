#include "radiant.h"
#include "lua_om.h"
#include "lua_component.h"
#include "lua_entity.h"
#include "lua_region.h"
#include "lua_data_store.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "lib/lua/script_host.h"
#include "lib/lua/register.h"
#include "lib/json/core_json.h"
#include "lib/json/dm_json.h"

#include "dm/store.h"
#include "om/all_object_defs.h"
#include "om/all_component_defs.h"
#include "lib/lua/dm/boxed_trace_wrapper.h"
#include "lib/lua/dm/trace_wrapper.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

DEFINE_INVALID_JSON_CONVERSION(Region2BoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(Region3BoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(Region2fBoxedPtrBoxed)
DEFINE_INVALID_JSON_CONVERSION(Region3fBoxedPtrBoxed)

DEFINE_INVALID_JSON_CONVERSION(DeepRegion2Guard)
DEFINE_INVALID_JSON_CONVERSION(DeepRegion3Guard)
DEFINE_INVALID_JSON_CONVERSION(DeepRegion2fGuard)
DEFINE_INVALID_JSON_CONVERSION(DeepRegion3fGuard)

IMPLEMENT_TRIVIAL_TOSTRING(Region2Boxed)
IMPLEMENT_TRIVIAL_TOSTRING(Region3Boxed)
IMPLEMENT_TRIVIAL_TOSTRING(Region2fBoxed)
IMPLEMENT_TRIVIAL_TOSTRING(Region3fBoxed)

IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion2Guard)
IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion3Guard)
IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion2fGuard)
IMPLEMENT_TRIVIAL_TOSTRING(DeepRegion3fGuard)

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
            RegisterModelLayer(L),
            LuaEntity::RegisterLuaTypes(L),
            LuaDataStore::RegisterLuaTypes(L),
            lua::RegisterStrongGameObject<Region2Boxed>(L, "Region2Boxed")
               .def("get",       &Region2Boxed::Get)
               .def("modify",    &ModifyBoxed<Region2Boxed>)
            ,
            lua::RegisterStrongGameObject<Region2fBoxed>(L, "Region2fBoxed")
               .def("get",       &Region2fBoxed::Get)
               .def("modify",    &ModifyBoxed<Region2fBoxed>)
            ,
            lua::RegisterStrongGameObject<Region3Boxed>(L, "Region3Boxed")
               .def("get",       &Region3Boxed::Get)
               .def("modify",    &ModifyBoxed<Region3Boxed>)
            ,
            lua::RegisterStrongGameObject<Region3fBoxed>(L, "Region3fBoxed")
               .def("get",       &Region3fBoxed::Get)
               .def("modify",    &ModifyBoxed<Region3fBoxed>)
            ,
            luabind::class_<LuaDeepRegion2Guard, LuaDeepRegion2GuardPtr>("LuaDeepRegion2Guard")
               .def("on_changed",         &LuaDeepRegion2Guard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion2Guard::PushObjectState)
               .def("destroy",            &LuaDeepRegion2Guard::Destroy)
            ,
            luabind::class_<LuaDeepRegion2fGuard, LuaDeepRegion2fGuardPtr>("LuaDeepRegion2fGuard")
               .def("on_changed",         &LuaDeepRegion2fGuard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion2fGuard::PushObjectState)
               .def("destroy",            &LuaDeepRegion2fGuard::Destroy)
            ,
            luabind::class_<LuaDeepRegion3Guard, LuaDeepRegion3GuardPtr>("LuaDeepRegion3Guard")
               .def("on_changed",         &LuaDeepRegion3Guard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion3Guard::PushObjectState)
               .def("destroy",            &LuaDeepRegion3Guard::Destroy)
            ,
            luabind::class_<LuaDeepRegion3fGuard, LuaDeepRegion3fGuardPtr>("LuaDeepRegion3fGuard")
               .def("on_changed",         &LuaDeepRegion3fGuard::OnChanged)
               .def("push_object_state",  &LuaDeepRegion3fGuard::PushObjectState)
               .def("destroy",            &LuaDeepRegion3fGuard::Destroy)
         ]
      ]
   ];
}
