#ifndef _RADIANT_OM_LUA_DATA_STORE_H
#define _RADIANT_OM_LUA_DATA_STORE_H

#include "lib/lua/bind.h"
#include "om/namespace.h"

BEGIN_RADIANT_OM_NAMESPACE

struct LuaDataStore {
   static luabind::scope RegisterLuaTypes(lua_State* L);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
