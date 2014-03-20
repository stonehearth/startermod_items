#ifndef _RADIANT_LIB_TYPEINFO_DISPATCH_TABLE_H
#define _RADIANT_LIB_TYPEINFO_DISPATCH_TABLE_H

#include "typeinfo.h"

BEGIN_RADIANT_TYPEINFO_NAMESPACE

class DispatchTable
{
public:
   typedef std::function<void(dm::Store const& store, luabind::object const& obj, Protocol::Value* msg, int flags)> LuaToProtobuf;
   typedef std::function<void(dm::Store const& store, Protocol::Value const& msg, luabind::object& obj, int flags)> ProtobufToLua;

public:
   LuaToProtobuf     lua_to_protobuf;
   ProtobufToLua     protobuf_to_lua;
};

END_RADIANT_TYPEINFO_NAMESPACE

#endif // _RADIANT_LIB_TYPEINFO_DISPATCH_TABLE_H
