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
      rpc::LuaDeferredPtr result = std::make_shared<rpc::LuaDeferred>(name);

      d->Progress([L, result](JSONNode const& n) {
         result->Notify(lua::ScriptHost::JsonToLua(L, n));
      });
      d->Done([L, result, d](JSONNode const& n) {
         result->Resolve(lua::ScriptHost::JsonToLua(L, n));
      });
      d->Fail([L, result, d](JSONNode const& n) {
         result->Reject(lua::ScriptHost::JsonToLua(L, n));
      });
      return result;
   }
   return nullptr;
}
