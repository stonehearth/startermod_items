#ifndef _RADIANT_SIMULATION_LUA_RESOURCE_H
#define _RADIANT_SIMULATION_LUA_RESOURCE_H

#include "namespace.h"
#include "luabind/luabind.hpp"
#include "resources/resource.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class LuaResource
{
public:
   static void RegisterType(lua_State* L);
   static luabind::object ConvertResourceToLua(lua_State* L, std::shared_ptr<resources::Resource> obj);
};


END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_RESOURCE_H