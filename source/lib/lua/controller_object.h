#ifndef _RADIANT_LUA_CONTROLLER_OBJECT_H
#define _RADIANT_LUA_CONTROLLER_OBJECT_H

#include "bind.h"
#include "dm/dm.h"

BEGIN_RADIANT_LUA_NAMESPACE

class ControllerObject {
public:
   ControllerObject();
   ControllerObject(std::string const& source, luabind::object object);

   std::string const& GetLuaSource() const;
   luabind::object GetLuaObject() const;
   void SetLuaObject(luabind::object obj) const;
   void DestroyLuaObject() const;

   void SaveValue(Protocol::LuaControllerObject *msg) const;
   void LoadValue(const Protocol::LuaControllerObject &msg);

private:
   std::string                lua_source_;
   mutable luabind::object    lua_object_;
};

std::ostream& operator<<(std::ostream& os, ControllerObject const& o);

DECLARE_SHARED_POINTER_TYPES(ControllerObject)

END_RADIANT_LUA_NAMESPACE


#endif

