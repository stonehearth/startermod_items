#ifndef _RADIANT_LIB_RPC_REACTOR_REQUEST_H
#define _RADIANT_LIB_RPC_REACTOR_REQUEST_H

#include <atomic>
#include "namespace.h"
#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class Request {
public:
   Request() :
      call_id(next_call_id_++)
   {
   }

   Request(SessionPtr s, int id) :
      caller(s),
      call_id(id)
   {
   }

private:
   static std::atomic<int>  next_call_id_;

public:
   SessionPtr           caller;
   int                  call_id;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_REQUEST_H
