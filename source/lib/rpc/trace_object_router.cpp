#include "pch.h"
#include "trace.h"
#include "reactor_deferred.h"
#include "trace_object_router.h"
#include "om/object_formatter/object_formatter.h"
#include "dm/object.h"

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
      ObjectTraceEntry &ote = i->second;
      dm::ObjectPtr obj = ote.o.lock();
      ReactorDeferredPtr d = ote.d.lock();
      if (obj && d) {
         return d;
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

   ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(trace.desc());
   ObjectTraceEntry ote(obj, d);

   auto fireTrace = [this, uri, ote]() {
      auto obj = ote.o.lock();
      auto d = ote.d.lock();
      if (obj && d) {
         JSONNode data = om::ObjectFormatter().ObjectToJson(obj);
         d->Notify(data);
      } else {
         traces_.erase(uri);
         guards_.erase(uri);
      }
   };
   traces_[uri] = ote;
   guards_[uri] = obj->TraceObjectChanges(reason, fireTrace);
   fireTrace();

   return d;
}
