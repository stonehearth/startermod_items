#include "pch.h"
#include "radiant_macros.h"
#include "radiant_json.h"
#include "lua_module_router.h"
#include "lua_deferred.h"
#include "reactor_deferred.h"
#include "session.h"
#include "function.h"
#include "lua/script_host.h"
#include "lua/radiant_lua.h"
#include "resources/manifest.h"
#include "resources/res_manager.h"

using namespace radiant;
using namespace radiant::rpc;
using namespace luabind;

LuaModuleRouter::LuaModuleRouter(lua::ScriptHost* s, std::string const& endpoint) :
   LuaRouter(s),
   endpoint_(endpoint)
{
}

ReactorDeferredPtr LuaModuleRouter::Call(Function const& fn)
{
   try {
      // Make sure the object looks like a module..
      if (fn.object.find('/') != std::string::npos) {
         return nullptr;
      }
      res::Manifest manifest = res::ResourceManager2::GetInstance().LookupManifest(fn.object);
      res::Function f = manifest.get_function(fn.route);
      if (!f || f.endpoint() != endpoint_) {
         return nullptr;
      }
      return CallModuleFunction(fn, f.script());
   } catch (std::exception &e) {
      LOG(WARNING) << "error dispatching " << fn << " in lua module router: " << e.what();
   }
   return nullptr;
}

ReactorDeferredPtr LuaModuleRouter::CallModuleFunction(Function const& fn, std::string const& script)
{
   ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(fn.desc());

   try {
      LOG(INFO) << "lua router dispatching call '" << fn << "'...";

      lua::ScriptHost* sh = GetScriptHost();
      lua_State* L = sh->GetCallbackThread();
      object ctor(L, sh->RequireScript(script));
      if (!ctor.is_valid()) {
         throw core::Exception(BUILD_STRING("failed to retrieve lua call handler while processing " << fn));
      }
      object obj;
      try {
         obj = call_function<object>(ctor);
      } catch (std::exception& e) {
         throw core::Exception(BUILD_STRING("failed to create lua call handler while processing " << fn << ": " << e.what()));
      }
      object method = obj[fn.route]; 
      if (!method.is_valid() || type(method) != LUA_TFUNCTION) {
         throw core::Exception(BUILD_STRING("constructed lua handler does not implement " << fn));
      }
      CallLuaMethod(d, obj, method, fn);
   } catch (std::exception const& e) {
      LOG(WARNING) << "error attempting to call " << fn << " in lua module router: " << e.what();
      d->RejectWithMsg(e.what());
   }
   return d;
}
