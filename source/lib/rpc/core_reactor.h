#ifndef _RADIANT_LIB_RPC_CORE_REACTOR_H
#define _RADIANT_LIB_RPC_CORE_REACTOR_H

#include <unordered_map>
#include "namespace.h"
#include "lib/json/node.h"
#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class CoreReactor {
public:
   typedef std::function<ReactorDeferredPtr(Function const&)> CallCb;
   typedef std::function<bool(Function const&)> CallBCb;
   typedef std::function<void(Function const&)> CallVCb;
   typedef std::function<std::string(Function const&)> CallSCb;

public:
   ReactorDeferredPtr Call(Function const& fn);
   ReactorDeferredPtr InstallTrace(Trace const& t);
   ReactorDeferredPtr RemoveTrace(UnTrace const& u);

   void RemoveRouter(IRouterPtr router);
   void AddRouter(IRouterPtr router);
   void SetRemoteRouter(IRouterPtr router);
   void AddRoute(std::string const& route, CallCb cb);
   void AddRouteB(std::string const& route, CallBCb cb);
   void AddRouteV(std::string const& route, CallVCb cb);
   void AddRouteS(std::string const& route, CallSCb cb);

private:
   std::string FindRouteToFunction(Function const& fn);

private:
   IRouterPtr                                remoteRouter_;
   std::vector<IRouterPtr>                   routers_;
   std::unordered_map<std::string, CallCb>   routes_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_CORE_REACTOR_H
