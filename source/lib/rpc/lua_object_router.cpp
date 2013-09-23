#include "pch.h"
#include <regex>
#include <luabind/detail/pcall.hpp>
#include "radiant_json.h"
#include "radiant_macros.h"
#include "lua_object_router.h"
#include "reactor_deferred.h"
#include "function.h"
#include "dm/store.h"
#include "om/object_formatter/object_formatter.h"
#include "om/data_binding.h"

using namespace ::radiant;
using namespace ::radiant::rpc;
using namespace luabind;

LuaObjectRouter::LuaObjectRouter(lua::ScriptHost *s, dm::Store& store) :
   LuaRouter(s),
   store_(store)
{
}

ReactorDeferredPtr LuaObjectRouter::Call(Function const& fn)
{
   ReactorDeferredPtr d;

   try {
      dm::ObjectPtr o = om::ObjectFormatter().GetObject(store_, fn.object);
      if (!o || o->GetObjectType() != om::DataBindingObjectType) {
         return nullptr;
      }
      om::DataBindingPtr db = std::static_pointer_cast<om::DataBinding>(o);

      object obj = db->GetModelObject();
      if (!obj.is_valid()) {
         return nullptr;
      }
      d = std::make_shared<ReactorDeferred>(fn.desc());

      object method = obj[fn.route];
      if (!method.is_valid() || type(method) != LUA_TFUNCTION) {
         throw std::invalid_argument(BUILD_STRING("object does not implement function."));
      }
      CallLuaMethod(d, obj, method, fn);
   } catch (std::exception &e) {
      LOG(WARNING) << "error dispatching " << fn << " in lua module router: " << e.what();
      if (d) {
         d->RejectWithMsg(e.what());
      }
   }
   return d;
}
