#include "pch.h"
#include "lua/register.h"
#include "lua_lua_components_component.h"
#include "om/components/lua_components.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaLuaComponentsComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<LuaComponents, Component>()
        .def("get_lua_component",   &LuaComponents::GetLuaComponent)
        .def("add_lua_component",   &LuaComponents::AddLuaComponent)
      ,
      lua::RegisterObject<LuaComponent>()
         .def("mark_changed",       &LuaComponent::MarkChanged)
      ;
}
