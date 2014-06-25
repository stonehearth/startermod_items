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
#include "csg/region.h"
#include "csg/random_number_generator.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T> std::string repr_point(T const& obj, const char* tname)
{
   std::ostringstream buf;
   buf << "_radiant.csg." << tname << "(";
   for (int i = 0; i < T::Dimension; i++) {
      buf << obj[i];
      if (i < T::Dimension - 1) {
         buf << ", ";
      }
   }
   buf << ")";
   return buf.str();
}

template <typename T> std::string repr_cube(T const& obj, const char* tname)
{
   std::ostringstream buf;
   buf << "_radiant.csg." << tname << "(";
   buf << lua::Repr(obj.min) << ", ";
   buf << lua::Repr(obj.min) << ")";
   return buf.str();
}

template <typename T> std::string repr_region(T const& obj, const char* tname)
{
   std::ostringstream buf;
   buf << "_radiant.csg." << tname << "(";
   buf << lua::Repr(obj.min) << ", ";
   buf << lua::Repr(obj.min) << ")";
   return buf.str();
}

#define DECLARE_TYPE(lower, T) \
   template <> std::string lua::Repr(T const& obj) { \
      return repr_ ## lower<T>(obj, #T); \
   } \

DECLARE_TYPE(point, Point1)
DECLARE_TYPE(point, Point2)
DECLARE_TYPE(point, Point3)
DECLARE_TYPE(point, Point1f)
DECLARE_TYPE(point, Point2f)
DECLARE_TYPE(point, Point3f)
DECLARE_TYPE(cube,  Line1)
DECLARE_TYPE(cube, Rect2)
DECLARE_TYPE(cube, Rect2f)
DECLARE_TYPE(cube, Cube3)
DECLARE_TYPE(cube, Cube3f)
DEFINE_INVALID_LUA_CONVERSION(csg::Region1)
DEFINE_INVALID_LUA_CONVERSION(csg::Region2)
DEFINE_INVALID_LUA_CONVERSION(csg::Region2f)
DEFINE_INVALID_LUA_CONVERSION(csg::Region3)
DEFINE_INVALID_LUA_CONVERSION(csg::Region3f)
DEFINE_INVALID_LUA_CONVERSION(csg::Color3)
DEFINE_INVALID_LUA_CONVERSION(csg::Color4)
DEFINE_INVALID_LUA_CONVERSION(csg::Cube3::PointIterator)
DEFINE_INVALID_LUA_CONVERSION(csg::Transform)
DEFINE_INVALID_LUA_CONVERSION(csg::Quaternion)
DEFINE_INVALID_LUA_CONVERSION(csg::Ray3)
DEFINE_INVALID_LUA_CONVERSION(csg::RandomNumberGenerator)

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
            def("get_cube_centroid", (csg::Point3f(*)(csg::Cube3 const&)) &csg::GetCentroid<int, 3>),
            def("get_region_centroid", (csg::Point3f(*)(csg::Region3 const&)) &csg::GetCentroid<int, 3>)
         ]
      ]
   ];
   object Point3 = globals(L)["_radiant"]["csg"]["Point3"];
   object Point3f = globals(L)["_radiant"]["csg"]["Point3f"];
   object Point2 = globals(L)["_radiant"]["csg"]["Point2"];
   object Point2f = globals(L)["_radiant"]["csg"]["Point2f"];
   Point3["zero"] = csg::Point3::zero;
   Point3["one"] = csg::Point3::one;
   Point3f["zero"] = csg::Point3f::zero;
   Point3f["one"] = csg::Point3f::one;
   Point2["zero"] = csg::Point2::zero;
   Point2["one"] = csg::Point2::one;
   Point2f["zero"] = csg::Point2f::zero;
   Point2f["one"] = csg::Point2f::one;
}
