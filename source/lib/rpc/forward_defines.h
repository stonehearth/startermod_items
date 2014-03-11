#ifndef _RADIANT_LIB_RPC_FORWARD_DEFINES_H
#define _RADIANT_LIB_RPC_FORWARD_DEFINES_H

#include "radiant_macros.h"
#include "namespace.h"

BEGIN_RADIANT_RPC_NAMESPACE

class Session;
class Function;
class Trace;
class UnTrace;
class LuaDeferred;
class LuaRouter;
class LuaObjectRouter;
class LuaModuleRouter;
class HttpDeferred;
class HttpReactor;
class ReactorDeferred;
class ProtobufReactor;
class ProtobufRouter;
class CoreReactor;
class TraceObjectRouter;
class IRouter;

DECLARE_SHARED_POINTER_TYPES(LuaDeferred)
DECLARE_SHARED_POINTER_TYPES(LuaRouter)
DECLARE_SHARED_POINTER_TYPES(LuaObjectRouter)
DECLARE_SHARED_POINTER_TYPES(LuaModuleRouter)
DECLARE_SHARED_POINTER_TYPES(HttpDeferred)
DECLARE_SHARED_POINTER_TYPES(ReactorDeferred)
DECLARE_SHARED_POINTER_TYPES(CoreReactor)
DECLARE_SHARED_POINTER_TYPES(HttpReactor)
DECLARE_SHARED_POINTER_TYPES(IRouter)
DECLARE_SHARED_POINTER_TYPES(ProtobufReactor)
DECLARE_SHARED_POINTER_TYPES(ProtobufRouter)
DECLARE_SHARED_POINTER_TYPES(TraceObjectRouter)
DECLARE_SHARED_POINTER_TYPES(Session)

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_FORWARD_DEFINES_H
