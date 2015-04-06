#include "../pch.h"
#include "open.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/session.h"
#include "lib/rpc/function.h"
#include "lib/rpc/trace.h"
#include "lib/rpc/untrace.h"
#include "lib/rpc/core_reactor.h"
#include "lib/lua/script_host.h"
#include "lib/lua/register.h"
#include "lib/json/core_json.h"

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

LuaDeferredPtr LuaDeferred_Destroy(lua_State* L, LuaDeferredPtr deferred)
{
   if (deferred) {
      deferred->Destroy();
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
      object obj = object(from_stack(L, i));
      JSONNode arg = lua::ScriptHost::LuaToJson(L, obj); // xx: LuaToProtobuf!!
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
   std::string obj_address;
   luabind::object obj = luabind::object(luabind::from_stack(L, 1));
   int type = luabind::type(obj);
   if (type == LUA_TSTRING) {
      obj_address = luabind::object_cast<std::string>(obj);
   } else if (type == LUA_TUSERDATA) {
      obj_address = luabind::object_cast<std::string>(obj["get_address"](obj));
   }
   if (obj_address.empty()) {
      throw std::logic_error("unxpected type in argument 1 of call_obj");
   }
   return call_impl(L, 3, obj_address.c_str(), luaL_checkstring(L, 2));
}

IMPLEMENT_TRIVIAL_TOSTRING(CoreReactor);
DEFINE_INVALID_JSON_CONVERSION(CoreReactor);
DEFINE_INVALID_JSON_CONVERSION(LuaDeferred);
DEFINE_INVALID_JSON_CONVERSION(Session);

void lua::rpc::open(lua_State* L, CoreReactorPtr reactor)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("rpc") [
            lua::RegisterType_NoTypeInfo<CoreReactor>("CoreReactor"),
            lua::RegisterTypePtr_NoTypeInfo<LuaDeferred>("LuaDeferred")
               .def("resolve",    &LuaDeferred::Resolve)
               .def("reject",     &LuaDeferred::Reject)
               .def("notify",     &LuaDeferred::Notify)
               .def("done",       &LuaDeferred_Done)
               .def("fail",       &LuaDeferred_Fail)
               .def("progress",   &LuaDeferred_Progress)
               .def("always",     &LuaDeferred_Always)
               .def("destroy",    &LuaDeferred_Destroy)
               ,
            lua::RegisterTypePtr_NoTypeInfo<Session>("Session")
               .def_readonly("player_id", &Session::player_id)
         ]
      ]
   ];
   globals(L)["_reactor"] = object(L, reactor.get());
  
   object radiant = globals(L)["_radiant"];
   radiant["call"] = GetPointerToCFunction(L, &call);
   radiant["call_obj"] = GetPointerToCFunction(L, &call_obj);
}
