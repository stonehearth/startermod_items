#ifndef _RADIANT_CSG_LUA_LUA_ITERATOR_H
#define _RADIANT_CSG_LUA_LUA_ITERATOR_H

#include "lib/lua/bind.h"
#include "csg/namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

luabind::scope RegisterIteratorTypes(lua_State* L);

void EachPointRect2f(lua_State *L, Rect2f const& cube);
void EachPointCube3f(lua_State *L, Cube3f const& cube);
void EachPointRegion3f(lua_State *L, Region3f const& region);
void EachPointRegion2f(lua_State *L, Region2f const& region);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_LUA_LUA_ITERATOR_H