#ifndef _RADIANT_LIB_RPC_TRACE_OBJECT_ROUTER_H
#define _RADIANT_LIB_RPC_TRACE_OBJECT_ROUTER_H

#include <unordered_map>
#include "irouter.h"
#include "forward_defines.h"
#include "protocols/tesseract.pb.h"
#include "dm/dm.h"

BEGIN_RADIANT_RPC_NAMESPACE

class TraceObjectRouter : public IRouter
{
public:
   TraceObjectRouter(dm::Store& store);

   void CheckDeferredTraces();

public:     // IRouter
   ReactorDeferredPtr InstallTrace(Trace const& trace) override;

private:
   struct ObjectTraceEntry {
      dm::ObjectRef           obj;
      ReactorDeferredRef      deferred;
      dm::TracePtr            trace;
   };

private:
   ReactorDeferredPtr GetTrace(Trace const& trace);
   void InstallTrace(std::string const& uri, ReactorDeferredPtr deferred, dm::ObjectPtr obj);

private:
   dm::Store&                                            store_;
   std::unordered_map<std::string, ObjectTraceEntry>     traces_;
   std::unordered_map<std::string, ReactorDeferredRef>   deferred_traces_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_TRACE_OBJECT_ROUTER_H
