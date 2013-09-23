#include "pch.h"
#include "lua/register.h"
#include "lua_entity_container_component.h"
#include "om/components/entity_container.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, EntityContainer::Container const& f)
{
   return f.ToString(os);
}

std::ostream& operator<<(std::ostream& os, EntityContainer::Container::LuaIteratorType const& f)
{
   return os << "[MapIterator]";
}

std::ostream& operator<<(std::ostream& os, EntityContainer::Container::LuaPromise const& f)
{
   return os << "[MapPromise]";
}

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
