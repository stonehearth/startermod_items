#ifndef _RADIANT_OM_LUA_SENSOR_LIST_COMPONENT_H
#define _RADIANT_OM_LUA_SENSOR_LIST_COMPONENT_H

#include "lib/lua/bind.h"
#include "om/namespace.h"

BEGIN_RADIANT_OM_NAMESPACE

struct LuaSensorListComponent {
   static luabind::scope RegisterLuaTypes(lua_State* L);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
