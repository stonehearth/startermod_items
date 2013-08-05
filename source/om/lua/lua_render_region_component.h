#ifndef _RADIANT_OM_LUA_RENDER_REGION_COMPONENT_H
#define _RADIANT_OM_LUA_RENDER_REGION_COMPONENT_H

#include "radiant_luabind.h"
#include "om/namespace.h"

BEGIN_RADIANT_OM_NAMESPACE

struct LuaRenderRegionComponent {
   static luabind::scope RegisterLuaTypes(lua_State* L);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
