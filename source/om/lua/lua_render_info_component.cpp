#include "pch.h"
#include "lua/register.h"
#include "lua_render_info_component.h"
#include "om/components/render_info.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::string RenderInfo_GetModelVariant(om::RenderInfo const& ri)
{
   return *ri.GetModelVariant();
}

void RenderInfo_SetModelVariant(om::RenderInfo& ri, std::string const& mode)
{
   ri.GetModelVariant() = mode;
}

scope LuaRenderInfoComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<RenderInfo, Component>()
         .def("set_model_variant",       &RenderInfo_SetModelVariant)
         .def("get_model_variant",       &RenderInfo_GetModelVariant)
         .def("attach_entity",           &RenderInfo::AttachEntity)
         .def("remove_entity",           &RenderInfo::RemoveEntity)
         .def("get_animation_table_name",&RenderInfo::GetAnimationTable)
      ;
}
