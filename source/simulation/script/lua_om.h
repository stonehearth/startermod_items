#ifndef _RADIANT_SIMULATION_LUA_OBJECT_MODEL_H
#define _RADIANT_SIMULATION_LUA_OBJECT_MODEL_H

#include "namespace.h"

struct lua_State;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Simulation;

class LuaObjectModel {
public:
   static void RegisterType(lua_State* state);
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_OBJECT_MODEL_H