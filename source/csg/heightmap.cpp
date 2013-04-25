#include "pch.h"
#include "heightmap.h"

using namespace ::radiant;
using namespace ::radiant::csg;

template <class S>
luabind::scope HeightMap<S>::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<HeightMap<S>>(name)
         .def(constructor<int, int>())
         .def("get",    &HeightMap::get)
         .def("set",    &HeightMap::set)
         .def("size",   &HeightMap::get_size);
}

#define MAKE_REGION(Cls) \
   template luabind::scope Cls::RegisterLuaType(struct lua_State*, const char*); \

MAKE_REGION(HeightMap<double>)
