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

      LuaFuturePtr futre = std::make_shared<LuaFuture>(L, d, fn.desc());
      try {
         int top = lua_gettop(L);

         int nargs = 3 + fn.args.size();
         detail::push(L, method);               // method
         detail::push(L, obj);                  // self
         detail::push(L, fn.caller);            // session
         detail::push(L, futre);         // response
         for (const auto& arg : fn.args) {
            detail::push(L, scriptHost_->JsonToLua(L, arg));
         }
         if (detail::pcall(L, nargs, 1)) {
            throw luabind::error(L);
         }
         object result(from_stack(L, -1));
         lua_pop(L, 1);

         if (result.is_valid() && type(result) != LUA_TNIL) {
            futre->Resolve(result);
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
