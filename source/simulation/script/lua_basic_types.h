#ifndef _RADIANT_SIMULATION_LUA_BASIC_TYPES_H
#define _RADIANT_SIMULATION_LUA_BASIC_TYPES_H

#include "namespace.h"
#include "math3d.h"

struct lua_State;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class LuaBasicTypes {
public:
   static void RegisterType(lua_State* state);
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_BASIC_TYPES_H