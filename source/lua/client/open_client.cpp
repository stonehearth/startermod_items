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
      rpc::LuaDeferredPtr result = rpc::LuaDeferred::Wrap(L, BUILD_STRING(uri << " trace"), d);
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
