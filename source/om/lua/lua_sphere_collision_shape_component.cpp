#include "pch.h"
#include "lib/lua/register.h"
#include "lua_sphere_collision_shape_component.h"
#include "om/components/sphere_collision_shape.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaSphereCollisionShapeComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<SphereCollisionShape, Component>()
      ;
}
