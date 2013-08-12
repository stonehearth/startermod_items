#include "pch.h"
#include "lua/register.h"
#include "lua_render_rig_iconic_component.h"
#include "om/components/render_rig.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaRenderRigIconicComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<RenderRigIconic, Component>()
         .def("add_rig",               &om::RenderRig::AddRig)
         .def("remove_rig",            &om::RenderRig::RemoveRig)
         .def("set_animation_table",   &om::RenderRig::SetAnimationTable)
         .def("get_animation_table",   &om::RenderRig::GetAnimationTable)
         .def("get_rigs",              &om::RenderRig::GetRigs)
      ;
}
