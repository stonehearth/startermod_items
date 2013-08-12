#include "pch.h"
#include "lua/register.h"
#include "lua_entity_container_component.h"
#include "om/components/entity_container.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaEntityContainerComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<EntityContainer, Component>()
         .def("add_child",             &EntityContainer::AddChild)
         .def("remove_child",          &EntityContainer::RemoveChild)
         .def("get_children",          &EntityContainer::GetChildren)
      ,
      EntityContainer::Container::RegisterLuaType(L)
      ;
}
