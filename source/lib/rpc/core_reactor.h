#ifndef _RADIANT_LIB_RPC_CORE_REACTOR_H
#define _RADIANT_LIB_RPC_CORE_REACTOR_H

#include <unordered_map>
#include "namespace.h"
#include "radiant_json.h"
#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class CoreReactor {
public:
   typedef std::function<ReactorDeferredPtr(Function const&)> CallCb;

public:
   ReactorDeferredPtr Call(Function const& fn);
   ReactorDeferredPtr InstallTrace(Trace const& t);
   ReactorDeferredPtr RemoveTrace(UnTrace const& u);

   void AddRouter(IRouterPtr router);
   void AddRoute(std::string const& route, CallCb cb);

private:
   std::string FindRouteToFunction(Function const& fn);

private:
   std::vector<IRouterPtr>                   routers_;
   std::unordered_map<std::string, CallCb>   routes_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_CORE_REACTOR_H
