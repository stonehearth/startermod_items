#include "../pch.h"
#include "open.h"
#include "client/client.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/reactor_deferred.h"
#include "lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::client;
using namespace luabind;

object Client_TraceObject(lua_State* L, std::string const& uri, const char* reason)
{
   rpc::ReactorDeferredPtr d = Client::GetInstance().TraceObject(uri, reason);
   if (d) {
      rpc::LuaDeferredPtr result = std::make_shared<rpc::LuaDeferred>(BUILD_STRING(uri << " trace"));
      lua::ScriptHost* sh = lua::ScriptHost::GetScriptHost(L);

      d->Progress([sh, result](JSONNode const& n) {
         result->Notify(sh->JsonToLua(n));
      });
      d->Done([sh, result, d](JSONNode const& n) {
         result->Resolve(sh->JsonToLua(n));
      });
      d->Fail([sh, result, d](JSONNode const& n) {
         result->Reject(sh->JsonToLua(n));
      });
      return object(L, result);
   }
   return object();
}


void lua::client::open(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("client") [
            def("trace_object",        &Client_TraceObject)
         ]
      ]
   ];
}
