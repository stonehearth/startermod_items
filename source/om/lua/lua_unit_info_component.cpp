#include "pch.h"
#include "lua/register.h"
#include "lua_unit_info_component.h"
#include "om/components/unit_info.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaUnitInfoComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<UnitInfo, Component>()
         .def("set_display_name",      &om::UnitInfo::SetDisplayName)
         .def("get_display_name",      &om::UnitInfo::GetDisplayName)
         .def("set_description",       &om::UnitInfo::SetDescription)
         .def("get_description",       &om::UnitInfo::GetDescription)
         .def("set_faction",           &om::UnitInfo::SetFaction)
         .def("get_faction",           &om::UnitInfo::GetFaction)
         .def("set_icon",           &om::UnitInfo::SetIcon)
         .def("get_icon",           &om::UnitInfo::GetIcon)
      ;
}
