#include "pch.h"
#include "lua_attributes_component.h"
#include "lua_aura_list_component.h"
#include "lua_clock_component.h"
#include "lua_component.h"
#include "lua_effect_list_component.h"
#include "lua_entity.h"
#include "lua_entity_container_component.h"
#include "lua_mob_component.h"
#include "lua_region.h"
#include "lua_region_collision_shape_component.h"
#include "lua_render_info_component.h"
#include "lua_render_rig_component.h"
#include "lua_render_rig_iconic_component.h"
#include "lua_sensor_list_component.h"
#include "lua_target_tables_component.h"
#include "lua_sphere_collision_shape_component.h"
#include "lua_terrain_component.h"
#include "lua_vertical_pathing_region_component.h"
#include "lua_lua_components_component.h"
#include "lua_destination_component.h"
#include "lua_render_region_component.h"
#include "lua_unit_info_component.h"
#include "lua_item_component.h"
#include "lua_paperdoll_component.h"
#include "lua_carry_block_component.h"

#include "om/all_object_defs.h"
#include "om/all_component_defs.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

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
            LuaRegion::RegisterLuaTypes(L)
         ]
      ]
   ];
}
