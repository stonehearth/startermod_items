#include "pch.h"
#include "register.h"
#include "radiant_luabind.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::lua;

void ::radiant::lua::RegisterBasicTypes(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("math3d") [
            lua::RegisterType<math3d::point3>("RadiantPoint3")
               .def(constructor<const math3d::ipoint3 &>())
               .def(constructor<float, float, float>())
               .def(tostring(const_self))
               .def_readwrite("x", &math3d::point3::x)
               .def_readwrite("y", &math3d::point3::y)
               .def_readwrite("z", &math3d::point3::z)
               .def("distance_to", &math3d::point3::distance)
               .def("normalize",   &math3d::point3::normalize)
               .def(self + other<const math3d::point3&>())
               .def(self - other<const math3d::point3&>())
               .def(self * other<float>())
            ,
            lua::RegisterType<math3d::transform>("RadiantTranform")
               .def(tostring(const_self))
            ,
            lua::RegisterType<math3d::ipoint3>("RadiantIPoint3")
               .def(constructor<>())
               .def(constructor<const math3d::point3 &>())
               .def(constructor<const math3d::ipoint3 &>())
               .def(constructor<int, int, int>())
               .def(tostring(const_self))
               .def_readwrite("x", &math3d::ipoint3::x)
               .def_readwrite("y", &math3d::ipoint3::y)
               .def_readwrite("z", &math3d::ipoint3::z)
               .def("distance_squared", &math3d::ipoint3::distanceSquared)
               .def("distance_to", &math3d::ipoint3::distanceTo)
               .def("is_adjacent_to", &math3d::ipoint3::is_adjacent_to)
               .def(self + other<const math3d::ipoint3&>())
               .def(self - other<const math3d::ipoint3&>())
            ,
            lua::RegisterType<math3d::aabb>("RadiantAABB")
               .def(constructor<const math3d::point3&, const math3d::point3&>())
               .def(tostring(const_self))
               .def_readwrite("min", &math3d::aabb::_min)
               .def_readwrite("max", &math3d::aabb::_max)
               .def("center", &math3d::aabb::GetCentroid)
            ,
            lua::RegisterType<math3d::ibounds3>("RadiantBounds3")
               .def(constructor<const math3d::ipoint3&, const math3d::ipoint3&>())
               .def(tostring(const_self))
               .def_readwrite("min", &math3d::ibounds3::_min)
               .def_readwrite("max", &math3d::ibounds3::_max)
               .def("center", &math3d::ibounds3::centroid)
               .def("contains", &math3d::ibounds3::contains)
               .def(const_self + other<const math3d::ipoint3&>())
         ]
      ]
   ];
}
