#include "pch.h"
#include "lib/lua/register.h"
#include "lua_cube.h"
#include "csg/cube.h"
#include "lib/json/namespace.h"
#include "csg/util.h"
#include "csg/rotate_shape.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T> 
class LuaPointIterator
{
public:
   LuaPointIterator(T const& cube) :
      cube_(cube),
      i_(cube_.begin())
   {
   }

   static int NextIteration(lua_State *L)
   {
      LuaPointIterator* iter = object_cast<LuaPointIterator*>(object(from_stack(L, -2)));
      return iter->Next(L);
   }

   int Next(lua_State *L) {
      if (i_ != cube_.end()) {
         luabind::object(L, *i_).push(L);
         ++i_;
         return 1;
      }
      return 0;
   }

private:
   NO_COPY_CONSTRUCTOR(LuaPointIterator)

private:
   const T& cube_;
   typename T::PointIterator i_;
};

template <typename T>
static void EachPoint(lua_State *L, T const& cube)
{
   lua_pushcfunction(L, LuaPointIterator<T>::NextIteration);  // f
   object(L, new LuaPointIterator<T>(cube)).push(L);          // s
   object(L, 1).push(L);                                   // var (ignored)
}

IMPLEMENT_TRIVIAL_TOSTRING(LuaPointIterator<Cube3f>);
IMPLEMENT_TRIVIAL_TOSTRING(LuaPointIterator<Rect2f>);
DEFINE_INVALID_LUA_CONVERSION(LuaPointIterator<Cube3f>);
DEFINE_INVALID_LUA_CONVERSION(LuaPointIterator<Rect2f>);
DEFINE_INVALID_JSON_CONVERSION(LuaPointIterator<Cube3f>);
DEFINE_INVALID_JSON_CONVERSION(LuaPointIterator<Rect2f>);

template <typename T>
T IntersectCube(T const& lhs, T const& rhs)
{
   return lhs & rhs;
}

Rect2f ProjectOntoXZPlane(Cube3f const& cube)
{
   Rect2f projection(
      Point2f(cube.min.x, cube.min.z),
      Point2f(cube.max.x, cube.max.z),
      cube.GetTag()
   );
   return projection;
}

template <typename T>
void LoadCube(lua_State* L, T& region, object obj)
{
   // converts the lua object to a json object, then the json
   // object to a region.  Slow?  Yes, but effective in 1 line of code.
   region = json::Node(lua::ScriptHost::LuaToJson(L, obj)).as<T>();
}


// The csg version of these functions are optimized for use in templated code.  They
// avoid copies for nop conversions by returning a const& to the input parameter.
// For lua's "to_int" and "to_float", we always want to return a copy to avoid confusion.

template <typename S, int C>
csg::Cube<float, C> Cube_ToInt(csg::Cube<S, C> const& c)
{
   return csg::ToFloat(csg::ToInt(c));
}

template <typename T>
static luabind::class_<T> Register(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(constructor<>())
         .def(constructor<const T&>())
         .def(constructor<const typename T::Point&, const typename T::Point&>())
         .def(constructor<const typename T::Point&, const typename T::Point&, int>())
         .def_readwrite("min", &T::min)
         .def_readwrite("max", &T::max)
         .property("tag",     &T::GetTag, &T::SetTag)
         .def("to_int",       &Cube_ToInt<T::ScalarType, T::Dimension>)
         .def("load",         &LoadCube<T>)
         .def("get_area",     &T::GetArea)
         .def("get_size",     &T::GetSize)
         .def("contains",     &T::Contains)
         .def("width",        &T::GetWidth) 
         .def("height",       &T::GetHeight)
         .def("translate",    &T::Translate)
         .def("translated",   &T::Translated)
         .def("inflated",     &T::Inflated)
         .def("intersects",   &T::Intersects)
         .def("get_border",   &T::GetBorder)
         .def("distance_to",  (float (T::*)(T const&) const)&T::DistanceTo)
         .def("distance_to",  (float (T::*)(typename T::Point const&) const)&T::DistanceTo)
         .def("scaled",       &T::Scaled)
      ;
}

scope LuaCube::RegisterLuaTypes(lua_State* L)
{
   return
      def("construct_cube3", &Cube3f::Construct),
      def("intersect_cube3", IntersectCube<Cube3f>),
      def("intersect_cube2", IntersectCube<Rect2f>),
      Register<Cube3f>(L, "Cube3")
         .def("rotated",      &(Cube3f (*)(Cube3f const&, int))&csg::Rotated)
         .def("each_point",   &EachPoint<Cube3f>)
         .def("project_onto_xz_plane", &ProjectOntoXZPlane),
      Register<Rect2f>(L,  "Rect2")
         .def("each_point",   &EachPoint<Rect2f>),
      Register<Line1f>(L,  "Line1"),
      lua::RegisterType_NoTypeInfo<LuaPointIterator<Cube3f>>("Cube3Iterator"),
      lua::RegisterType_NoTypeInfo<LuaPointIterator<Rect2f>>("Rect2Iterator")
   ;
}
