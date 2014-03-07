#include "pch.h"
#include "radiant_stdutil.h"
#include "trace.h"
#include "reactor_deferred.h"
#include "trace_object_router.h"
#include "om/json.h"
#include "dm/object.h"
#include "dm/store.h"
#include "dm/trace.h"
#include "lib/json/node.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

TraceObjectRouter::TraceObjectRouter(dm::Store& store) :
   store_(store)
{
}

void TraceObjectRouter::CheckDeferredTraces()
{
   auto i = deferred_traces_.begin();
   while (i != deferred_traces_.end()) {
      ReactorDeferredPtr d = i->second.lock();
      if (!d) {
         i = deferred_traces_.erase(i);
         continue;
      }
      std::string const& uri = i->first;
      dm::ObjectPtr obj = store_.FetchObject<dm::Object>(uri);
      if (obj) {
         InstallTrace(uri, d, obj);
         i = deferred_traces_.erase(i);
         continue;
      }
      i++;
   }
}

ReactorDeferredPtr TraceObjectRouter::InstallTrace(Trace const& trace)
{
   const char* reason = trace.reason.c_str();

   ReactorDeferredPtr d = GetTrace(trace);
   if (d) {
      return d;
   }

   if (!store_.IsValidStoreAddress(trace.route)) {
      // Make sure the path is actually valid for this store (for example, if someone is trying
      // to trace the authoring store and this is the router for the game store, we should
      // just bail unconditionally).
      return nullptr;
   }

   dm::ObjectPtr obj = store_.FetchObject<dm::Object>(trace.route);
   if (obj) {
      ReactorDeferredPtr deferred = std::make_shared<ReactorDeferred>(trace.desc());
      InstallTrace(trace.route, deferred, obj);
      return deferred;
   }

   // Hmm.  This is the right store, but the object isn't in it.  Maybe it
   // will show up later! (e.g. when the client or ui traces an object that they
   // know exists, but it hasn't been streamed by the server yet).    These
   // traces get installed in CheckDeferredTraces.
   ReactorDeferredPtr deferred = std::make_shared<ReactorDeferred>(trace.desc());
   deferred_traces_[trace.route] = deferred;
   return deferred;
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

void TraceObjectRouter::InstallTrace(std::string const& uri, ReactorDeferredPtr deferred, dm::ObjectPtr obj)
{
   ASSERT(obj);
   ASSERT(!stdutil::contains(traces_, uri));

   ObjectTraceEntry& entry = traces_[uri];
   entry.deferred = deferred;
   entry.obj = obj;
   entry.trace = obj->TraceObjectChanges("rpc", dm::RPC_TRACES);

   entry.trace->OnModified_([this, uri, entry]() {
      auto obj = entry.obj.lock();
      auto deferred = entry.deferred.lock();
      if (obj && deferred) {
         json::Node data;
          if (obj->GetObjectType() == om::JsonBoxedObjectType) {
             // writing this code makes me a bad person.  i'm certain of it. =(
             data = std::static_pointer_cast<om::JsonBoxed>(obj)->Get();
          } else {
            obj->SerializeToJson(data);
          }
         deferred->Notify(data);
      } else {
         traces_.erase(uri);
      }
   });
   entry.trace->PushObjectState_();
}
