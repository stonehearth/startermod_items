#include "pch.h"
#include "lib/lua/register.h"
#include "lib/lua/script_host.h"
#include "lua_unit_info_component.h"
#include "om/components/unit_info.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;


void UnitInfo_ExtendObject(lua_State* L, UnitInfo& unitInfo, luabind::object o)
{
   unitInfo.ExtendObject(lua::ScriptHost::LuaToJson(L, o));
}

dm::Object::LuaPromise<om::UnitInfo>* UnitInfo_Trace(om::UnitInfo const& db, const char* reason)
{
   return new dm::Object::LuaPromise<om::UnitInfo>(reason, db);
}

scope LuaUnitInfoComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<UnitInfo, Component>()
         .def("extend",                &UnitInfo_ExtendObject)
         .def("set_display_name",      &om::UnitInfo::SetDisplayName)
         .def("get_display_name",      &om::UnitInfo::GetDisplayName)
         .def("set_description",       &om::UnitInfo::SetDescription)
         .def("get_description",       &om::UnitInfo::GetDescription)
         .def("set_faction",           &om::UnitInfo::SetFaction)
         .def("get_faction",           &om::UnitInfo::GetFaction)
         .def("set_icon",           &om::UnitInfo::SetIcon)
         .def("get_icon",           &om::UnitInfo::GetIcon)
         .def("trace",              &UnitInfo_Trace)
      ,
      dm::Object::LuaPromise<om::UnitInfo>::RegisterLuaType(L)
      ;
}
