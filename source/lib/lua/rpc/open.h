#ifndef _RADIANT_LUA_RPC_OPEN_H
#define _RADIANT_LUA_RPC_OPEN_H

#include "lib/lua/lua.h"
#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_LUA_NAMESPACE

namespace rpc {
   void open(lua_State* lua, radiant::rpc::CoreReactorPtr reactor);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_RPC_OPEN_H