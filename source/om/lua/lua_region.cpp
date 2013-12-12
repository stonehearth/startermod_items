#include "pch.h"
#include "om/lua/lua_region.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;

LuaDeepRegionGuard::LuaDeepRegionGuard(DeepRegionGuardPtr trace) :
   trace_(trace)
{
}

std::shared_ptr<LuaDeepRegionGuard> LuaDeepRegionGuard::OnChanged(luabind::object changed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_changed on invalid trace");
   }
   trace_->OnChanged([this, changed_cb](csg::Region3 const& value) {
      try {
         luabind::call_function<void>(changed_cb, value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         lua::ScriptHost::ReportCStackException(changed_cb.interpreter(), e);
      }
   });
   return shared_from_this();
}

std::shared_ptr<LuaDeepRegionGuard> LuaDeepRegionGuard::PushObjectState()
{
   if (!trace_) {
      throw std::logic_error("called push_object_state on invalid trace");
   }
   trace_->PushObjectState();
   return shared_from_this();
}

void LuaDeepRegionGuard::Destroy()
{
   trace_ = nullptr;
}
