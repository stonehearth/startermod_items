#include "../pch.h"
#include "open.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/session.h"
#include "lib/rpc/function.h"
#include "lib/rpc/core_reactor.h"
#include "lua/script_host.h"
#include "lua/register.h"

using namespace ::radiant;
using namespace ::radiant::rpc;
using namespace luabind;

LuaDeferredPtr LuaDeferred_Done(lua_State* L, LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Done([L, cb](luabind::object result) {
         call_function<void>(cb, result);
      });
   }
   return deferred;
}

LuaDeferredPtr LuaDeferred_Progress(lua_State* L, LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Progress([L, cb](luabind::object result) {
         call_function<void>(cb, result);
      });
   }
   return deferred;
}

LuaDeferredPtr LuaDeferred_Fail(lua_State* L, LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Fail([L, cb](luabind::object result) {
         call_function<void>(cb, result);
      });
   }
   return deferred;
}

CoreReactor* GetReactor(lua_State* L)
{
   CoreReactor* reactor = object_cast<CoreReactor*>(globals(L)["_reactor"]);
   if (!reactor) {
      throw std::logic_error("could not find reactor in interpreter");
   }
   return reactor;
}

LuaDeferredPtr call_impl(lua_State* L, int start, std::string const& obj, std::string const& route)
{
   int top = lua_gettop(L);

   JSONNode args;
   for (int i = start; i < top; i++) {
      std::string arg = lua::ScriptHost::LuaToJson(L, object(L, from_stack(L, i)));
      args.push_back(libjson::parse(arg));
   }
   Function fn;
   fn.object = obj;
   fn.route = route;
   fn.args = args;

   std::string name = BUILD_STRING("lua " << fn);
   ReactorDeferredPtr d = GetReactor(L)->Call(fn);
   return LuaDeferred::Wrap(L, name, d);
}

LuaDeferredPtr call(lua_State* L)
{
   return call_impl(L, 2, "", luaL_checkstring(L, 1));
}

LuaDeferredPtr call_obj(lua_State* L)
{
   return call_impl(L, 3, luaL_checkstring(L, 1), luaL_checkstring(L, 2));
}

IMPLEMENT_TRIVIAL_TOSTRING(CoreReactor);

void lua::rpc::open(lua_State* L, CoreReactorPtr reactor)
{
   module(L) [
      namespace_("_radiant") [
         def("call",             &call),
         def("call_obj",         &call_obj),
         namespace_("rpc") [
            lua::RegisterType<CoreReactor>(),
            lua::RegisterTypePtr<LuaDeferred>()
               .def("done",       &LuaDeferred_Done)
               .def("fail",       &LuaDeferred_Fail)
               .def("progress",   &LuaDeferred_Progress),
            lua::RegisterTypePtr<Session>()
               .def_readonly("faction", &Session::faction)
         ]
      ]
   ];
   globals(L)["_reactor"] = object(L, reactor.get());
}
