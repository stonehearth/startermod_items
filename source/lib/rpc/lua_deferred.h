#ifndef _RADIANT_LIB_RPC_LUA_DEFERRED_H
#define _RADIANT_LIB_RPC_LUA_DEFERRED_H

#include "namespace.h"
#include "core/deferred.h"
#include "lib/rpc/forward_defines.h"
#include "lib/lua/bind.h"

struct lua_State;

BEGIN_RADIANT_RPC_NAMESPACE

class LuaDeferred : public core::Deferred<luabind::object, luabind::object>
{
public:
   LuaDeferred(std::string const& dbg_name) : core::Deferred<luabind::object, luabind::object>(dbg_name) {}
   virtual void Destroy() {}

public:
   static LuaDeferredPtr Wrap(lua_State* L, std::string const& name, ReactorDeferredPtr d);
};

template <typename T>
class LuaDeferredObject : public LuaDeferred
{
public:
   LuaDeferredObject(std::shared_ptr<T> obj, std::string const& dbg_name) :
      LuaDeferred(dbg_name),
      _obj(obj)
   {
   }
   void Destroy() override {
      _obj.reset();
   }

private:
   std::shared_ptr<T>   _obj;
};

DECLARE_SHARED_POINTER_TYPES(LuaDeferred)

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_LUA_DEFERRED_H
