#include "pch.h"
#include "lib/lua/register.h"
#include "lua_effect_list_component.h"
#include "om/components/effect_list.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaEffectListComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObject<Effect>()
         .def("get_name",              &Effect::GetName)
         .def("get_start_time",        &Effect::GetStartTime)
         .def("add_param",             &Effect::AddParam)
         .def("get_param",             &Effect::GetParam)
      ,
      lua::RegisterWeakGameObjectDerived<EffectList, Component>()
         .def("add_effect",            &EffectList::AddEffect)
         .def("remove_effect",         &EffectList::RemoveEffect)
      ;
}
