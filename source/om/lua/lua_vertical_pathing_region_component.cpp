#include "pch.h"
#include "lua/register.h"
#include "lua_vertical_pathing_region_component.h"
#include "om/components/vertical_pathing_region.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaVerticalPathingRegionComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<VerticalPathingRegion, Component>()
      ;
}
