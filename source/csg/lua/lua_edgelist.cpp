#include "pch.h"
#include "lua/register.h"
#include "lua_edgelist.h"
#include "csg/util.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

static EdgeListPtr Region2ToEdgeListUnclipped(Region2 const& rgn)
{
   return Region2ToEdgeList(rgn, INT_MIN, Region3());
}

static std::shared_ptr<Region2> EdgeListToRegion2Unclipped(EdgeListPtr edges, int width)
{
   return std::make_shared<Region2>(EdgeListToRegion2(edges, width, nullptr));
}

scope LuaEdgeList::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterTypePtr<EdgePoint>()
         .def_readonly("x",      &EdgePoint::x)
         .def_readonly("y",      &EdgePoint::y)
      ,
      lua::RegisterTypePtr<Edge>()
         .def_readonly("start",     &Edge::start)
         .def_readonly("end",       &Edge::end)
         .def_readonly("normal",    &Edge::normal)
      ,
      lua::RegisterTypePtr<EdgeList>()
         .def_readonly("edges",     &EdgeList::edges,    return_stl_iterator)
         .def_readonly("points",    &EdgeList::points,   return_stl_iterator)
         .def("inset",              &EdgeList::Inset)
         .def("grow",               &EdgeList::Grow)
         .def("fragment",           &EdgeList::Fragment)
      ,
         def("region2_to_edge_list", &Region2ToEdgeListUnclipped),
         def("edge_list_to_region2", &EdgeListToRegion2Unclipped),
         def("convert_heightmap_to_region2", &csg::HeightmapToRegion2)
   ;
}
