#include "pch.h"
#include "lua/register.h"
#include "lua_attributes_component.h"
#include "om/components/attributes.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaAttributesComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<Attributes, Component>()
         .def("get_attribute",            &om::Attributes::GetAttribute)
         .def("has_attribute",            &om::Attributes::HasAttribute)
         .def("set_attribute",            &om::Attributes::SetAttribute)
      ;
}
