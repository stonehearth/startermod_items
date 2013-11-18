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
      .def("generate_uniform_int", &RandomNumberGenerator::GenerateUniformInt<int>)
      .def("generate_uniform_real", &RandomNumberGenerator::GenerateUniformReal<double>)
      .def("generate_gaussian", &RandomNumberGenerator::GenerateGaussian<double>)
   ;
}
