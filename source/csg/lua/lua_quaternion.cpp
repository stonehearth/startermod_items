#include "pch.h"
#include "lua/register.h"
#include "lua_quaternion.h"
#include "csg/quaternion.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;


std::string QuaternionToJson(Quaternion const& pt, luabind::object state)
{
   std::stringstream os;
   os << "{ \"x\" :" << pt.x << ", \"y\" :" << pt.y << ", \"z\" :" << pt.z << ", \"w\" :" << pt.w << "}";
   return os.str();
}

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
         .def("__tojson",    &QuaternionToJson)
         .def_readwrite("x", &csg::Quaternion::x)
         .def_readwrite("y", &csg::Quaternion::y)
         .def_readwrite("z", &csg::Quaternion::z)
         .def_readwrite("w", &csg::Quaternion::w)
         .def("rotate",      &Quaternion_Rotate)
      ;
}
