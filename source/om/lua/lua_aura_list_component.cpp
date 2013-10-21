#include "pch.h"
#include "lua/register.h"
#include "lua_aura_list_component.h"
#include "om/components/aura_list.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaAuraListComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<AuraList, Component>()
         .def("create_aura",              &om::AuraList::CreateAura)
         .def("get_aura",                 &om::AuraList::GetAura)
      ;
}
