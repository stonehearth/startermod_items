#include "pch.h"
#include "lua/register.h"
#include "lua_region_collision_shape_component.h"
#include "om/components/region_collision_shape.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;


// xxx: put this stuff a template somewhere (see lua_destination.cpp)
DeepRegionGuardLuaPtr RegionCollisionShape_TraceRegion(lua_State* L, RegionCollisionShape const& r, const char* reason)
{
   return std::make_shared<DeepRegionGuardLua>(L, r.GetRegion(), reason);
}

Region3BoxedPtr RegionCollisionShape_GetRegion(RegionCollisionShape const& r)
{
   return *r.GetRegion();
}

RegionCollisionShapeRef RegionCollisionShape_SetRegion(RegionCollisionShapeRef r, Region3BoxedPtr region)
{
   RegionCollisionShapePtr rcs = r.lock();
   if (rcs) {
      rcs->SetRegion(region);
   }
   return r;
}

scope LuaRegionCollisionShapeComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<RegionCollisionShape, Component>()
         .def("get_region",               &RegionCollisionShape_GetRegion)
         .def("set_region",               &RegionCollisionShape_SetRegion)
         .def("trace_region",             &RegionCollisionShape_TraceRegion)
      ;
}
