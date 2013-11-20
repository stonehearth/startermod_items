#include "pch.h"
#include "lua_clock_component.h"
#include "lua_component.h"
#include "lua_effect_list_component.h"
#include "lua_entity.h"
#include "lua_entity_container_component.h"
#include "lua_mob_component.h"
#include "lua_region.h"
#include "lua_region_collision_shape_component.h"
#include "lua_render_info_component.h"
#include "lua_model_variants_component.h"
#include "lua_sensor_list_component.h"
#include "lua_target_tables_component.h"
#include "lua_terrain_component.h"
#include "lua_vertical_pathing_region_component.h"
#include "lua_lua_components_component.h"
#include "lua_destination_component.h"
#include "lua_unit_info_component.h"
#include "lua_item_component.h"
#include "lua_paperdoll_component.h"
#include "lua_carry_block_component.h"
#include "lua_data_binding.h"
#include "lib/lua/script_host.h"

#include "lib/json/core_json.h"
#include "lib/json/dm_json.h"

#include "dm/store.h"
#include "om/all_object_defs.h"
#include "om/all_component_defs.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

IMPLEMENT_TRIVIAL_TOSTRING(DeepRegionGuardLua)
DEFINE_INVALID_JSON_CONVERSION(DeepRegionGuardLua)
DEFINE_INVALID_JSON_CONVERSION(om::Region3BoxedPtrBoxed)

DeepRegionGuardLua::DeepRegionGuardLua(lua_State* L, Region3BoxedPtrBoxed const& bbrp, const char* reason)
{
   L_ = lua::ScriptHost::GetCallbackThread(L);
   region_guard_ = DeepTraceRegionVoid(bbrp, reason, [=]{
      FireTrace();
   });
}

DeepRegionGuardLuaPtr DeepRegionGuardLua::OnChanged(object cb)
{
   cbs_.push_back(object(L_, cb));
   return shared_from_this();
}

void DeepRegionGuardLua::Destroy()
{
   region_guard_ = nullptr;
   cbs_.clear();
}

void DeepRegionGuardLua::FireTrace()
{
   for (const auto& cb : cbs_) {
      try {
         call_function<void>(cb);
      } catch (std::exception &e) {
         LOG(WARNING) << "error in lua callback: " << e.what();
      }
   }
}

void radiant::om::RegisterLuaTypes(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("om") [
            LuaComponent_::RegisterLuaTypes(L),

#define OM_OBJECT(Cls, lower) Lua ## Cls ## Component::RegisterLuaTypes(L),
            OM_ALL_COMPONENTS
#undef OM_OBJECT
            LuaEntity::RegisterLuaTypes(L),
            LuaRegion::RegisterLuaTypes(L),
            LuaDataBinding::RegisterLuaTypes(L),

            lua::RegisterTypePtr<DeepRegionGuardLua>()
               .def("on_changed",               &DeepRegionGuardLua::OnChanged)
               .def("destroy",                  &DeepRegionGuardLua::Destroy)
         ]
      ]
   ];
}
