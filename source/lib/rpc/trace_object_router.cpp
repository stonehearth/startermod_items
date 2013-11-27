#include "pch.h"
#include "trace.h"
#include "reactor_deferred.h"
#include "trace_object_router.h"
#include "om/object_formatter/object_formatter.h"
#include "dm/object.h"
#include "dm/store.h"
#include "dm/trace.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

TraceObjectRouter::TraceObjectRouter(dm::Store& store) :
   store_(store)
{
}

void TraceObjectRouter::AddObject(std::string const& name, dm::ObjectPtr obj)
{
   namedObjects_[name] = obj;
}

ReactorDeferredPtr TraceObjectRouter::InstallTrace(Trace const& trace)
{
   const char* reason = trace.reason.c_str();

   ReactorDeferredPtr result = GetTrace(trace);
   if (!result) {
      // look in the named object store first...
      auto i = namedObjects_.find(trace.route);
      if (i != namedObjects_.end()) {
         result = InstallTrace(trace, i->second.lock(), reason);
      }
   }
   if (!result) {
      // now check the object store...
      dm::ObjectPtr obj = om::ObjectFormatter().GetObject(store_, trace.route);
      result = InstallTrace(trace, obj, reason);
   }
   return result;
}

ReactorDeferredPtr TraceObjectRouter::GetTrace(Trace const& trace)
{
   auto i = traces_.find(trace.route);
   if (i != traces_.end()) {
      ObjectTraceEntry &entry = i->second;
      dm::ObjectPtr obj = entry.obj.lock();
      ReactorDeferredPtr deferred = entry.deferred.lock();
      if (obj && deferred) {
         return deferred;
      }
      traces_.erase(i);
   }
   return nullptr;
}

ReactorDeferredPtr TraceObjectRouter::InstallTrace(Trace const& trace, dm::ObjectPtr obj, const char* reason)
{
   std::string uri = trace.route;

   if (!obj) {
      return nullptr;
   }

   ReactorDeferredPtr deferred = std::make_shared<ReactorDeferred>(trace.desc());
   ObjectTraceEntry& entry = traces_[uri];
   entry.obj = obj;
   entry.deferred = deferred;
   entry.trace = obj->TraceObjectChanges("rpc", dm::RPC_TRACES);
   entry.trace->OnModified_([this, uri, entry]() {
      auto obj = entry.obj.lock();
      auto deferred = entry.deferred.lock();
      if (obj && deferred) {
         JSONNode data = om::ObjectFormatter().ObjectToJson(obj);
         deferred->Notify(data);
      } else {
         traces_.erase(uri);
      }
   });
   entry.trace->PushObjectState_();

   return deferred;
}
