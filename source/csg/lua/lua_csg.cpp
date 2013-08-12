#include "pch.h"
#include "lua_cube.h"
#include "lua_point.h"
#include "lua_region.h"
#include "lua_heightmap.h"
#include "lua_voronoi.h"
#include "lua_edgelist.h"

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
            LuaVoronoi::RegisterLuaTypes(L),
            LuaEdgeList::RegisterLuaTypes(L)
         ]
      ]
   ];
}
