#ifndef _RADIANT_LUA_DM_TRACE_WRAPPER_H
#define _RADIANT_LUA_DM_TRACE_WRAPPER_H

#include "lib/lua/lua.h"
#include "dm/trace.h"

BEGIN_RADIANT_LUA_NAMESPACE

class TraceWrapper : public std::enable_shared_from_this<TraceWrapper>
{
public:
   TraceWrapper(dm::TracePtr trace) :
      trace_(trace)
   {
   }

   std::shared_ptr<TraceWrapper> OnChanged(luabind::object changed_cb)
   {
      if (!trace_) {
         throw std::logic_error("called on_changed on invalid trace");
      }
      trace_->OnModified_([this, changed_cb]() {
         try {
            luabind::call_function<void>(changed_cb);
         } catch (std::exception const& e) {
            LOG(WARNING) << "exception delivering lua trace: " << e.what();
         }
      });
      return shared_from_this();
   }

   std::shared_ptr<TraceWrapper> OnDestroyed(luabind::object destroyed_cb)
   {
      if (!trace_) {
         throw std::logic_error("called on_destroyed on invalid trace");
      }
      trace_->OnDestroyed_([this, destroyed_cb]() {
         try {
            luabind::call_function<void>(destroyed_cb);
         } catch (std::exception const& e) {
            LOG(WARNING) << "exception delivering lua trace: " << e.what();
         }
      });
      return shared_from_this();
   }

   std::shared_ptr<TraceWrapper> PushObjectState()
   {
      if (!trace_) {
         throw std::logic_error("called push_object_state on invalid trace");
      }
      trace_->PushObjectState_();
      return shared_from_this();
   }

   void Destroy()
   {
      trace_ = nullptr;
   }
   
private:
   dm::TracePtr   trace_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_TRACE_WRAPPER_H