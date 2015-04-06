#include "pch.h"
#include "lib/lua/lua.h"
#include "lua_cube.h"
#include "lua_point.h"
#include "lua_region.h"
#include "lua_heightmap.h"
#include "lua_edgelist.h"
#include "lua_quaternion.h"
#include "lua_ray.h"
#include "lua_mesh.h"
#include "lua_iterator.h"
#include "lua_random_number_generator.h"
#include "csg/region.h"
#include "csg/random_number_generator.h"

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
            LuaRandomNumberGenerator::RegisterLuaTypes(L),
            LuaMesh::RegisterLuaTypes(L),
            RegisterIteratorTypes(L),
            def("get_rect_centroid", (csg::Point2f(*)(csg::Rect2f const&)) &csg::GetCentroid<double, 2>),
            def("get_cube_centroid", (csg::Point3f(*)(csg::Cube3f const&)) &csg::GetCentroid<double, 3>),
            def("get_region_centroid", (csg::Point2f(*)(csg::Region2f const&)) &csg::GetCentroid<double, 2>),
            def("get_region_centroid", (csg::Point3f(*)(csg::Region3f const&)) &csg::GetCentroid<double, 3>)
         ]
      ]
   ];
   object Point3f = globals(L)["_radiant"]["csg"]["Point3"];
   object Point2f = globals(L)["_radiant"]["csg"]["Point2"];
   Point3f["zero"] = csg::Point3f::zero;
   Point3f["one"] = csg::Point3f::one;
   Point3f["unit_x"] = csg::Point3f::unitX;
   Point3f["unit_y"] = csg::Point3f::unitY;
   Point3f["unit_z"] = csg::Point3f::unitZ;
   Point2f["zero"] = csg::Point2f::zero;
   Point2f["one"] = csg::Point2f::one;
}
