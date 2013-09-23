#ifndef _RADIANT_LIB_RPC_LUA_OBJECT_ROUTER_H
#define _RADIANT_LIB_RPC_LUA_OBJECT_ROUTER_H

#include "namespace.h"
#include "lua_router.h"
#include "dm/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class LuaObjectRouter : public LuaRouter {
public:
   LuaObjectRouter(lua::ScriptHost *s, dm::Store& store);

public:     // IRouter
   ReactorDeferredPtr Call(Function const& fn) override;

private:
   dm::Store&        store_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_OBJECT_ROUTER_H
