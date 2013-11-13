#ifndef _RADIANT_CSG_LUA_LUA_REGION_H
#define _RADIANT_CSG_LUA_LUA_REGION_H

#include "lib/lua/bind.h"
#include "csg/namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

struct LuaRegion {
   static luabind::scope RegisterLuaTypes(lua_State* L);
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_LUA_LUA_REGION_H
