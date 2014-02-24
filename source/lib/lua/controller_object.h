#ifndef _RADIANT_LUA_CONTROLLER_OBJECT_H
#define _RADIANT_LUA_CONTROLLER_OBJECT_H

#include "bind.h"
#include "dm/dm.h"

BEGIN_RADIANT_LUA_NAMESPACE

class ControllerObject {
public:
   ControllerObject()
   {
   }

   ControllerObject(std::string const& source, luabind::object object) :
      lua_source_(source),
      lua_object_(object)
   {
   }

   std::string const& GetLuaSource() const
   {
      return lua_source_;
   }

   luabind::object GetLuaObject() const
   {
      return lua_object_;
   }

private:
   std::string       lua_source_;
   luabind::object   lua_object_;
};

DECLARE_SHARED_POINTER_TYPES(ControllerObject)

END_RADIANT_LUA_NAMESPACE

#endif

