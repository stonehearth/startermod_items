#include "pch.h"
#include <regex>
#include <luabind/detail/pcall.hpp>
#include "lib/json/node.h"
#include "radiant_macros.h"
#include "lua_object_router.h"
#include "reactor_deferred.h"
#include "function.h"
#include "dm/store.h"
#include "om/components/data_store.ridl.h"

using namespace ::radiant;
using namespace ::radiant::rpc;
using namespace luabind;

#define LUA_LOG(level)      LOG_CATEGORY(rpc, level, "rpc lua")

LuaObjectRouter::LuaObjectRouter(lua::ScriptHost *s, dm::Store& store) :
   LuaRouter(s),
   store_(store)
{
}

ReactorDeferredPtr LuaObjectRouter::Call(Function const& fn)
{
   ReactorDeferredPtr d;

   LUA_LOG(9) << "trying to dispatch call '" << fn << "'...";

   try {
      dm::ObjectPtr o = store_.FetchObject<dm::Object>(fn.object);
      if (!o) {
         LUA_LOG(9) << "failed to fetch object in " << store_.GetName() << " store.  aborting.";
         return nullptr;
      }
      dm::ObjectType t = o->GetObjectType();
      if (t != om::DataStoreObjectType) {
         LUA_LOG(9) << "object is not datastore.  aborting";
         return nullptr;
      }
      om::DataStoreRef db = std::dynamic_pointer_cast<om::DataStore>(o);

      object obj = db.lock()->GetController();
      if (!obj.is_valid()) {
         LUA_LOG(9) << "object controller is not valid.  aborting.";
         return nullptr;
      }
      d = std::make_shared<ReactorDeferred>(fn.desc());

      object method = obj[fn.route];
      if (!method.is_valid() || type(method) != LUA_TFUNCTION) {
         LUA_LOG(9) << "object does not implement method.  aborting";
         throw std::invalid_argument(BUILD_STRING("object does not implement function."));
      }

      LUA_LOG(9) << "calling lua method " << fn;
      CallLuaMethod(d, obj, method, fn);
   } catch (std::exception &e) {
      RPC_LOG(3) << "error dispatching " << fn << " in lua module router: " << e.what();
      if (d) {
         d->RejectWithMsg(e.what());
      }
   }
   return d;
}
