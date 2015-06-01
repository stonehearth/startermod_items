#include "pch.h"
#include "lib/lua/register.h"
#include "lua_quaternion.h"
#include "csg/quaternion.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

Point3f Quaternion_Rotate(Quaternion const& qt, Point3f pt)
{
   return qt.rotate(pt);
}

Quaternion Quaternion_Slerp(Quaternion const& qt1, Quaternion const& qt2, float t)
{
   Quaternion q;
   slerp(q, qt1, qt2, t);
   return q;
}

void Quaternion_GetAxisAngle(lua_State *L, Quaternion const& qt)
{
   Point3f axis;
   double angle;

   GetAxisAngleNormalized(qt, axis, angle);

   luabind::object lua_axis = luabind::object(L, axis);
   lua_axis.push(L);

   double degrees = ToDegrees(angle);
   luabind::object lua_angle = luabind::object(L, degrees);
   lua_angle.push(L);
}

scope LuaQuaternion::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterType<Quaternion>("Quaternion")
         .def(constructor<>())
         .def(constructor<const Point3f&, float>())
         .def(constructor<const Point3f&, const Point3f&>())
         .def(constructor<double, double, double, double>())
         .def(const_self * other<Quaternion const&>())
         .def(const_self + other<Quaternion const&>())
         .def_readwrite("x",    &Quaternion::x)
         .def_readwrite("y",    &Quaternion::y)
         .def_readwrite("z",    &Quaternion::z)
         .def_readwrite("w",    &Quaternion::w)
         .def("rotate",         &Quaternion_Rotate)
         .def("slerp",          &Quaternion_Slerp)
         .def("normalize",      &Quaternion::Normalize)
         .def("look_at",        &Quaternion::lookAt)
         .def("get_axis_angle", &Quaternion_GetAxisAngle)
      ;
}
