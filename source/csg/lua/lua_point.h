#ifndef _RADIANT_CSG_LUA_LUA_POINT_H
#define _RADIANT_CSG_LUA_LUA_POINT_H

#include "radiant_luabind.h"
#include "csg/namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

struct LuaPoint {
   static luabind::scope RegisterLuaTypes(lua_State* L);
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_LUA_LUA_POINT_H