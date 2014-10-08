#include "pch.h"
#include "lib/lua/register.h"
#include "lua_iterator.h"
#include "csg/region.h"
#include "csg/iterators.h"
#include "lib/json/namespace.h"
#include "csg/util.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;


template <typename T, typename I> 
class LuaPointIterator
{
public:
   LuaPointIterator(T const& t) :
      _i(t)
   {
   }

   static int NextIteration(lua_State *L)
   {
      LuaPointIterator* iter = object_cast<LuaPointIterator*>(object(from_stack(L, -2)));
      return iter->Next(L);
   }

   int Next(lua_State *L) {
      I end;
      if (_i != end) {
         luabind::object(L, csg::ToFloat(*_i)).push(L);
         ++_i;
         return 1;
      }
      return 0;
   }

private:
   NO_COPY_CONSTRUCTOR(LuaPointIterator)

private:
   I                       _i;
};

typedef LuaPointIterator<Cube3f, Cube3fPointIterator> LuaCube3fPointIterator;
typedef LuaPointIterator<Rect2f, Rect2fPointIterator> LuaRect2fPointIterator;
typedef LuaPointIterator<Region2f, Region2fPointIterator> LuaRegion2fPointIterator;
typedef LuaPointIterator<Region3f, Region3fPointIterator> LuaRegion3fPointIterator;

void csg::EachPointRect2f(lua_State *L, Rect2f const& cube)
{
   lua_pushcfunction(L, LuaRect2fPointIterator::NextIteration);  // f
   object(L, new LuaRect2fPointIterator(cube)).push(L);          // s
   object(L, 1).push(L);                                         // var (ignored)
}

void csg::EachPointCube3f(lua_State *L, Cube3f const& cube)
{
   lua_pushcfunction(L, LuaCube3fPointIterator::NextIteration);  // f
   object(L, new LuaCube3fPointIterator(cube)).push(L);          // s
   object(L, 1).push(L);                                         // var (ignored)
}

void csg::EachPointRegion3f(lua_State *L, Region3f const& region)
{
   lua_pushcfunction(L, LuaRegion3fPointIterator::NextIteration);  // f
   object(L, new LuaRegion3fPointIterator(region)).push(L);          // s
   object(L, 1).push(L);                                         // var (ignored)
}

void csg::EachPointRegion2f(lua_State *L, Region2f const& region)
{
   lua_pushcfunction(L, LuaRegion2fPointIterator::NextIteration);  // f
   object(L, new LuaRegion2fPointIterator(region)).push(L);          // s
   object(L, 1).push(L);                                         // var (ignored)
}

IMPLEMENT_TRIVIAL_TOSTRING(LuaCube3fPointIterator);
IMPLEMENT_TRIVIAL_TOSTRING(LuaRect2fPointIterator);
IMPLEMENT_TRIVIAL_TOSTRING(LuaRegion2fPointIterator);
IMPLEMENT_TRIVIAL_TOSTRING(LuaRegion3fPointIterator);
DEFINE_INVALID_LUA_CONVERSION(LuaCube3fPointIterator);
DEFINE_INVALID_LUA_CONVERSION(LuaRect2fPointIterator);
DEFINE_INVALID_LUA_CONVERSION(LuaRegion2fPointIterator);
DEFINE_INVALID_LUA_CONVERSION(LuaRegion3fPointIterator);
DEFINE_INVALID_JSON_CONVERSION(LuaCube3fPointIterator);
DEFINE_INVALID_JSON_CONVERSION(LuaRect2fPointIterator);
DEFINE_INVALID_JSON_CONVERSION(LuaRegion2fPointIterator);
DEFINE_INVALID_JSON_CONVERSION(LuaRegion3fPointIterator);

scope csg::RegisterIteratorTypes(lua_State* L)
{
   return
      lua::RegisterType_NoTypeInfo<LuaCube3fPointIterator>("Cube3Iterator"),
      lua::RegisterType_NoTypeInfo<LuaRect2fPointIterator>("Rect2Iterator"),
      lua::RegisterType_NoTypeInfo<LuaRegion3fPointIterator>("Region3Iterator"),
      lua::RegisterType_NoTypeInfo<LuaRegion2fPointIterator>("Region2Iterator")
   ;
}
