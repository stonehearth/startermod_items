#ifndef _RADIANT_LIB_RPC_LUA_MODULE_ROUTER_H
#define _RADIANT_LIB_RPC_LUA_MODULE_ROUTER_H

#include "namespace.h"
#include "lua_router.h"
#include "resources/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class LuaModuleRouter : public LuaRouter {
public:
   LuaModuleRouter(lua::ScriptHost *s, std::string const& endpoint);

public:     // IRouter
   ReactorDeferredPtr Call(Function const& fn) override;

private:
   void CallModuleFunction(ReactorDeferredPtr d, Function const& fn, std::string const& script, std::string const& function_name);

private:
   lua_State*        L_;
   std::string       endpoint_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_MODULE_ROUTER_H
