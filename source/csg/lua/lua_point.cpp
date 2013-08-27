#include "pch.h"
#include "lua/register.h"
#include "lua_point.h"
#include "csg/point.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;


template <typename T>
std::string PointToJson(T const& pt, luabind::object state)
{
   std::stringstream os;
   os << "{ \"x\" :" << pt.x << ", \"y\" :" << pt.y << ", \"z\" :" << pt.z << "}";
   return os.str();
}

bool Point3_IsAdjacentTo(Point3 const& a, Point3 const& b)
{
   int sum = 0;
   for (int i = 0; i < 3; i++) {
      sum += abs(a[i] - b[i]);
   }
   return sum == 1;
}

scope LuaPoint::RegisterLuaTypes(lua_State* L)
{

   return
      lua::RegisterType<Point3>("Point3")
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const csg::Point3&>())
         .def(constructor<int, int, int>())
         .def("__tojson",    &PointToJson<csg::Point3>)
         .def_readwrite("x", &csg::Point3::x)
         .def_readwrite("y", &csg::Point3::y)
         .def_readwrite("z", &csg::Point3::z)
         .def(self + other<csg::Point3 const&>())
         .def(self - other<csg::Point3 const&>())
         .def("distance_squared",   &csg::Point3::LengthSquared)
         .def("distance_to",        &csg::Point3::DistanceTo)
         .def("is_adjacent_to",     &Point3_IsAdjacentTo)
      ,
      lua::RegisterType<Point2>("Point2")
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<int, int>())
         .def_readwrite("x", &csg::Point2::x)
         .def_readwrite("y", &csg::Point2::y)
      ;
}
