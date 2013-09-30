#include "../pch.h"
#include "open.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/session.h"
#include "lib/rpc/function.h"
#include "lib/rpc/trace.h"
#include "lib/rpc/untrace.h"
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
         call_function<void>(object(L, cb), result);
      });
   }
   return deferred;
}

LuaDeferredPtr LuaDeferred_Always(lua_State* L, LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Always([L, cb]() {
         call_function<void>(object(L, cb));
      });
   }
   return deferred;
}

LuaDeferredPtr LuaDeferred_Progress(lua_State* L, LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Progress([L, cb](luabind::object result) {
         call_function<void>(object(L, cb), result);
      });
   }
   return deferred;
}

LuaDeferredPtr LuaDeferred_Fail(lua_State* L, LuaDeferredPtr deferred, object cb)
{
   L = lua::ScriptHost::GetCallbackThread(L);
   if (deferred) {
      deferred->Fail([L, cb](luabind::object result) {
         call_function<void>(object(L, cb), result);
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

int call_impl(lua_State* L, int start, std::string const& obj, std::string const& route)
{
   int top = lua_gettop(L);

   Function fn;
   fn.object = obj;
   fn.route = route;

   for (int i = start; i <= top; i++) {
      JSONNode arg;
      bool converted = false;
      object obj = object(from_stack(L, i));
      if (type(obj) == LUA_TUSERDATA) {
         try {
            object toarg = obj["__toarg"];
            if (toarg.is_valid() && type(toarg) == LUA_TFUNCTION) {
               arg = JSONNode("", call_function<std::string>(toarg, obj));
               converted = true;
            }
         } catch (std::exception& e) {
            LOG(WARNING) << "failed to convert arg " << (i - start) << " calling " << fn << ". falling back to script host (" << e.what() << ")";
         }         
      }
      if (!converted) {
         arg = lua::ScriptHost::LuaToJson(L, obj);
      }
      fn.args.push_back(arg);
   }

   std::string name = BUILD_STRING("lua " << fn);
   ReactorDeferredPtr d = GetReactor(L)->Call(fn);

   object(L, LuaDeferred::Wrap(L, name, d)).push(L);
   return 1;
}

int call(lua_State* L)
{
   return call_impl(L, 2, "", luaL_checkstring(L, 1));
}

int call_obj(lua_State* L)
{
   return call_impl(L, 3, luaL_checkstring(L, 1), luaL_checkstring(L, 2));
}

LuaDeferredPtr trace_obj(lua_State* L, object obj)
{
   std::string object_name;
   if (type(obj) == LUA_TSTRING) {
      object_name = object_cast<std::string>(obj);
   } else if (type(obj) == LUA_TUSERDATA) {
      try {
         object_name = call_function<std::string>(obj["__tojson"], obj, obj);
      } catch (std::exception& e) {
         throw std::invalid_argument(BUILD_STRING("failed to convert object to uri in trace_obj: " << e.what()));
      }
   }
   Trace t(object_name);

   ReactorDeferredPtr d = GetReactor(L)->InstallTrace(t);
   LuaDeferredPtr l = LuaDeferred::Wrap(L, BUILD_STRING("lua " << t), d);

   l->Always([L, t]() {
      GetReactor(L)->RemoveTrace(UnTrace(t.caller, t.call_id));
   });
   return l;
}

IMPLEMENT_TRIVIAL_TOSTRING(CoreReactor);

void lua::rpc::open(lua_State* L, CoreReactorPtr reactor)
{
   module(L) [
      namespace_("_radiant") [
         def("call",             &call),
         def("call_obj",         &call_obj),
         // def("trace_obj",        &trace_obj), <- pretty sure this is crap..
         namespace_("rpc") [
            lua::RegisterType<CoreReactor>(),
            lua::RegisterTypePtr<LuaDeferred>()
               .def("resolve",    &LuaDeferred::Resolve)
               .def("reject",     &LuaDeferred::Reject)
               .def("notify",     &LuaDeferred::Notify)
               .def("done",       &LuaDeferred_Done)
               .def("fail",       &LuaDeferred_Fail)
               .def("progress",   &LuaDeferred_Progress)
               .def("always",     &LuaDeferred_Always),
            lua::RegisterTypePtr<Session>()
               .def_readonly("faction", &Session::faction)
         ]
      ]
   ];
   globals(L)["_reactor"] = object(L, reactor.get());

   auto register_var_args_fn = [=](lua_CFunction f) -> object {
      lua_register(L, "_radiant_tmp_fn", f);
      return globals(L)["_radiant_tmp_fn"];
   };

   object radiant = globals(L)["_radiant"];
   radiant["call"] = register_var_args_fn(&call);
   radiant["call_obj"] = register_var_args_fn(&call_obj);
}
