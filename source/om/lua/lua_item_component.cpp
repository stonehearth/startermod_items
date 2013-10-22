#include "pch.h"
#include "lib/lua/register.h"
#include "lua_item_component.h"
#include "om/components/item.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaItemComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<Item, Component>()
         .def("set_stacks",            &om::Item::SetStacks)
         .def("get_stacks",            &om::Item::GetStacks)
         .def("set_max_stacks",        &om::Item::SetMaxStacks)
         .def("get_max_stacks",        &om::Item::GetMaxStacks)
         .def("get_material",          &om::Item::GetMaterial)
         .def("set_material",          &om::Item::SetMaterial)
         .def("set_category",          &om::Item::SetCategory)
         .def("get_category",          &om::Item::GetCategory)
      ;
}
