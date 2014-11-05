#include "pch.h"
#include "radiant_stdutil.h"
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

#define LOR_LOG(level)      LOG_CATEGORY(rpc, level, "rpc lua")

LuaObjectRouter::LuaObjectRouter(lua::ScriptHost *s, dm::Store& store) :
   LuaRouter(s),
   store_(store)
{
}

ReactorDeferredPtr LuaObjectRouter::Call(Function const& fn)
{
   ReactorDeferredPtr d;

   LOR_LOG(9) << "trying to dispatch call '" << fn << "'...";

   try {
      if (fn.object.empty()) {
         LOR_LOG(9) << "fn.object is empty. ignoring";
         return nullptr;
      }

      luabind::object obj;
      if (boost::starts_with(fn.object, "object://")) {
         obj = LookupObjectFromStore(fn);
      } else {
         obj = LookupObjectFromLuaState(fn);
      }
      if (!obj.is_valid()) {
         return nullptr;
      }

      d = std::make_shared<ReactorDeferred>(fn.desc());

      object method = obj[fn.route];
      if (!method.is_valid() || type(method) != LUA_TFUNCTION) {
         LOR_LOG(9) << "object does not implement method.  aborting";
         throw std::invalid_argument(BUILD_STRING("object does not implement function."));
      }

      LOR_LOG(9) << "calling lua method " << fn;
      CallLuaMethod(d, obj, method, fn);
   } catch (std::exception &e) {
      LOR_LOG(3) << "error dispatching " << fn << " in lua module router: " << e.what();
      if (d) {
         d->RejectWithMsg(e.what());
      }
   }
   return d;
}

luabind::object LuaObjectRouter::LookupObjectFromStore(Function const& fn)
{
   dm::ObjectPtr o = store_.FetchObject<dm::Object>(fn.object);
   if (!o) {
      LOR_LOG(9) << "failed to fetch object in " << store_.GetName() << " store.  aborting.";
      return luabind::object();
   }
   dm::ObjectType t = o->GetObjectType();
   if (t != om::DataStoreObjectType) {
      LOR_LOG(9) << "object is not datastore.  aborting";
      return luabind::object();
   }
   om::DataStoreRef db = std::dynamic_pointer_cast<om::DataStore>(o);
   luabind::object obj = db.lock()->GetController();
   if (!obj.is_valid()) {
      LOR_LOG(9) << "object controller is not valid.  aborting.";
   }
   return obj;
}

luabind::object LuaObjectRouter::LookupObjectFromLuaState(Function const& fn)
{
   luabind::object obj;
   lua_State* L = store_.GetInterpreter();

   try {
      obj = luabind::globals(L);
      std::vector<std::string> parts = strutil::split(fn.object, ".");
      for (std::string const& part : parts) {
         if (luabind::type(obj) != LUA_TTABLE) {
            LOR_LOG(9) << "failed to fetch object in lua interpreter (reason: prefix is not a table)";
            return luabind::object();
         }
         obj = obj[part];
         if (luabind::type(obj) == LUA_TNIL) {
            LOR_LOG(9) << "failed to fetch object in lua interpreter (reason: lookup for '" << part << "' returned nil)";
            return luabind::object();
         }
      }
   } catch (std::exception const& e) {
      LOR_LOG(9) << "failed to fetch object in lua interpreter (reason:" << e.what() << ")";
      return luabind::object();
   }
   return obj;
}

