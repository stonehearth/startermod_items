#ifndef _RADIANT_LIB_RPC_LUA_DEFERRED_H
#define _RADIANT_LIB_RPC_LUA_DEFERRED_H

#include "namespace.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/rpc/forward_defines.h"
#include "lib/lua/bind.h"

struct lua_State;

BEGIN_RADIANT_RPC_NAMESPACE

class LuaFuture : public core::Deferred<luabind::object, luabind::object>
{
public:
   LuaFuture(lua_State* L, rpc::ReactorDeferredPtr d, std::string const& dbgName);
   ~LuaFuture();

   void Destroy();

private:
   rpc::ReactorDeferredPtr    _future;
   std::string                _dbgName;
};

class LuaPromise : public core::Deferred<luabind::object, luabind::object>,
                   public std::enable_shared_from_this<LuaPromise>
{
public:
   static LuaPromisePtr Create(lua_State* L, rpc::ReactorDeferredPtr d, std::string const& dbgName);
   ~LuaPromise();

   void Destroy();
   
private:
   LuaPromise(rpc::ReactorDeferredPtr d, std::string const& dbgName);
   void Initialize(lua_State* L);

private:
   rpc::ReactorDeferredPtr    _promise;
   std::string                _dbgName;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_LUA_DEFERRED_H
