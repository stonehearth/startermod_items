#ifndef _RADIANT_LUA_DM_OPEN_H
#define _RADIANT_LUA_DM_OPEN_H

#include "dm/dm.h"
#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

namespace dm {
   void open(lua_State* lua);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_OPEN_H