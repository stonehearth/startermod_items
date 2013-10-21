#ifndef _RADIANT_LIB_RPC_LUA_ROUTER_H
#define _RADIANT_LIB_RPC_LUA_ROUTER_H

#include "namespace.h"
#include "radiant_luabind.h"
#include "irouter.h"
#include "lib/rpc/forward_defines.h"
#include "lua/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class LuaRouter : public IRouter {
public:
   LuaRouter(lua::ScriptHost* s);

protected:
   void CallLuaMethod(ReactorDeferredPtr d, luabind::object obj, luabind::object method, Function const& fn);
   lua::ScriptHost* GetScriptHost() { return scriptHost_; }

private:
   lua_State*        L_;
   lua::ScriptHost*  scriptHost_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_ROUTER_H
