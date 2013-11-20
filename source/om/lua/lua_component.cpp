#include "pch.h"
#include "lib/lua/register.h"
#include "lua_component.h"
#include "om/components/component.ridl.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaComponent_::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObject<Component>()
         .def("get_entity",     &Component::GetEntityRef)
         .def("extend",         &Component::ExtendObject)
      ;
}
