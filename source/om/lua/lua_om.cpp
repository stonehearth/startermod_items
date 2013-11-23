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

DEFINE_INVALID_JSON_CONVERSION(om::Region3BoxedPtrBoxed)

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
            LuaDataStore::RegisterLuaTypes(L),
         ]
      ]
   ];
}
