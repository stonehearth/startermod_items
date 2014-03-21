#ifndef _RADIANT_LIB_TYPEINFO_REGISTER_TYPE_H
#define _RADIANT_LIB_TYPEINFO_REGISTER_TYPE_H

#include "typeinfo.h"
#include "type.h"
#include "dispatcher.h"
#include "dispatch_table.h"

BEGIN_RADIANT_TYPEINFO_NAMESPACE

template <typename T>
class RegisterType
{
public:
   RegisterType(){
      ASSERT(!Type<T>::registered);
   }
   
   ~RegisterType(){
      Type<T>::registered = true;
      Dispatcher::Register(Type<T>::id, _dispatch_table);
   }

   RegisterType& SetLuaToProtobuf(DispatchTable::LuaToProtobuf fn){
      _dispatch_table.lua_to_protobuf = fn;
      return *this;
   }
   
   RegisterType& SetProtobufToLua(DispatchTable::ProtobufToLua fn){
      _dispatch_table.protobuf_to_lua = fn;
      return *this;
   }

private:
   DispatchTable    _dispatch_table;
};

END_RADIANT_TYPEINFO_NAMESPACE

#endif // _RADIANT_LIB_TYPEINFO_REGISTER_TYPE_H
