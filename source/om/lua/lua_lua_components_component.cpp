#include "pch.h"
#include "lib/lua/register.h"
#include "lua_lua_components_component.h"
#include "om/components/lua_components.ridl.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaLuaComponentsComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<LuaComponents, Component>()
        .def("get_lua_component",   &LuaComponents::GetLuaComponent)
        .def("add_lua_component",   &LuaComponents::AddLuaComponent)
      ;
}
