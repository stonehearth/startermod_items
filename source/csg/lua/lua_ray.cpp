#include "pch.h"
#include "lua/register.h"
#include "lua_ray.h"
#include "csg/ray.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;


std::string RayToJson(csg::Ray3 const& ray, luabind::object state)
{
   std::stringstream os;
   os << "{ \"ox\" :" << ray.origin.x << ", \"oy\" :" << ray.origin.y << ", \"oz\" :" << ray.origin.z << ", ";
   os << "\"dx\" :" << ray.direction.x << ", \"dy\" :" << ray.direction.y << ", \"dz\" :" << ray.direction.z << "}";
   return os.str();
}

scope LuaRay::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterType<Ray3>("Ray3")
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const csg::Point3f&, const csg::Point3f&>())
         .def("__tojson",    &RayToJson)
         .def_readwrite("origin", &csg::Ray3::origin)
         .def_readwrite("direction", &csg::Ray3::direction)
      ;
}
