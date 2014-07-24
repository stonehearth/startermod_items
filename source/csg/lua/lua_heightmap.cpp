#include "pch.h"
#include "lib/lua/register.h"
#include "lua_heightmap.h"
#include "csg/array_2d.h"
#include "csg/heightmap.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
static luabind::scope Register(struct lua_State* L, const char* name)
{
   return
      lua::RegisterTypePtr_NoTypeInfo<T>(name)
         .def(constructor<int, int>())
         .def("get",    &T::get)
         .def("set",    &T::set)
         .def("size",   &T::get_size);
      ;
}

scope LuaHeightmap::RegisterLuaTypes(lua_State* L)
{
   return
      Register<HeightMap<double>>(L, "HeightMap"),
      lua::RegisterTypePtr_NoTypeInfo<Array2D<int>>("Array2D")
         .def(constructor<int, int>())
         .def(constructor<int, int, int>())
         .def(constructor<int, int, csg::Point2>())
         .def(constructor<int, int, csg::Point2, int>())
         .def("get",        &Array2D<int>::get)
         .def("get",        &Array2D<int>::fetch)
         .def("set",        &Array2D<int>::set)
         .def("get_width",  &Array2D<int>::get_width)
         .def("get_height", &Array2D<int>::get_height)
      ;

}
