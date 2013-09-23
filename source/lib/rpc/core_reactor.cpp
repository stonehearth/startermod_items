#include "pch.h"
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

   LOG(INFO) << "dispatching call '" << fn << "'...";

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
            return d;
         }
      }
   }
   if (!d) {
      throw core::InvalidArgumentException(BUILD_STRING("could not dispatch " << fn));
   }
   return d;
}

ReactorDeferredPtr CoreReactor::InstallTrace(Trace const& t)
{
   ReactorDeferredPtr d;

   LOG(INFO) << "dispatching trace '" << t << "'...";
   for (auto &router : routers_) {
      d = router->InstallTrace(t);
      if (d) {
         break;
         return d;
      }
   }
   if (!d) {
      throw core::InvalidArgumentException(BUILD_STRING("could not trace" << t));
   }
   return d;
}

ReactorDeferredPtr CoreReactor::RemoveTrace(UnTrace const& u)
{
   ReactorDeferredPtr d;

   LOG(INFO) << "removing trace '" << u << "'...";
   for (auto &router : routers_) {
      d = router->RemoveTrace(u);
      if (d) {
         return d;
      }
   }
   if (!d) {
      throw core::InvalidArgumentException(BUILD_STRING("could not remove trace" << u));
   }
   return d;
}

void CoreReactor::AddRouter(IRouterPtr router)
{
   routers_.emplace_back(router);
}

void CoreReactor::AddRoute(std::string const& route, CallCb cb)
{
   auto i = routes_.find(route);
   if (i != routes_.end()) {
      LOG(WARNING) << "overwriting existing route for " << route;
      i->second = cb;
   } else {
      routes_[route] = cb;
   }
}
