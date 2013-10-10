#include "pch.h"
#include <regex>
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

// Parses 'a.b.c.d' -> 'a' , 'b.c.d'
static const std::regex route_parser_regex__("^([^:\\\\/]+):([^\\\\/]+)$");

LuaModuleRouter::LuaModuleRouter(lua::ScriptHost* s, std::string const& endpoint) :
   LuaRouter(s),
   endpoint_(endpoint)
{
}

ReactorDeferredPtr LuaModuleRouter::Call(Function const& fn)
{
   try {
      if (!fn.object.empty()) {
         return nullptr;
      }
      std::smatch match;

      if (!std::regex_match(fn.route, match, route_parser_regex__)) {
         return nullptr;
      }
      std::string const& module = match[1];
      std::string const& route = match[2];

      res::Manifest manifest = res::ResourceManager2::GetInstance().LookupManifest(module);
      res::Function f = manifest.get_function(route);
      if (!f || f.endpoint() != endpoint_) {
         return nullptr;
      }
      LOG(INFO) << "lua router dispatching call '" << fn << "'...";

      ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(fn.desc());
      CallModuleFunction(d, fn, f.script(), route);
      return d;
   } catch (std::exception &e) {
      LOG(WARNING) << "error dispatching " << fn << " in lua module router: " << e.what();
   }
   return nullptr;
}

void LuaModuleRouter::CallModuleFunction(ReactorDeferredPtr d, Function const& fn, std::string const& script, std::string const& function_name)
{
   try {
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
      object method = obj[function_name];
      if (!method.is_valid() || type(method) != LUA_TFUNCTION) {
         throw core::Exception(BUILD_STRING("constructed lua handler does not implement " << fn));
      }
      CallLuaMethod(d, obj, method, fn);
   } catch (std::exception const& e) {
      LOG(WARNING) << "error attempting to call " << fn << " in lua module router: " << e.what();
      d->RejectWithMsg(e.what());
   }
}
