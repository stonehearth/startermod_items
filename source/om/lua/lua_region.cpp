#include "pch.h"
#include "lua/register.h"
#include "lua_region.h"
#include "om/region.h"
#include "dm/boxed.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, BoxedRegion3Promise const& f)
{
   return os << "[BoxedRegion3Promise]";
}

std::ostream& operator<<(std::ostream& os, BoxedRegion3::Promise const& f)
{
   return os << "[BoxedRegion3::Promise]";
}

scope LuaRegion::RegisterLuaTypes(lua_State* L)
{
   return
      BoxedRegion3::RegisterLuaType(L)
      ,
      lua::RegisterTypePtr<BoxedRegion3Promise>()
         .def("on_changed", &BoxedRegion3Promise::PushChangedCb) // xxx: why isn't this called 'progress'?
      ;
}
