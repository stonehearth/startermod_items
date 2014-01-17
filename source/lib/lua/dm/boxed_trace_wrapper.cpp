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
std::shared_ptr<BoxedTraceWrapper<T>> BoxedTraceWrapper<T>::OnDestroyed(luabind::object unsafe_destroyed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_destroyed_cb.interpreter());
   luabind::object destroyed_cb(cb_thread, unsafe_destroyed_cb);

   trace_->OnDestroyed([destroyed_cb]() mutable {
      try {
         destroyed_cb();
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<BoxedTraceWrapper<T>> BoxedTraceWrapper<T>::OnChanged(luabind::object unsafe_changed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_changed on invalid trace");
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_changed_cb.interpreter());
   luabind::object changed_cb(cb_thread, unsafe_changed_cb);

   trace_->OnChanged([changed_cb](typename T::Value const& value) mutable {
      try {
         changed_cb(value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
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
