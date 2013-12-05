#include "pch.h"
#include "lib/lua/register.h"
#include "om/lua/lua_component.h"
#include "om/components/component.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaComponent_::RegisterLuaTypes(lua_State* L)
{
   return luabind::class_<Component, std::weak_ptr<Component>>("Component");
}