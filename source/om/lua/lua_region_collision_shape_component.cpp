#include "pch.h"
#include "lua/register.h"
#include "lua_region_collision_shape_component.h"
#include "om/components/region_collision_shape.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::shared_ptr<BoxedRegion3Promise> RegionCollisionShape_TraceRegion(om::RegionCollisionShape const& c, const char* reason)
{
   return std::make_shared<BoxedRegion3Promise>(c.GetRegionField(), reason);
}

scope LuaRegionCollisionShapeComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<RegionCollisionShape, Component>()
         .def("set_region",      &om::RegionCollisionShape::SetRegionPtr)
         .def("get_region",      &om::RegionCollisionShape::GetRegionPtr)
         .def("trace_region",    &RegionCollisionShape_TraceRegion)
      ;
}
