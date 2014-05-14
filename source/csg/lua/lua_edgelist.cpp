#include "pch.h"
#include "lib/lua/register.h"
#include "lua_edgelist.h"
#include "csg/region_tools.h"
#include "csg/util.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfo3f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfo3);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfo2f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfo2);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfoVector3f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfoVector3);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfoVector2f);
IMPLEMENT_TRIVIAL_TOSTRING(EdgeInfoVector2);

template <typename S, int C>
static scope Register(struct lua_State* L, const char* edge, const char* edgelist)
{
   return
      lua::RegisterTypePtr_NoTypeInfo<EdgeInfo<S, C>>(edge)
         .def_readonly("min", &EdgeInfo<S, C>::min)
         .def_readonly("max", &EdgeInfo<S, C>::max)
         .def_readonly("normal", &EdgeInfo<S, C>::normal)
      ,
      lua::RegisterTypePtr_NoTypeInfo<EdgeInfoVector<S, C>>(edgelist)
         .def("each_edge",     &EdgeInfoVector<S, C>::GetEdges, return_stl_iterator)
      ;
}

scope LuaEdgeList::RegisterLuaTypes(lua_State* L)
{
   return
      Register<float, 3>(L, "Edge3f", "EdgeList3f"),
      Register<float, 2>(L, "Edge2f", "EdgeList2f"),
      Register<int, 3>(L, "Edge3", "EdgeList3"),
      Register<int, 2>(L, "Edge2", "EdgeList2"),
      def("convert_heightmap_to_region2", &csg::HeightmapToRegion2)
   ;
}
