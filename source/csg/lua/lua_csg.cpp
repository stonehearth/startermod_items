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

template <typename T> std::string repr_point(T const& obj)
{
   std::ostringstream buf;
   buf << "_radiant.csg." << GetShortTypeName<T>() << "(";
   for (int i = 0; i < T::Dimension; i++) {
      buf << obj[i];
      if (i < T::Dimension) {
         buf << ", ";
      }
   }
   buf << ")";
   return buf.str();
}

template <typename T> std::string repr_cube(T const& obj)
{
   std::ostringstream buf;
   buf << "_radiant.csg." << GetShortTypeName<T>() << "(";
   buf << lua::Repr(obj.min) << ", ";
   buf << lua::Repr(obj.min) << ")";
   return buf.str();
}

template <typename T> std::string repr_region(T const& obj)
{
   std::ostringstream buf;
   buf << "_radiant.csg." << GetShortTypeName<T>() << "(";
   buf << lua::Repr(obj.min) << ", ";
   buf << lua::Repr(obj.min) << ")";
   return buf.str();
}

#define DECLARE_TYPE(lower, T) \
   template <> std::string lua::Repr(T const& obj) { \
      return repr_ ## lower<T>(obj); \
   } \

DECLARE_TYPE(point, csg::Point1)
DECLARE_TYPE(point, csg::Point2)
DECLARE_TYPE(point, csg::Point3)
DECLARE_TYPE(point, csg::Point1f)
DECLARE_TYPE(point, csg::Point2f)
DECLARE_TYPE(point, csg::Point3f)
DECLARE_TYPE(cube,  csg::Line1)
DECLARE_TYPE(cube, csg::Rect2)
DECLARE_TYPE(cube, csg::Rect2f)
DECLARE_TYPE(cube, csg::Cube3)
DECLARE_TYPE(cube, csg::Cube3f)
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
            LuaRandomNumberGenerator::RegisterLuaTypes(L)
         ]
      ]
   ];
}
