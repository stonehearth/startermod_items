#include "pch.h"
extern "C" {
#  include "lua.h"
}
#include "luabind/luabind.hpp"
#include "lua_module.h"
#include "script_host.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

void LuaModule::RegisterType(lua_State* L)
{
   using namespace luabind;
   module(L) [
      class_<LuaModule>("RadiantModule")
         .def("hello_world", &LuaModule::HelloWorld)
   ];
}

LuaModule::LuaModule(lua_State* state) :
   state_(state)
{
}

void LuaModule::HelloWorld(lua_State* L)
{
   ScriptHost *host;
   lua_pushstring(L, "RadiantScriptHost");
   lua_gettable(L, LUA_REGISTRYINDEX);
   host = (ScriptHost *)lua_touserdata(L, -1);
}
