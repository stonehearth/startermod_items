#ifndef _RADIANT_RPC_NAMESPACE_H
#define _RADIANT_RPC_NAMESPACE_H

#define BEGIN_RADIANT_RPC_NAMESPACE  namespace radiant { namespace rpc {
#define END_RADIANT_RPC_NAMESPACE    } }

#define RADIANT_RPC_NAMESPACE    ::radiant::rpc

#define IN_RADIANT_RPC_NAMESPACE(x) \
   BEGIN_RADIANT_RPC_NAMESPACE \
   x  \
   END_RADIANT_RPC_NAMESPACE

#define RPC_LOG(level)     LOG(rpc, level)

#endif //  _RADIANT_RPC_NAMESPACE_H
