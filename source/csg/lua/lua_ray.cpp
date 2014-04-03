#include "pch.h"
#include "lib/lua/register.h"
#include "lua_ray.h"
#include "csg/ray.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;


scope LuaRay::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterType<Ray3>("Ray3")
         .def(constructor<>())
         .def(constructor<const csg::Point3f&, const csg::Point3f&>())
         .def_readwrite("origin", &csg::Ray3::origin)
         .def_readwrite("direction", &csg::Ray3::direction)
      ;
}
