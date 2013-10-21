#include "pch.h"
#include "lua/register.h"
#include "lua_quaternion.h"
#include "csg/quaternion.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

csg::Point3f Quaternion_Rotate(Quaternion const& qt, csg::Point3f pt)
{
   return qt.rotate(pt);
}


scope LuaQuaternion::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterType<Quaternion>("Quaternion")
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const csg::Point3f&, float>())
         .def_readwrite("x", &csg::Quaternion::x)
         .def_readwrite("y", &csg::Quaternion::y)
         .def_readwrite("z", &csg::Quaternion::z)
         .def_readwrite("w", &csg::Quaternion::w)
         .def("rotate",      &Quaternion_Rotate)
      ;
}
