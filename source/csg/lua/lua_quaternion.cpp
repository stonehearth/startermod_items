#include "pch.h"
#include "lib/lua/register.h"
#include "lua_quaternion.h"
#include "csg/quaternion.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

csg::Point3f Quaternion_Rotate(Quaternion const& qt, csg::Point3f pt)
{
   return qt.rotate(pt);
}

Quaternion Quaternion_Slerp(Quaternion const& qt1, Quaternion const& qt2, float t)
{
   Quaternion q;
   slerp(q, qt1, qt2, t);
   return q;
}


scope LuaQuaternion::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterType<Quaternion>("Quaternion")
         .def(constructor<>())
         .def(constructor<const csg::Point3f&, float>())
         .def(constructor<const csg::Point3f&, const csg::Point3f&>())
         .def(const_self * other<Quaternion const&>())
         .def(const_self + other<Quaternion const&>())
         .def_readwrite("x", &csg::Quaternion::x)
         .def_readwrite("y", &csg::Quaternion::y)
         .def_readwrite("z", &csg::Quaternion::z)
         .def_readwrite("w", &csg::Quaternion::w)
         .def("rotate",      &Quaternion_Rotate)
         .def("slerp",       &Quaternion_Slerp)
         .def("normalize",   &csg::Quaternion::Normalize)
         .def("look_at",     &csg::Quaternion::lookAt)
      ;
}
