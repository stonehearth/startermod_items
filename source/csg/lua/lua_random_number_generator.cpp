#include "pch.h"
#include "lib/lua/register.h"
#include "lua_random_number_generator.h"
#include "csg/random_number_generator.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

scope LuaRandomNumberGenerator::RegisterLuaTypes(lua_State* L)
{
   return
   lua::RegisterType<RandomNumberGenerator>("RandomNumberGenerator")
      .def(constructor<>())
      .def(constructor<unsigned int>())
      .def("set_seed", &RandomNumberGenerator::SetSeed)
      .def("get_int", &RandomNumberGenerator::GetInt<int>)
      .def("get_real", &RandomNumberGenerator::GetReal<double>)
      .def("get_gaussian", &RandomNumberGenerator::GetGaussian<double>)
      ,
      def("get_default_rng", &csg::RandomNumberGenerator::DefaultInstance)
      ;      
}
