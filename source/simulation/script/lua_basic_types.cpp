#include "pch.h"
#include "math3d.h"
#include "csg/point.h"
#include "csg/region.h"
#include "csg/heightmap.h"
#include "lua_basic_types.h"
#include "helpers.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

typedef std::shared_ptr<csg::Region3> Region3Ptr;
typedef std::shared_ptr<csg::Region2> Region2Ptr;

static Region2Ptr ProjectOntoXZ(const csg::Region3& region)
{
   Region2Ptr result = std::make_shared<csg::Region2>();
   for (const auto& cube : region.ProjectOnto(1, 0)) {
      (*result) += csg::Rect2(csg::Point2(cube.GetMin().x, cube.GetMin().z),
                              csg::Point2(cube.GetMax().x, cube.GetMax().z));
   }
   return result;
}

void LuaBasicTypes::RegisterType(lua_State* L)
{
   using namespace luabind;

   module(L) [
      csg::Region2::RegisterLuaType(L, "Region2"),
      csg::Region3::RegisterLuaType(L, "Region3"), 
      csg::Rect2::RegisterLuaType(L, "Rect2"),
      csg::Cube3::RegisterLuaType(L, "Cube3"), 
      csg::Point2::RegisterLuaType(L, "Point2"),
      csg::Point3::RegisterLuaType(L, "Point3"),
      csg::HeightMap<double>::RegisterLuaType(L, "HeightMap")
   ];

   module(L) [
      def("project_onto_xz",    &ProjectOntoXZ)
   ];

   module(L) [
      class_<math3d::point3>("RadiantPoint3")
         .def(constructor<const math3d::ipoint3 &>())
         .def(constructor<float, float, float>())
         .def(tostring(const_self))
         .def("__towatch",   &DefaultToWatch<math3d::point3>)
         .def_readwrite("x", &math3d::point3::x)
         .def_readwrite("y", &math3d::point3::y)
         .def_readwrite("z", &math3d::point3::z)
         .def("distance_to", &math3d::point3::distance)
         .def("normalize",   &math3d::point3::normalize)
         .def(self + other<const math3d::point3&>())
         .def(self - other<const math3d::point3&>())
         .def(self * other<float>())
      ,
      class_<math3d::ipoint3>("RadiantIPoint3")
         .def(constructor<>())
         .def(constructor<const math3d::point3 &>())
         .def(constructor<const math3d::ipoint3 &>())
         .def(constructor<int, int, int>())
         .def(tostring(const_self))
         .def("__towatch",   &DefaultToWatch<math3d::ipoint3>)
         .def_readwrite("x", &math3d::ipoint3::x)
         .def_readwrite("y", &math3d::ipoint3::y)
         .def_readwrite("z", &math3d::ipoint3::z)
         .def("distance_squared", &math3d::ipoint3::distanceSquared)
         .def("distance_to", &math3d::ipoint3::distanceTo)
         .def("is_adjacent_to", &math3d::ipoint3::is_adjacent_to)
         .def(self + other<const math3d::ipoint3&>())
         .def(self - other<const math3d::ipoint3&>())
      ,
      class_<math3d::aabb>("RadiantAABB")
         .def(constructor<const math3d::point3&, const math3d::point3&>())
         .def(tostring(const_self))
         .def("__towatch",   &DefaultToWatch<math3d::aabb>)
         .def_readwrite("min", &math3d::aabb::_min)
         .def_readwrite("max", &math3d::aabb::_max)
         .def("center", &math3d::aabb::GetCentroid)
      ,
      class_<math3d::ibounds3>("RadiantBounds3")
         .def(constructor<const math3d::ipoint3&, const math3d::ipoint3&>())
         .def(tostring(const_self))
         .def("__towatch",   &DefaultToWatch<math3d::ibounds3>)
         .def_readwrite("min", &math3d::ibounds3::_min)
         .def_readwrite("max", &math3d::ibounds3::_max)
         .def("center", &math3d::ibounds3::centroid)
         .def("contains", &math3d::ibounds3::contains)
         .def(const_self + other<const math3d::ipoint3&>())
   ];
}
