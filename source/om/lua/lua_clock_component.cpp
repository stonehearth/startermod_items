#include "pch.h"
#include "lua/register.h"
#include "lua_clock_component.h"
#include "om/components/clock.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaClockComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<Clock, Component>()
         .def("set_time",              &Clock::SetTime)
         .def("get_time",              &Clock::GetTime)
      ;
}
