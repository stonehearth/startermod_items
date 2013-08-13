#ifndef _RADIANT_CSG_LUA_LUA_CUBE_H
#define _RADIANT_CSG_LUA_LUA_CUBE_H

#include "radiant_luabind.h"
#include "csg/namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

struct LuaCube {
   static luabind::scope RegisterLuaTypes(lua_State* L);
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_LUA_LUA_CUBE_H