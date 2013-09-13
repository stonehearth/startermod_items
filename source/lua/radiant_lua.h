#ifndef _RADIANT_LUA_LUA_H
#define _RADIANT_LUA_LUA_H

#include "namespace.h"
#include "radiant_luabind.h"

class JSONNode;

BEGIN_RADIANT_LUA_NAMESPACE

luabind::object JsonToLua(lua_State* L, JSONNode const& json);

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_LUA_H