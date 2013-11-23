#ifndef _RADIANT_LUA_SIM_OPEN_H
#define _RADIANT_LUA_SIM_OPEN_H

#include "lib/lua/lua.h"
#include "simulation/namespace.h"

BEGIN_RADIANT_LUA_NAMESPACE

namespace sim {
   void open(lua_State* lua, simulation::Simulation* sim);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_SIM_OPEN_H