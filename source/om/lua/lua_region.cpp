#include "pch.h"
#include "lua/register.h"
#include "lua_region.h"
#include "om/region.h"
#include "dm/boxed.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaRegion::RegisterLuaTypes(lua_State* L)
{
   return
      BoxedRegion3::RegisterLuaType(L)
      ,
      lua::RegisterTypePtr<BoxedRegion3Promise>()
         .def("on_changed",&BoxedRegion3Promise::PushChangedCb)
      ;
}
