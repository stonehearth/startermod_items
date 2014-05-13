#ifndef _RADIANT_LUA_PHYSICS_OPEN_H
#define _RADIANT_LUA_PHYSICS_OPEN_H

#include "lib/lua/lua.h"
#include "physics/namespace.h"

BEGIN_RADIANT_LUA_NAMESPACE

namespace phys {
   void open(lua_State* lua, ::radiant::phys::OctTree& octtree);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_SIM_OPEN_H