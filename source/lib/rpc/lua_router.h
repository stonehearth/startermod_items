#ifndef _RADIANT_LIB_RPC_LUA_ROUTER_H
#define _RADIANT_LIB_RPC_LUA_ROUTER_H

#include "namespace.h"
#include "radiant_json.h"
#include "irouter.h"
#include "lib/rpc/forward_defines.h"
#include "lua/forward_defines.h"

struct lua_State;

BEGIN_RADIANT_RPC_NAMESPACE

class LuaRouter : public IRouter {
public:
   LuaRouter(lua_State* L, std::string const& endpoint);

public:     // IRouter
   ReactorDeferredPtr Call(Function const& fn) override;

private:
   ReactorDeferredPtr CallModuleFunction(Function const& fn, std::string const& script);

private:
   lua_State*        L_;
   std::string       endpoint_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_ROUTER_H
