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
std::shared_ptr<SetTraceWrapper<T>> SetTraceWrapper<T>::OnDestroyed(lua_State* L, luabind::object destroyed_cb)
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
std::shared_ptr<SetTraceWrapper<T>> SetTraceWrapper<T>::OnAdded(lua_State* L, luabind::object added_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
   trace_->OnAdded([this, L, cb_thread, added_cb](typename T::Value const& value) {
      try {
         luabind::object(cb_thread, added_cb)(value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         lua::ScriptHost::ReportCStackException(L, e);
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<SetTraceWrapper<T>> SetTraceWrapper<T>::OnRemoved(lua_State* L, luabind::object removed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_removed on invalid trace");
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
   trace_->OnRemoved([this, L, cb_thread, removed_cb](typename T::Value const& value) {
      try {
         luabind::object(cb_thread, removed_cb)(value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         lua::ScriptHost::ReportCStackException(L, e);
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
