#include "pch.h"
#include "radiant_exceptions.h"
#include "lua_deferred.h"
#include "reactor_deferred.h"
#include "lua/script_host.h"
#include "function.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

LuaDeferredPtr LuaDeferred::Wrap(lua_State* L, std::string const& name, ReactorDeferredPtr d)
{
   if (d) {
      rpc::LuaDeferredPtr lua_deferred = std::make_shared<rpc::LuaDeferred>(name);

      d->Progress([L, lua_deferred](JSONNode const& n) {
         lua_deferred->Notify(lua::ScriptHost::JsonToLua(L, n));
      });
      d->Done([L, lua_deferred, d](JSONNode const& n) {
         lua_deferred->Resolve(lua::ScriptHost::JsonToLua(L, n));
      });
      d->Fail([L, lua_deferred, d](JSONNode const& n) {
         lua_deferred->Reject(lua::ScriptHost::JsonToLua(L, n));
      });
      return lua_deferred;
   }
   return nullptr;
}
