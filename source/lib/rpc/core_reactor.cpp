#include "pch.h"
#include "radiant_stdutil.h"
#include "radiant_exceptions.h"
#include "core_reactor.h"
#include "reactor_deferred.h"
#include "trace.h"
#include "untrace.h"
#include "function.h"
#include "irouter.h"
#include "resources/manifest.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

ReactorDeferredPtr CoreReactor::Call(Function const& fn)
{
   ReactorDeferredPtr d;

   RPC_LOG(5) << "dispatching call '" << fn << "'...";

   if (fn.object.empty()) {
      auto i = routes_.find(fn.route);
      if (i != routes_.end()) {
         d = i->second(fn);
      }
   }
   if (!d) {
      for (auto &router : routers_) {
         d = router->Call(fn);
         if (d) {
            break;
         }
      }
   }
   if (!d && remoteRouter_) {
      d = remoteRouter_->Call(fn);
   }
   if (!d) {
      throw core::InvalidArgumentException(BUILD_STRING("could not dispatch " << fn));
   }
   return d;
}

ReactorDeferredPtr CoreReactor::InstallTrace(Trace const& t)
{
   ReactorDeferredPtr d;

   RPC_LOG(5) << "dispatching trace '" << t << "'...";
   for (auto &router : routers_) {
      d = router->InstallTrace(t);
      if (d) {
         break;
      }
   }
   if (!d && remoteRouter_) {
      d = remoteRouter_->InstallTrace(t);
   }
   if (!d) {
      throw core::InvalidArgumentException(BUILD_STRING("could not trace" << t));
   }
   return d;
}

ReactorDeferredPtr CoreReactor::RemoveTrace(UnTrace const& u)
{
   ReactorDeferredPtr d;

   RPC_LOG(5) << "removing trace '" << u << "'...";
   for (auto &router : routers_) {
      d = router->RemoveTrace(u);
      if (d) {
         return d;
      }
   }
   if (!d && remoteRouter_) {
      d = remoteRouter_->RemoveTrace(u);
   }
   if (!d) {
      throw core::InvalidArgumentException(BUILD_STRING("could not remove trace" << u));
   }
   return d;
}

void CoreReactor::RemoveRouter(IRouterPtr router)
{
   stdutil::UniqueRemove(routers_, router);
}

void CoreReactor::AddRouter(IRouterPtr router)
{
   routers_.emplace_back(router);
}

void CoreReactor::AddRoute(std::string const& route, CallCb cb)
{
   auto i = routes_.find(route);
   if (i != routes_.end()) {
      RPC_LOG(3) << "overwriting existing route for " << route;
      i->second = cb;
   } else {
      routes_[route] = cb;
   }
}

void CoreReactor::AddRouteB(std::string const& route, CallBCb cb)
{
   AddRoute(route, [cb](Function const& f) -> ReactorDeferredPtr {
      std::string msg;
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>(f.route);
      if (cb(f)) {
         d->ResolveWithMsg(BUILD_STRING(f.route << " returned true"));
      } else {
         d->RejectWithMsg(BUILD_STRING(f.route << " returned false"));
      }
      return d;
   });
}

void CoreReactor::AddRouteV(std::string const& route, CallVCb cb)
{
   AddRoute(route, [cb](Function const& f) -> ReactorDeferredPtr {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>(f.route);
      cb(f);
      d->ResolveWithMsg(BUILD_STRING(f.route << " completed"));
      return d;
   });
}

void CoreReactor::AddRouteS(std::string const& route, CallSCb cb)
{
   AddRoute(route, [cb](Function const& f) -> ReactorDeferredPtr {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>(f.route);
      d->ResolveWithMsg(cb(f));
      return d;
   });
}

void CoreReactor::AddRouteJ(std::string const& route, CallJCb cb)
{
   AddRoute(route, [cb](Function const& f) -> ReactorDeferredPtr {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>(f.route);
      d->Resolve(cb(f));
      return d;
   });
}

void CoreReactor::SetRemoteRouter(IRouterPtr router)
{
   remoteRouter_ = router;
}

