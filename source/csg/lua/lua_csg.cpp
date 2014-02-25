#include "pch.h"
#include "lib/lua/lua.h"
#include "lua_cube.h"
#include "lua_point.h"
#include "lua_region.h"
#include "lua_heightmap.h"
#include "lua_edgelist.h"
#include "lua_quaternion.h"
#include "lua_ray.h"
#include "lua_random_number_generator.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

void csg::RegisterLuaTypes(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("csg") [
            LuaCube::RegisterLuaTypes(L),
            LuaPoint::RegisterLuaTypes(L),
            LuaRegion::RegisterLuaTypes(L),
            LuaHeightmap::RegisterLuaTypes(L),
            LuaEdgeList::RegisterLuaTypes(L),
            LuaQuaternion::RegisterLuaTypes(L),
            LuaRay::RegisterLuaTypes(L),
            LuaRandomNumberGenerator::RegisterLuaTypes(L)
         ]
      ]
   ];
}
