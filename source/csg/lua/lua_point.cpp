#include "pch.h"
#include "lua/register.h"
#include "lua_point.h"
#include "csg/point.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

scope LuaPoint::RegisterLuaTypes(lua_State* L)
{

   return
      lua::RegisterType<Point3>("Point3")
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const csg::Point3&>())
         .def(constructor<int, int, int>())
         .def_readwrite("x", &csg::Point3::x)
         .def_readwrite("y", &csg::Point3::y)
         .def_readwrite("z", &csg::Point3::z)
         .def(self + other<csg::Point3 const&>())
         .def(self - other<csg::Point3 const&>())
      ,
      lua::RegisterType<Point2>("Point2")
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<int, int>())
         .def_readwrite("x", &csg::Point2::x)
         .def_readwrite("y", &csg::Point2::y)
      ;
}
