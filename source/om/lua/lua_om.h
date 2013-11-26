#ifndef _RADIANT_OM_LUA_OM_H
#define _RADIANT_OM_LUA_OM_H

#include "lib/lua/bind.h"
#include "om/region.h"
#include <ostream>

BEGIN_RADIANT_OM_NAMESPACE

void RegisterLuaTypes(lua_State* L);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
