#ifndef _RADIANT_LIB_RPC_IENDPOINT_H
#define _RADIANT_LIB_RPC_IENDPOINT_H

#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class IRouter {
public:
   virtual ReactorDeferredPtr Call(Function const& fn) { return nullptr; }
   virtual ReactorDeferredPtr InstallTrace(Trace const& trace) { return nullptr; }
   virtual ReactorDeferredPtr RemoveTrace(UnTrace const& u) { return nullptr; }
};

DECLARE_SHARED_POINTER_TYPES(IRouter)

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_IENDPOINT_H
