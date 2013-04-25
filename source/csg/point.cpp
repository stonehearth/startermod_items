#include "pch.h"
#include "point.h"

using namespace ::radiant::csg;

Point3f Point3f::zero(0, 0, 0);
Point3f Point3f::one(1, 1, 1);

Point3 Point3::zero(0, 0, 0);
Point3 Point3::one(1, 1, 1);

Point2 Point2::zero(0, 0);
Point2 Point2::one(1, 1);

Point1 Point1::zero(0);
Point1 Point1::one(1);

luabind::scope Point3::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<Point3>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const math3d::ipoint3&>())
         .def(constructor<const math3d::point3&>())
         .def(constructor<int, int, int>())
         .def_readwrite("x", &csg::Point3::x)
         .def_readwrite("y", &csg::Point3::y)
         .def_readwrite("z", &csg::Point3::z)
         .def(self + other<csg::Point3 const&>())
         .def(self - other<csg::Point3 const&>())
         .def("to_ipoint3",  &csg::Point3::operator radiant::math3d::ipoint3)
      ;
}

luabind::scope Point2::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<Point2>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<int, int>())
         .def_readwrite("x", &csg::Point2::x)
         .def_readwrite("y", &csg::Point2::y)
      ;
}

