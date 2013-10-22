#include "pch.h"
#include "lib/lua/register.h"
#include "lua_carry_block_component.h"
#include "om/components/carry_block.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;


void CarryBlockSetCarrying(om::CarryBlock& c, object o)
{
   if (luabind::type(o) == LUA_TNIL) {
      c.SetCarrying(om::EntityRef());
      return;
   }
   om::EntityRef e = object_cast<om::EntityRef>(o);
   auto entity = e.lock();
   if (entity) {
      c.SetCarrying(entity);
   }
}

bool CarryBlockIsCarrying(om::CarryBlock& c)
{
   return c.GetCarrying().lock() != nullptr;
}

scope LuaCarryBlockComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<CarryBlock, Component>()
         .def("get_carrying",          &om::CarryBlock::GetCarrying)
         .def("is_carrying",           &CarryBlockIsCarrying)
         .def("set_carrying",          &CarryBlockSetCarrying)
      ;
}
