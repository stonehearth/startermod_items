#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "dm/map_trace.h"
#include "map_trace_wrapper.h"

using namespace radiant;
using namespace radiant::lua;

template <typename T>
MapTraceWrapper<T>::MapTraceWrapper(std::shared_ptr<T> trace) :
   trace_(trace)
{
}

template <typename T>
std::shared_ptr<MapTraceWrapper<T>> MapTraceWrapper<T>::OnDestroyed(lua_State* L, luabind::object destroyed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
   trace_->OnDestroyed([this, cb_thread, destroyed_cb]() {
      try {
         luabind::object(cb_thread, destroyed_cb)();
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<MapTraceWrapper<T>> MapTraceWrapper<T>::OnAdded(lua_State* L, luabind::object changed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
   trace_->OnAdded([this, cb_thread, changed_cb](typename T::Key const& key, typename T::Value const& value) {
      try {
         luabind::object(cb_thread, changed_cb)(key, value);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<MapTraceWrapper<T>> MapTraceWrapper<T>::OnRemoved(lua_State* L, luabind::object removed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_removed on invalid trace");
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(L);
   trace_->OnRemoved([this, cb_thread, removed_cb](typename T::Key const& key) {
      try {
         luabind::object(cb_thread, removed_cb)(key);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<MapTraceWrapper<T>> MapTraceWrapper<T>::PushObjectState()
{
   if (!trace_) {
      throw std::logic_error("called push_object_state on invalid trace");
   }
   trace_->PushObjectState();
   return shared_from_this();
}

template <typename T>
void MapTraceWrapper<T>::Destroy()
{
   trace_ = nullptr;
}

#undef CREATE_MAP
#define CREATE_MAP(M)      template MapTraceWrapper<dm::MapTrace<M>>;
ALL_DM_TYPES
