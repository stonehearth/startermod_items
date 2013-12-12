#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "dm/boxed_trace.h"
#include "boxed_trace_wrapper.h"

using namespace radiant;
using namespace radiant::lua;

template <typename T>
BoxedTraceWrapper<T>::BoxedTraceWrapper(std::shared_ptr<T> trace) :
   trace_(trace)
{
}

template <typename T>
BoxedTraceWrapper<T>::~BoxedTraceWrapper()
{
   trace_ = nullptr;
}

template <typename T>
std::shared_ptr<BoxedTraceWrapper<T>> BoxedTraceWrapper<T>::OnDestroyed(lua_State* L, luabind::object destroyed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
   trace_->OnDestroyed([this, L, cb_thread, destroyed_cb]() {
      try {
         luabind::object(cb_thread, destroyed_cb)();
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         lua::ScriptHost::ReportCStackException(L, e);
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<BoxedTraceWrapper<T>> BoxedTraceWrapper<T>::OnChanged(lua_State* L, luabind::object changed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_changed on invalid trace");
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
   trace_->OnChanged([this, L, cb_thread, changed_cb](typename T::Value const& value) {
      try {
         luabind::object(cb_thread, changed_cb)(value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         lua::ScriptHost::ReportCStackException(L, e);
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<BoxedTraceWrapper<T>> BoxedTraceWrapper<T>::PushObjectState()
{
   if (!trace_) {
      throw std::logic_error("called push_object_state on invalid trace");
   }
   trace_->PushObjectState();
   return shared_from_this();
}

template <typename T>
void BoxedTraceWrapper<T>::Destroy()
{
   trace_ = nullptr;
}

#undef CREATE_BOXED
#define CREATE_BOXED(B)      template BoxedTraceWrapper<dm::BoxedTrace<B>>;
ALL_DM_TYPES
