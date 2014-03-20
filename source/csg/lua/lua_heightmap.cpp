#include "pch.h"
#include "lib/lua/register.h"
#include "lua_heightmap.h"
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
      Register<HeightMap<double>>(L, "HeightMap");
}
