#ifndef _RADIANT_LIB_MARSHALL_CONVERT_H
#define _RADIANT_LIB_MARSHALL_CONVERT_H

#include "marshall.h"
#include "protocols/forward_defines.h"
#include "lib/lua/bind_forward_declarations.h"
#include "dm/dm.h"

#define BEGIN_RADIANT_MARSHALL_NAMESPACE  namespace radiant { namespace marshall {
#define END_RADIANT_MARSHALL_NAMESPACE    } }

BEGIN_RADIANT_MARSHALL_NAMESPACE

class Convert
{
public:
   enum Flags {
      // xxx: get rid of dm::SaveImpl!  until then, these have to match
      REMOTE = 2,
   };
   Convert(dm::Store const& store, int flags = 0);

public:
   void ToProtobuf(luabind::object const& from, Protocol::Value* to);
   void ToLua(Protocol::Value const& from, luabind::object &to);

private:
   void ProtobufToLua(Protocol::LuaObject const& msg, luabind::object &obj);
   void LuaToProtobuf(luabind::object const &obj, Protocol::LuaObject* msg, std::vector<luabind::object> &visited);
   void ProtobufToUserdata(Protocol::Value const& msg, luabind::object& obj);
   void UserdataToProtobuf(luabind::object const& obj, Protocol::Value* msg);

private:
   dm::Store const&     store_;
   int                  flags_;
};

END_RADIANT_MARSHALL_NAMESPACE

#endif // _RADIANT_LIB_MARSHALL_CONVERT_H
