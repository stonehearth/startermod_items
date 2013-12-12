#include "pch.h"
#include <regex>
#include <luabind/detail/pcall.hpp>
#include "radiant_macros.h"
#include "lua_router.h"
#include "lua_deferred.h"
#include "reactor_deferred.h"
#include "session.h"
#include "function.h"
#include "lib/json/node.h"
#include "lib/lua/script_host.h"
#include "resources/manifest.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

using namespace luabind;

LuaRouter::LuaRouter(lua::ScriptHost* s) :
   scriptHost_(s)
{
}

void LuaRouter::CallLuaMethod(ReactorDeferredPtr d, object obj, object method, Function const& fn)
{
   try {
      lua_State* L = scriptHost_->GetCallbackThread();

      auto lua_to_json = [L, this](object o) -> JSONNode {
         JSONNode result;
         if (!o.is_valid()) {
            result.push_back(JSONNode("error", "call returned invalid json object"));
         } else {
            try {
               JSONNode value = lua::ScriptHost::LuaToJson(L, o);
               if (value.type() == JSON_NODE || value.type() == JSON_ARRAY) {
                  result = value;
               } else {
                  value.set_name("result");
                  result.push_back(value);
               }
            } catch (std::exception& e) {
               std::string err = BUILD_STRING("error converting call result: " << e.what());
               lua::ScriptHost::ReportCStackException(L, e);
               result.push_back(JSONNode("error", err));
            }               
         }
         return result;
      };

      LuaDeferredPtr lua_deferred = std::make_shared<LuaDeferred>(fn.desc());

      lua_deferred->Done([lua_to_json, d](object o) {
         d->Resolve(lua_to_json(o));
      });
      lua_deferred->Progress([lua_to_json, d](object o) {
         d->Notify(lua_to_json(o));
      });
      lua_deferred->Fail([lua_to_json, d](object o) {
         d->Reject(lua_to_json(o));
      });

   
      try {
         int top = lua_gettop(L);

         int nargs = 3 + fn.args.size();
         detail::push(L, method);               // method
         detail::push(L, obj);                  // self
         detail::push(L, fn.caller);            // session
         detail::push(L, lua_deferred);         // response
         for (const auto& arg : fn.args) {
            detail::push(L, scriptHost_->JsonToLua(L, arg));
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
         lua::ScriptHost::ReportCStackException(L, e);
         throw;
      }

   } catch (std::exception const& e) {
      RPC_LOG(3) << "error attempting to call " << fn << " in lua router: " << e.what();
      d->RejectWithMsg(e.what());
   }
}
