#include "pch.h"
#include "lib/lua/register.h"
#include "lua_target_tables_component.h"
#include "om/components/target_tables.ridl.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaTargetTablesComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<TargetTables, Component>()
         .def("add_table",                &TargetTables::AddTable)
         .def("remove_table",             &TargetTables::RemoveTable)
         .def("get_top",                  &TargetTables::GetTop)
      ,
      lua::RegisterWeakGameObject<TargetTableEntry>()
         .def("get_target",               &TargetTableEntry::GetTarget)
         .def("get_value",                &TargetTableEntry::GetValue)
         .def("get_expire_time",          &TargetTableEntry::GetExpireTime)
         .def("set_value",                &TargetTableEntry::SetValue)
         //.def("set_value",                &TargetTableEntrySetValue)
         .def("set_expire_time",          &TargetTableEntry::SetExpireTime)
      ,
      lua::RegisterWeakGameObject<TargetTable>()
         .def("get_entry",                &TargetTable::GetEntry)
         .def("add_entry",                &TargetTable::AddEntry)
         .def("set_name",                 &TargetTable::SetName)
      ,
      lua::RegisterWeakGameObject<TargetTableGroup>()
         .def("add_table",                &TargetTableGroup::AddTable)
      ,
      class_<TargetTableTop, TargetTableTopPtr>("TargetTableTop")
         .def_readonly("target",      &TargetTableTop::target)
         .def_readonly("expires",     &TargetTableTop::expires)
         .def_readonly("value",       &TargetTableTop::value)
      ;
}
