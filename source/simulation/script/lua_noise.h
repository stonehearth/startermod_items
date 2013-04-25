#ifndef _RADIANT_SIMULATION_LUA_NOISE_H
#define _RADIANT_SIMULATION_LUA_NOISE_H

#include "namespace.h"
#include "math3d.h"

struct lua_State;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class LuaNoise {
public:
   static void RegisterType(lua_State* state);
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_NOISE_H