#include "pch.h"
#include "radiant_exceptions.h"
#include "lua_deferred.h"
#include "reactor_deferred.h"
#include "lib/lua/script_host.h"
#include "function.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

#define LUA_DEFERRED_LOG(n) LUA_LOG(n) << LogPrefix()

static JSONNode LuaToJson(lua_State* L, luabind::object o)
{
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
}

LuaFuture::LuaFuture(lua_State* L, rpc::ReactorDeferredPtr future, std::string const& dbgName) :
   core::Deferred<luabind::object, luabind::object>(dbgName),
   _dbgName(dbgName),
   _future(future)
   
{
   LUA_DEFERRED_LOG(7) << "creating lua future";

   // When some lua code marks us as done, forward the result to our future
   Done([L, this](luabind::object o) {
      if (_future) {
         _future->Resolve(LuaToJson(L, o));
      }
   });

   Progress([L, this](luabind::object o) {
      if (_future) {
         _future->Notify(LuaToJson(L, o));
      }
   });

   Fail([L, this](luabind::object o) {
      if (_future) {
         _future->Reject(LuaToJson(L, o));
      }
   });
}

LuaFuture::~LuaFuture()
{
   LUA_DEFERRED_LOG(7) << "lua future destructor";
   Destroy();
}

void LuaFuture::Destroy() {
   LUA_DEFERRED_LOG(7) << "destroying lua Future wrapper (refcount:" << _future.use_count() << ")";
   rpc::ReactorDeferredPtr keepAlive = std::move(_future);
}


LuaPromise::LuaPromise(rpc::ReactorDeferredPtr promise, std::string const& dbgName) :
   core::Deferred<luabind::object, luabind::object>(dbgName),
   _dbgName(dbgName),
   _promise(promise)
{
   LUA_DEFERRED_LOG(7) << "creating lua promise";
}

void LuaPromise::Initialize(lua_State* L)
{
   LUA_DEFERRED_LOG(7) << "saving lua promise";
   lua::ScriptHost::GetScriptHost(L)->SaveLuaPromise(shared_from_this());

   _promise->Done([L, this](JSONNode const& json) {
      Resolve(lua::ScriptHost::JsonToLua(L, json));
   });

   _promise->Fail([L, this](JSONNode const& json) {
      Reject(lua::ScriptHost::JsonToLua(L, json));
   });

   _promise->Progress([L, this](JSONNode const& json) {
      Notify(lua::ScriptHost::JsonToLua(L, json));
   });

   _promise->Always([L, this]() {
      lua::ScriptHost* sh = lua::ScriptHost::GetScriptHost(L);
      if (sh) {
         LUA_DEFERRED_LOG(7) << "freeing lua promise";
         sh->FreeLuaPromise(shared_from_this());
      }
   });
}

LuaPromisePtr LuaPromise::Create(lua_State* L, rpc::ReactorDeferredPtr promise, std::string const& dbgName)
{
   // the typical pattern for using lua promises looks like:
   // 
   //    _radiant.call('some_function')
   //       :done(function()
   //             ...
   //          end)
   //
   // notice that there's nothing there to keep the promise alive (ug!).  the conclusion
   // we draw is that the lua promise must live at least along as the `promise` passed in.
   // to implement this, make the script host hold onto this promise.  we'll clear it out
   // in the always handler in ::Initialize

   LuaPromisePtr luaPromise(new LuaPromise(promise, dbgName));
   luaPromise->Initialize(L);
   return luaPromise;
}

LuaPromise::~LuaPromise()
{
   LUA_DEFERRED_LOG(7) << "lua promise destructor";
   Destroy();
}

void LuaPromise::Destroy() {
   LUA_DEFERRED_LOG(7) << "destroying lua promise wrapper (refcount:" << _promise.use_count() << ")";
   rpc::ReactorDeferredPtr keepAlive = std::move(_promise);
}
