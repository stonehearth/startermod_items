#ifndef _RADIANT_LUA_CLIENT_OPEN_H
#define _RADIANT_LUA_CLIENT_OPEN_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

namespace client {
   void open(lua_State* lua);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_CLIENT_OPEN_H