#include "lib/lua/pch.h"
#include "lib/lua/script_host.h"
#include "dm/trace.h"
#include "trace_wrapper.h"

using namespace radiant;
using namespace radiant::lua;


TraceWrapper::TraceWrapper(dm::TracePtr trace) :
   trace_(trace)
{
}

std::shared_ptr<TraceWrapper> TraceWrapper::OnChanged(luabind::object changed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_changed on invalid trace");
   }
   trace_->OnModified_([this, changed_cb]() {
      try {
         luabind::call_function<void>(changed_cb);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

std::shared_ptr<TraceWrapper> TraceWrapper::OnDestroyed(luabind::object destroyed_cb)
{
   if (!trace_) {
      throw std::logic_error("called on_destroyed on invalid trace");
   }
   trace_->OnDestroyed_([this, destroyed_cb]() {
      try {
         luabind::call_function<void>(destroyed_cb);
      } catch (std::exception const& e) {
         LUA_LOG(1) << "exception delivering lua trace: " << e.what();
      }
   });
   return shared_from_this();
}

std::shared_ptr<TraceWrapper> TraceWrapper::PushObjectState()
{
   if (!trace_) {
      throw std::logic_error("called push_object_state on invalid trace");
   }
   trace_->PushObjectState_();
   return shared_from_this();
}

void TraceWrapper::Destroy()
{
   trace_ = nullptr;
}
