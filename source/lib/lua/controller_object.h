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

   ControllerObject(luabind::object object) :
      object_(object)
   {
   }

   operator luabind::object() const
   {
      return object_;
   }

private:
   luabind::object      object_;
};


END_RADIANT_LUA_NAMESPACE

IMPLEMENT_DM_NOP(radiant::lua::ControllerObject);

#endif

