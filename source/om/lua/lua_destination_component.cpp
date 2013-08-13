#include "pch.h"
#include "lua/register.h"
#include "lua_destination_component.h"
#include "om/components/destination.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaDestinationComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<Destination, Component>()
         .def("get_region",   &Destination::GetRegion)
         .def("set_region",   &Destination::SetRegion)
      ;
}
