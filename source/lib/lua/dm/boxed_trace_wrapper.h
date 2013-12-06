#ifndef _RADIANT_LUA_DM_BOXED_TRACE_WRAPPER_H
#define _RADIANT_LUA_DM_BOXED_TRACE_WRAPPER_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename T>
class BoxedTraceWrapper : public std::enable_shared_from_this<BoxedTraceWrapper<T>>
{
public:
   BoxedTraceWrapper(std::shared_ptr<T> trace) :
      trace_(trace)
   {
   }

   std::shared_ptr<BoxedTraceWrapper<T>> OnChanged(luabind::object changed_cb)
   {
      if (!trace_) {
         throw std::logic_error("called on_changed on invalid trace");
      }
      trace_->OnChanged([this, changed_cb](typename T::Value const& value) {
         try {
            call_function<void>(changed_cb, value);
         } catch (std::exception const& e) {
            LUA_LOG(1) << "exception delivering lua trace: " << e.what();
         }
      });
      return shared_from_this();
   }

   std::shared_ptr<BoxedTraceWrapper<T>> PushObjectState()
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

#endif //  _RADIANT_LUA_DM_BOXED_TRACE_WRAPPER_H