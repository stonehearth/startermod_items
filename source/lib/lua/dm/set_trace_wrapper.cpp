#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "dm/set_trace.h"
#include "set_trace_wrapper.h"

using namespace radiant;
using namespace radiant::lua;

template <typename T>
SetTraceWrapper<T>::SetTraceWrapper(std::shared_ptr<T> trace) :
   trace_(trace)
{
}

template <typename T>
std::shared_ptr<SetTraceWrapper<T>> SetTraceWrapper<T>::OnDestroyed(luabind::object unsafe_destroyed_cb)
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
         lua::ScriptHost::ReportCStackException(destroyed_cb.interpreter(), e);
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<SetTraceWrapper<T>> SetTraceWrapper<T>::OnAdded(luabind::object unsafe_added_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_added_cb.interpreter());
   luabind::object added_cb(cb_thread, unsafe_added_cb);

   trace_->OnAdded([added_cb](typename T::Value const& value) mutable {
      try {
         added_cb(value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         lua::ScriptHost::ReportCStackException(added_cb.interpreter(), e);
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<SetTraceWrapper<T>> SetTraceWrapper<T>::OnRemoved(luabind::object unsafe_removed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_removed on invalid trace");
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_removed_cb.interpreter());
   luabind::object removed_cb(cb_thread, unsafe_removed_cb);

   trace_->OnRemoved([removed_cb](typename T::Value const& value) mutable {
      try {
         removed_cb(value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         lua::ScriptHost::ReportCStackException(removed_cb.interpreter(), e);
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<SetTraceWrapper<T>> SetTraceWrapper<T>::PushObjectState()
{
   if (!trace_) {
      throw std::logic_error("called push_object_state on invalid trace");
   }
   trace_->PushObjectState();
   return shared_from_this();
}

template <typename T>
void SetTraceWrapper<T>::Destroy()
{
   trace_ = nullptr;
}

#undef CREATE_SET
#define CREATE_SET(S)      template SetTraceWrapper<dm::SetTrace<S>>;
ALL_DM_TYPES
