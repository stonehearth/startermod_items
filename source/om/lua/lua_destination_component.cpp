#include "pch.h"
#include "radiant.h"
#include "radiant_macros.h"
#include "lua/register.h"
#include "lua/script_host.h"
#include "lua_om.h"
#include "lua_destination_component.h"
#include "om/components/destination.h"
#include "dm/guard.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

DeepRegionGuardLuaPtr Destination_TraceRegion(lua_State* L, Destination const& d, const char* reason)
{
   return std::make_shared<DeepRegionGuardLua>(L, d.GetRegion(), reason);
}

Region3BoxedPtr Destination_GetRegion(Destination const& d)
{
   return *d.GetRegion();
}

DestinationRef Destination_SetRegion(DestinationRef d, Region3BoxedPtr r)
{
   DestinationPtr destination = d.lock();
   if (destination) {
      destination->SetRegion(r);
   }
   return d;
}

DeepRegionGuardLuaPtr Destination_TraceAdjacent(lua_State* L, Destination const& d, const char* reason)
{
   return std::make_shared<DeepRegionGuardLua>(L, d.GetAdjacent(), reason);
}

Region3BoxedPtr Destination_GetAdjacent(Destination const& d)
{
   return *d.GetAdjacent();
}

DestinationRef Destination_SetAdjacent(DestinationRef d, Region3BoxedPtr r)
{
   DestinationPtr destination = d.lock();
   if (destination) {
      destination->SetAdjacent(r);
   }
   return d;
}

scope LuaDestinationComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<Destination, Component>()
         .def("get_region",               &Destination_GetRegion)
         .def("set_region",               &Destination_SetRegion)
         .def("trace_region",             &Destination_TraceRegion)
         .def("get_adjacent",             &Destination_GetAdjacent)
         .def("set_adjacent",             &Destination_SetAdjacent)
         .def("trace_adjacent",           &Destination_TraceAdjacent)
         .def("set_auto_update_adjacent", &Destination::SetAutoUpdateAdjacent)
      ;
}
