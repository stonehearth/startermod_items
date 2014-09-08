#include "radiant.h"
#include "om/lua/lua_region.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;

template <typename OuterBox>
LuaDeepRegionGuard<OuterBox>::LuaDeepRegionGuard(RegionTracePtr trace) :
   trace_(trace)
{
}

template <typename OuterBox>
std::shared_ptr<LuaDeepRegionGuard<OuterBox>> LuaDeepRegionGuard<OuterBox>::OnChanged(luabind::object unsafe_changed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_changed on invalid trace");
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_changed_cb.interpreter());
   luabind::object changed_cb(cb_thread, unsafe_changed_cb);

   typedef OuterBox::Value::element_type::Value Region;
   trace_->OnChanged([this, changed_cb](Region const& value) {
      try {
         luabind::call_function<void>(changed_cb, value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

template <typename OuterBox>
std::shared_ptr<LuaDeepRegionGuard<OuterBox>> LuaDeepRegionGuard<OuterBox>::PushObjectState()
{
   if (!trace_) {
      throw std::logic_error("called push_object_state on invalid trace");
   }
   trace_->PushObjectState();
   return shared_from_this();
}

template <typename OuterBox>
void LuaDeepRegionGuard<OuterBox>::Destroy()
{
   trace_ = nullptr;
}

#define MAKE_CLS(Cls) \
   template Cls::LuaDeepRegionGuard(RegionTracePtr trace); \
   template std::shared_ptr<Cls> Cls::OnChanged(luabind::object unsafe_changed_cb); \
   template std::shared_ptr<Cls> Cls::PushObjectState(); \
   template void Cls::Destroy(); \

MAKE_CLS(LuaDeepRegion2Guard);
MAKE_CLS(LuaDeepRegion3Guard);
MAKE_CLS(LuaDeepRegion2fGuard);
MAKE_CLS(LuaDeepRegion3fGuard);
