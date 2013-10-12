#ifndef _RADIANT_LIB_RPC_TRACE_OBJECT_ROUTER_H
#define _RADIANT_LIB_RPC_TRACE_OBJECT_ROUTER_H

#include <unordered_map>
#include "irouter.h"
#include "forward_defines.h"
#include "tesseract.pb.h"
#include "dm/dm.h"

BEGIN_RADIANT_RPC_NAMESPACE

class TraceObjectRouter : public IRouter
{
public:
   TraceObjectRouter(dm::Store& store);

   void AddObject(std::string const& name, dm::ObjectPtr obj);

public:     // IRouter
   ReactorDeferredPtr InstallTrace(Trace const& trace) override;

private:
   ReactorDeferredPtr InstallTrace(Trace const& trace, dm::ObjectPtr obj, const char* reason);
   ReactorDeferredPtr GetTrace(Trace const& trace);

private:
   struct ObjectTraceEntry {
      dm::ObjectRef           o;
      ReactorDeferredRef      d;
      ObjectTraceEntry() {}
      ObjectTraceEntry(dm::ObjectPtr op, ReactorDeferredPtr dp) : o(op), d(dp) { } 
   };
private:
   dm::Store&                                          store_;
   std::unordered_map<std::string, ObjectTraceEntry>   traces_;
   std::unordered_map<std::string, core::Guard>          guards_;
   std::unordered_map<std::string, dm::ObjectRef>      namedObjects_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_TRACE_OBJECT_ROUTER_H
