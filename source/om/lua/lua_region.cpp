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

#if 0
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
#endif

scope LuaRegion::RegisterLuaTypes(lua_State* L)
{
   return
      Region2Boxed::RegisterLuaType(L),
      Region3Boxed::RegisterLuaType(L)
      ;
}
