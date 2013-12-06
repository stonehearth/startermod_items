#ifndef _RADIANT_LUA_DM_MAP_TRACE_WRAPPER_H
#define _RADIANT_LUA_DM_MAP_TRACE_WRAPPER_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename T>
class MapTraceWrapper : public std::enable_shared_from_this<MapTraceWrapper<T>>
{
public:
   MapTraceWrapper(std::shared_ptr<T> trace) :
      trace_(trace)
   {
   }

   std::shared_ptr<MapTraceWrapper<T>> OnAdded(luabind::object changed_cb)
   {
      if (!trace_) {
         throw std::logic_error("called on_added on invalid trace");
      }
      trace_->OnAdded([this, changed_cb](typename T::Key const& key, typename T::Value const& value) {
         try {
            call_function<void>(changed_cb, key, value);
         } catch (std::exception const& e) {
            LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         }
      });
      return shared_from_this();
   }

   std::shared_ptr<MapTraceWrapper<T>> OnRemoved(luabind::object removed_cb)
   {
      if (!trace_) {
         throw std::logic_error("called on_removed on invalid trace");
      }
      trace_->OnRemoved([this, removed_cb](typename T::Key const& key) {
         try {
            call_function<void>(removed_cb, key);
         } catch (std::exception const& e) {
            LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         }
      });
      return shared_from_this();
   }

   std::shared_ptr<MapTraceWrapper<T>> PushObjectState()
   {
      if (!trace_) {
         throw std::logic_error("called push_object_state on invalid trace");
      }
      trace_->PushObjectState();
      return shared_from_this();
   }

   void Destroy()
   {
      trace_ = nullptr;
   }
   
private:
   std::shared_ptr<T>   trace_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_MAP_TRACE_WRAPPER_H