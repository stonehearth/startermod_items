#ifndef _RADIANT_LIB_RPC_REACTOR_DEFERRED_H
#define _RADIANT_LIB_RPC_REACTOR_DEFERRED_H

#include <libjson.h>
#include "namespace.h"
#include "core/deferred.h"

BEGIN_RADIANT_RPC_NAMESPACE

class ReactorDeferred : public core::Deferred<JSONNode, JSONNode>
{
public:
   ReactorDeferred(std::string const& dbg_msg) : core::Deferred<JSONNode, JSONNode>(dbg_msg) { }

   void ResolveWithMsg(std::string const& msg) {
      JSONNode ok;
      ok.push_back(JSONNode("success", "true"));
      ok.push_back(JSONNode("msg", msg));
      Resolve(ok);
   }

   void RejectWithMsg(std::string const &s) {
      JSONNode err;
      err.push_back(JSONNode("error", s));
      Reject(err);      
   }

   void RejectWithException(std::exception const& e) {
      RejectWithMsg(e.what());
   }
};

DECLARE_SHARED_POINTER_TYPES(ReactorDeferred)

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_DEFERRED_H
