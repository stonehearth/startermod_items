#ifndef _RADIANT_LIB_RPC_HTTP_REACTOR_H
#define _RADIANT_LIB_RPC_HTTP_REACTOR_H

#include <unordered_map>
#include "namespace.h"
#include "lib/json/node.h"
#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class HttpReactor {
public:
   HttpReactor(CoreReactor &core);

   ReactorDeferredPtr Call(json::Node const& query, std::string const& postdata);
   ReactorDeferredPtr InstallTrace(Trace const& t);

   bool HttpGetFile(std::string const& uri, int &code, std::string& content, std::string& mimetype);
   void QueueEvent(std::string type, JSONNode payload);
   void FlushEvents();

private:
   ReactorDeferredPtr GetEvents(rpc::Function const& f);
   ReactorDeferredPtr CreateDeferredResponse(Function const& fn, ReactorDeferredPtr d);

private:
   CoreReactor&            core_;
   ReactorDeferredPtr      get_events_deferred_;
   JSONNode                queued_events_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_H
