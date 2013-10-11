#include "pch.h"
#include "lua/register.h"
#include "lua_region.h"
#include "om/region.h"
#include "dm/boxed.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, Region3BoxedPromise const& f)
{
   return os << "[Region3BoxedPromise]";
}

std::ostream& operator<<(std::ostream& os, Region3Boxed::Promise const& f)
{
   return os << "[Region3Boxed::Promise]";
}

scope LuaRegion::RegisterLuaTypes(lua_State* L)
{
   return
      Region3Boxed::RegisterLuaType(L)
      ,
      lua::RegisterTypePtr<Region3BoxedPromise>()
         .def("on_changed", &Region3BoxedPromise::PushChangedCb) // xxx: why isn't this called 'progress'?
      ;
}
