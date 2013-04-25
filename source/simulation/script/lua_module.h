#ifndef _RADIANT_SIMULATION_LUA_MODULE_H
#define _RADIANT_SIMULATION_LUA_MODULE_H

#include "namespace.h"

struct lua_State;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class ScriptHost;

class LuaModule {
public:
   LuaModule(lua_State* state);
   ~LuaModule();

private:
   static void RegisterType(lua_State* state);
   void HelloWorld(lua_State* state);

private:
   lua_State*     state_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_MODULE_H