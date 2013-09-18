#include "../pch.h"
#include "open.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/session.h"
#include "lua/script_host.h"
#include "lua/register.h"

using namespace ::radiant;
using namespace ::radiant::rpc;
using namespace luabind;

rpc::LuaDeferredPtr LuaDeferred_Done(lua_State* L, rpc::LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Done([L, cb](luabind::object result) {
         call_function<void>(cb, result);
      });
   }
   return deferred;
}

rpc::LuaDeferredPtr LuaDeferred_Progress(lua_State* L, rpc::LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Progress([L, cb](luabind::object result) {
         call_function<void>(cb, result);
      });
   }
   return deferred;
}

rpc::LuaDeferredPtr LuaDeferred_Fail(lua_State* L, rpc::LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Fail([L, cb](luabind::object result) {
         call_function<void>(cb, result);
      });
   }
   return deferred;
}

void lua::rpc::open(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("rpc") [
            lua::RegisterTypePtr<radiant::rpc::LuaDeferred>()
               .def("done",       &LuaDeferred_Done)
               .def("fail",       &LuaDeferred_Fail)
               .def("progress",   &LuaDeferred_Progress),
            lua::RegisterTypePtr<radiant::rpc::Session>()
               .def_readonly("faction", &radiant::rpc::Session::faction)
         ]
      ]
   ];
}
