#include "pch.h"
#include "lib/lua/register.h"
#include "lua_edgelist.h"
#include "csg/region_tools.h"
#include "csg/util.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

IMPLEMENT_TRIVIAL_TOSTRING(Edge3f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgePoint3f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeMap3f);
IMPLEMENT_TRIVIAL_TOSTRING(Edge3);
IMPLEMENT_TRIVIAL_TOSTRING(EdgePoint3);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeMap3);
IMPLEMENT_TRIVIAL_TOSTRING(Edge2f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgePoint2f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeMap2f);
IMPLEMENT_TRIVIAL_TOSTRING(Edge2);
IMPLEMENT_TRIVIAL_TOSTRING(EdgePoint2);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeMap2);
IMPLEMENT_TRIVIAL_TOSTRING(Edge1f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgePoint1f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeMap1f);
IMPLEMENT_TRIVIAL_TOSTRING(Edge1);
IMPLEMENT_TRIVIAL_TOSTRING(EdgePoint1);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeMap1);


template <typename S, int C>
std::vector<Edge<S, C>> const& EdgeMap_EachEdges(EdgeMap<S, C> const& map)
{
   return map.GetEdges();
}

template <typename S, int C>
static scope Register(struct lua_State* L, const char* suffix)
{
   return
      lua::RegisterTypePtr_NoTypeInfo<Edge<S, C>>(BUILD_STRING("Edge" << suffix).c_str())
         .def_readonly("min", &Edge<S, C>::min)
         .def_readonly("max", &Edge<S, C>::max)
         .def_readonly("normal", &Edge<S, C>::normal)
      ,
      lua::RegisterTypePtr_NoTypeInfo<EdgePoint<S, C>>(BUILD_STRING("EdgePoint" << suffix).c_str())
         .def_readonly("location",            &EdgePoint<S, C>::location)
         .def_readonly("accumulated_normals", &EdgePoint<S, C>::accumulated_normals)
      ,
      lua::RegisterTypePtr_NoTypeInfo<EdgeMap<S, C>>(BUILD_STRING("EdgeMap" << suffix).c_str())
         .def("each_edge",           &EdgeMap_EachEdges<S, C>, return_stl_iterator)
      ;
}

scope LuaEdgeList::RegisterLuaTypes(lua_State* L)
{
   return
      Register<double, 3>(L, "3"),
      Register<double, 2>(L, "2"),
      Register<double, 1>(L, "1"),
      Register<int, 3>(L, "3i"),
      Register<int, 2>(L, "2i"),
      Register<int, 1>(L, "1i"),
      def("convert_heightmap_to_region2", &csg::HeightmapToRegion2f)
      ;
}
