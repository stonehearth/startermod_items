#include "pch.h"
#include "lua/register.h"
#include "lua_render_info_component.h"
#include "om/components/render_info.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaRenderInfoComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<RenderInfo, Component>()
         .def("set_display_iconic",       &om::RenderInfo::SetDisplayIconic)
         .def("get_display_iconic",       &om::RenderInfo::GetDisplayIconic)
      ;
}
