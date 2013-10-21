#include "pch.h"
#include "lua/register.h"
#include "lua_vertical_pathing_region_component.h"
#include "om/components/vertical_pathing_region.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;


DeepRegionGuardLuaPtr VerticalPathingRegion_TraceRegion(lua_State* L, VerticalPathingRegion const& v, const char* reason)
{
   return std::make_shared<DeepRegionGuardLua>(L, v.GetRegion(), reason);
}

Region3BoxedPtr VerticalPathingRegion_GetRegion(VerticalPathingRegion const& v)
{
   return *v.GetRegion();
}

VerticalPathingRegionRef VerticalPathingRegion_SetRegion(VerticalPathingRegionRef v, Region3BoxedPtr r)
{
   VerticalPathingRegionPtr vpr = v.lock();
   if (vpr) {
      vpr->SetRegion(r);
   }
   return v;
}

VerticalPathingRegionRef VerticalPathingRegion_SetNormal(VerticalPathingRegionRef v, csg::Point3 const& normal)
{
   VerticalPathingRegionPtr vpr = v.lock();
   if (vpr) {
      vpr->SetNormal(normal);
   }
   return v;
}

scope LuaVerticalPathingRegionComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<VerticalPathingRegion, Component>()
         .def("get_region",               &VerticalPathingRegion_GetRegion)
         .def("set_region",               &VerticalPathingRegion_SetRegion)
         .def("trace_region",             &VerticalPathingRegion_TraceRegion)
         .def("get_normal",               &VerticalPathingRegion::GetNormal)
         .def("set_normal",               &VerticalPathingRegion_SetNormal)
      ;
}
