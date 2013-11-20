#include "pch.h"
#include "lib/lua/register.h"
#include "lua_entity_container_component.h"
#include "om/components/entity_container.ridl.h"
#include "lib/json/dm_json.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, EntityContainer::Container const& f)
{
   return f.ToString(os);
}

std::ostream& operator<<(std::ostream& os, EntityContainer::Container::LuaIterator const& f)
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
      lua::RegisterWeakGameObjectDerived<EntityContainer, Component>()
         .def("add_child",             &EntityContainer::AddChild)
         .def("remove_child",          &EntityContainer::RemoveChild)
         .def("get_children",          &EntityContainer::GetChildren)
      ,
      EntityContainer::Container::RegisterLuaType(L)
      ;
}
