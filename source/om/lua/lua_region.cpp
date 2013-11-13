#include "pch.h"
#include "lib/lua/register.h"
#include "lua_region.h"
#include "om/region.h"
#include "dm/boxed.h"
#include "lib/json/core_json.h"
#include "lib/json/dm_json.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

BEGIN_RADIANT_JSON_NAMESPACE
DEFINE_INVALID_JSON_CONVERSION(Region3BoxedPromise)
END_RADIANT_JSON_NAMESPACE


std::ostream& operator<<(std::ostream& os, Region2Boxed::Promise const& f)
{
   return os << "[Region2Boxed::Promise]";
}

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
      Region2Boxed::RegisterLuaType(L)
      ,
      Region3Boxed::RegisterLuaType(L)
      ,
      lua::RegisterTypePtr<Region3BoxedPromise>()
         .def("on_changed", &Region3BoxedPromise::PushChangedCb)
      ;
}
