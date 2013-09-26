#include "pch.h"
#include "lua/register.h"
#include "lua_sensor_list_component.h"
#include "om/components/sensor_list.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaSensorListComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<SensorList, Component>()
         .def("add_sensor",               &SensorList::AddSensor)
         .def("get_sensor",               &SensorList::GetSensor)
         .def("remove_sensor",            &SensorList::RemoveSensor)
      ,
      lua::RegisterObject<Sensor>()
         .def("set_name",                 &Sensor::SetName)
         .def("get_name",                 &Sensor::GetName)
         .def("get_contents",             &Sensor::GetContents)
      ,
      dm::Set<EntityId>::RegisterLuaType(L, "Set<EntityId>")
      ;
}
