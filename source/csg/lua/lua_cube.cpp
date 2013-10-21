#include "pch.h"
#include "lua/register.h"
#include "lua_cube.h"
#include "csg/cube.h"
#include "lib/json/namespace.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
static luabind::scope Register(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const T&>())
         .def(constructor<const typename T::Point&, const typename T::Point&>())
         .def(constructor<const typename T::Point&, const typename T::Point&, int>())
         .property("min",     &T::GetMin, &T::SetMin)
         .property("max",     &T::GetMax, &T::SetMax)
         .property("tag",     &T::GetTag, &T::SetTag)
         .def("get_area",     &T::GetArea)
         .def("contains",     &T::Contains)
         .def("width",        &T::GetWidth) 
         .def("height",       &T::GetHeight)
      ;
}

scope LuaCube::RegisterLuaTypes(lua_State* L)
{
   return
      def("ConstructCube3", &Cube3::Construct),
      Register<Cube3>(L,  "Cube3"),
      Register<Cube3f>(L, "Cube3f"),
      Register<Rect2>(L,  "Rect2"),
      Register<Line1>(L,  "Line1");
}
