#ifndef _RADIANT_SIMULATION_LUA_JOBS_H
#define _RADIANT_SIMULATION_LUA_JOBS_H

#include "namespace.h"

struct lua_State;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Simulation;

class LuaJobs {
public:
   static void RegisterType(lua_State* state);
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_JOBS_H