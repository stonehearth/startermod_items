#include "pch.h"
#include "lib/lua/register.h"
#include "lua_cube.h"
#include "lua_iterator.h"
#include "csg/cube.h"
#include "csg/iterators.h"
#include "lib/json/namespace.h"
#include "csg/util.h"
#include "csg/rotate_shape.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

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
// For lua's "to_int" and "to_double", we always want to return a copy to avoid confusion.

template <typename S, int C>
csg::Cube<double, C> Cube_ToInt(csg::Cube<S, C> const& c)
{
   return csg::ToFloat(csg::ToInt(c));
}

template <typename S, int C>
csg::Cube<double, C> Cube_ToFloat(csg::Cube<S, C> const& c)
{
   return csg::ToFloat(c);
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
         .def("grow",         (void (T::*)(T::Point const&))&T::Grow)
         .def("grow",         (void (T::*)(T const&))&T::Grow)
         .def("translate",    &T::Translate)
         .def("translated",   &T::Translated)
         .def("inflated",     &T::Inflated)
         .def("intersects",   &T::Intersects)
         .def("get_border",   &T::GetBorder)
         .def("distance_to",  (double (T::*)(T const&) const)&T::DistanceTo)
         .def("distance_to",  (double (T::*)(typename T::Point const&) const)&T::DistanceTo)
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
         .def("each_point",   &EachPointCube3f)
         .def("rotated",      &(Cube3f (*)(Cube3f const&, int))&csg::Rotated)
         .def("project_onto_xz_plane", &ProjectOntoXZPlane),
      Register<Cube3>(L, "Cube3i")
         .def("to_float",     &Cube_ToFloat<int, 3>),
      Register<Rect2f>(L,  "Rect2")
         .def("each_point",   &EachPointRect2f),
      Register<Line1f>(L,  "Line1")
   ;
}
