#include "pch.h"
#include "lua/register.h"
#include "lua_render_region_component.h"
#include "om/components/render_region.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::shared_ptr<Region3BoxedPromise> RenderRegion_TraceRenderRegion(om::RenderRegion const& region, const char* reason)
{
   return std::make_shared<Region3BoxedPromise>(region.GetRegion(), reason);
}

scope LuaRenderRegionComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<RenderRegion, Component>()
         .def("get_region",    &om::RenderRegion::GetRegion)
         .def("set_region",    &om::RenderRegion::SetRegion)
         .def("trace_region",  &RenderRegion_TraceRenderRegion)
      ;
}
