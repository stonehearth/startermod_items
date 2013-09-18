#include "pch.h"
#include <regex>
#include <luabind/detail/pcall.hpp>
#include "radiant_macros.h"
#include "lua_router.h"
#include "lua_deferred.h"
#include "reactor_deferred.h"
#include "session.h"
#include "function.h"
#include "lua/script_host.h"
#include "resources/manifest.h"
#include "resources/res_manager.h"
#include "radiant_json.h"
#include "lua/radiant_lua.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

LuaRouter::LuaRouter(lua::ScriptHost& host, std::string const& endpoint) :
   host_(host),
   endpoint_(endpoint)
{
}

ReactorDeferredPtr LuaRouter::Call(Function const& fn)
{
   try {
      res::Manifest manifest = res::ResourceManager2::GetInstance().LookupManifest(fn.object);
      res::Function f = manifest.get_function(fn.route);
      if (f) {
         if (f.endpoint() != endpoint_) {
            return nullptr;
         }
         return CallModuleFunction(fn, f.script());
      }
   } catch (std::exception &e) {
      LOG(WARNING) << "error dispatching " << fn << " in lua router: " << e.what();
   }
   return nullptr;
}

ReactorDeferredPtr LuaRouter::CallModuleFunction(Function const& fn, std::string const& script)
{

   using namespace luabind;
   ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(fn.desc());

   try {
      LOG(INFO) << "lua router dispatching call '" << fn << "'...";

      lua_State* L = host_.GetCallbackThread();
      object ctor(L, host_.RequireScript(script));
      if (!ctor.is_valid() || type(ctor) == LUA_TNIL) {
         throw core::Exception(BUILD_STRING("failed to retrieve lua call handler while processing " << fn));
      }
      object obj = call_function<object>(ctor);
      object method = obj[fn.route]; 
      if (!method.is_valid() || type(method) != LUA_TFUNCTION) {
         throw core::Exception(BUILD_STRING("constructed lua handler does not implement " << fn));
      }

      auto lua_to_json = [this](object o) -> JSONNode {
         if (o.is_valid() && type(o) == LUA_TTABLE) {
            return libjson::parse(host_.LuaToJson(o));
         }
         return JSONNode();
      };

      LuaDeferredPtr lua_deferred = std::make_shared<LuaDeferred>(fn.desc());

      lua_deferred->Done([lua_to_json, d](object o) {
         if (o.is_valid()) {
            d->Resolve(lua_to_json(o));
         }
      });
      lua_deferred->Progress([lua_to_json, d](object o) {
         if (o.is_valid()) {
            d->Notify(lua_to_json(o));
         }
      });
      lua_deferred->Fail([lua_to_json, d](object o) {
         if (o.is_valid()) {
            d->Reject(lua_to_json(o));
         }
      });

   
      int top = lua_gettop(L);

      int nargs = 3 + fn.args.size();
      detail::push(L, method);               // method
      detail::push(L, obj);                  // self
      detail::push(L, fn.caller);            // session
      detail::push(L, lua_deferred);         // response
      for (const auto& arg : fn.args) {
         detail::push(L, host_.JsonToLua(L, arg));
      }
      if (detail::pcall(L, nargs, 1)) {
         throw luabind::error(L);
      }
      object result(from_stack(L, -1));
      lua_pop(L, 1);

      if (result.is_valid() && type(result) != LUA_TNIL) {
         lua_deferred->Resolve(result);
      }
   } catch (std::exception const& e) {
      LOG(WARNING) << "error attempting to call module function in lua router: " << e.what();
      d->RejectWithMsg(e.what());
   }

   return d;
}
