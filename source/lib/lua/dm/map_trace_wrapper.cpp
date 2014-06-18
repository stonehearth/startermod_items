#include "lib/lua/pch.h"
#include "dm/types/all_types.h"
#include "dm/map_trace.h"
#include "map_trace_wrapper.h"

using namespace radiant;
using namespace radiant::lua;

/*
 * ValueCast Template.
 *
 * Casts a value into the cpp type we'd like it to represented in Lua trace callbacks.  For
 * now, let's just say all std::shared_ptr<>s should be converted to weak_ptrs.  This fixes
 * a problem where lua traces on an Entity's component map would keep components alive
 * longer than the entity itself (until the temporary constructed to deliver the OnAdded
 * cb got reaped).  We may want to opt-into this based on the dm::Map type later, but this
 * is a good, intermediate step toward that goal.
 */

template <typename T>
struct ValueCast {
   // By default, just return the value
   T ToLua(T const& value)
   {
      return value;
   }
};

template <typename T>
struct ValueCast<std::shared_ptr<T>> {
   // Shared pointers turn into weak pointers.
   std::weak_ptr<T> ToLua(std::shared_ptr<T> value)
   {
      return value;
   }
};

template <typename T>
MapTraceWrapper<T>::MapTraceWrapper(std::shared_ptr<T> trace) :
   trace_(trace)
{
}

template <typename T>
std::shared_ptr<MapTraceWrapper<T>> MapTraceWrapper<T>::OnDestroyed(luabind::object unsafe_destroyed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }

   // 1) make sure we use the callback thread which is guarenteed not to be in a yielded coroutine.
   // 2) make sure we capture a new variable pointing to the cb thread interpreter.  luabind seems
   //    to have a bug where it doesn't ref the interpreter of objects.  so if the callback was
   //    defined in a coroutine and that corutine goes away, we still have a reference to the
   //    callback, but the lua_State gets reaped!  next time we try to do something with that
   //    callback, we're going to blow up.

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
std::shared_ptr<MapTraceWrapper<T>> MapTraceWrapper<T>::OnAdded(luabind::object unsafe_changed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_added on invalid trace");
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_changed_cb.interpreter());
   luabind::object changed_cb(cb_thread, unsafe_changed_cb);

   trace_->OnAdded([changed_cb](typename T::Key const& key, typename T::Value const& value) mutable {
      try {
         changed_cb(key, ValueCast<T::Value>().ToLua(value));
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

template <typename T>
std::shared_ptr<MapTraceWrapper<T>> MapTraceWrapper<T>::OnRemoved(luabind::object unsafe_removed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_removed on invalid trace");
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_removed_cb.interpreter());
   luabind::object removed_cb(cb_thread, unsafe_removed_cb);

   trace_->OnRemoved([removed_cb](typename T::Key const& key) mutable {
      try {
         removed_cb(key);
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
