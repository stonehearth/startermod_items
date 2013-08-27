#include "pch.h"
#include "register.h"
#include "radiant_luabind.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::lua;

template <typename T>
std::string PointToJson(T const& pt, luabind::object state)
{
   std::stringstream os;
   os << "{ \"x\" :" << pt.x << ", \"y\" :" << pt.y << ", \"z\" :" << pt.z << "}";
   return os.str();
}

void ::radiant::lua::RegisterBasicTypes(lua_State* L)
{
#if 0
   module(L) [
      namespace_("_radiant") [
            lua::RegisterType<csg::Point3f>("RadiantPoint3")
               .def(constructor<const csg::Point3 &>())
               .def(constructor<float, float, float>())
               .def("__tojson",    &PointToJson<csg::Point3f>)
               .def(tostring(const_self))
               .def_readwrite("x", &csg::Point3f::x)
               .def_readwrite("y", &csg::Point3f::y)
               .def_readwrite("z", &csg::Point3f::z)
               .def("distance_to", &csg::Point3f::distance)
               .def("Normalize",   &csg::Point3f::Normalize)
               .def(self + other<const csg::Point3f&>())
               .def(self - other<const csg::Point3f&>())
               .def(self * other<float>())
            ,
            lua::RegisterType<csg::Transform>("RadiantTranform")
               .def(tostring(const_self))
            ,
            lua::RegisterType<csg::Point3>("Point3")
               .def(constructor<>())
               .def(constructor<const csg::Point3f &>())
               .def(constructor<const csg::Point3 &>())
               .def(constructor<int, int, int>())
               .def("__tojson",    &PointToJson<csg::Point3>)
               .def(tostring(const_self))
               .def_readwrite("x", &csg::Point3::x)
               .def_readwrite("y", &csg::Point3::y)
               .def_readwrite("z", &csg::Point3::z)
               .def("distance_squared", &csg::Point3::distanceSquared)
               .def("distance_to", &csg::Point3::distanceTo)
               .def("is_adjacent_to", &csg::Point3::is_adjacent_to)
               .def(self + other<const csg::Point3&>())
               .def(self - other<const csg::Point3&>())
            ,
            lua::RegisterType<csg::Cube3f>("RadiantAABB")
               .def(constructor<const csg::Point3f&, const csg::Point3f&>())
               .def(tostring(const_self))
               .def_readwrite("min", &csg::Cube3f::_min)
               .def_readwrite("max", &csg::Cube3f::_max)
               .def("center", &csg::Cube3f::GetCentroid)
            ,
            lua::RegisterType<csg::Cube3>("RadiantBounds3")
               .def(constructor<const csg::Point3&, const csg::Point3&>())
               .def(tostring(const_self))
               .def_readwrite("min", &csg::Cube3::_min)
               .def_readwrite("max", &csg::Cube3::_max)
               .def("center", &csg::Cube3::centroid)
               .def("contains", &csg::Cube3::contains)
               .def(const_self + other<const csg::Point3&>())
      ]
   ];
#endif
}
