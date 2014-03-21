#ifndef _RADIANT_LIB_TYPEINFO_TYPE_DISPATCHER_H
#define _RADIANT_LIB_TYPEINFO_TYPE_DISPATCHER_H

#include "typeinfo.h"

BEGIN_RADIANT_TYPEINFO_NAMESPACE

class Dispatcher
{
public:
   Dispatcher(dm::Store const& store, int flags = 0);

   bool LuaToProto(TypeId typeId, luabind::object const& obj, Protocol::Value* msg);   
   bool ProtoToLua(TypeId typeId, Protocol::Value const& msg, luabind::object& obj);

public:
   static void Dispatcher::Register(TypeId id, DispatchTable const& dispatch);

private:
   dm::Store const&     store_;
   int                  flags_;
};

END_RADIANT_TYPEINFO_NAMESPACE

#endif // _RADIANT_LIB_TYPEINFO_TYPE_DISPATCHER_H
