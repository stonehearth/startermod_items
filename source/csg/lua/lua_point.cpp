#include "pch.h"
#include "lib/lua/register.h"
#include "lua_point.h"
#include "csg/point.h"
#include "csg/color.h"
#include "csg/transform.h"
#include "csg/rotate_shape.h"
#include "csg/util.h" // xxx: should be in csg/csg.h

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
bool Point_IsAdjacentTo(T const& a, T const& b)
{
   typename T::ScalarType sum = 0;
   for (int i = 0; i < T::Dimension; i++) {
      if (a[i] < b[i]) {
         sum += (b[i] - a[i]);
      } else {
         sum += (a[i] - b[i]);
      }
   }
   return csg::IsZero(static_cast<double>(sum - 1));
}

/*
 * -- Point_KeyValue
 *
 * Useful for converting a point to a stable value which can be used as the key to a
 * lua function.  There are serious issues using luabind generated objects as keys
 * to tables.  See this thread for more info:
 *
 *    http://lua.2524044.n2.nabble.com/luabind-problem-retrieving-values-from-table-indexed-by-non-built-in-classes-td7582227.html 
 *
 * Until those are solved, just convert the Point to a string to use as a key.  This
 * requires manual calls to :key_value() by the client, but it's better than them having
 * to roll their own.
 *
 */
template <typename T>
std::string Point_KeyValue(T const& a)
{
   return BUILD_STRING(a);
}

// The csg version of these functions are optimized for use in templated code.  They
// avoid copies for nop conversions by returning a const& to the input parameter.
// For lua's "to_int" and "to_double", we always want to return a copy to avoid confusion.

template <int C>
csg::Point<double, C> Pointf_ToInt(csg::Point<double, C> const& p)
{
   return csg::ToFloat(csg::ToInt(p));
}

template <int C>
csg::Point<double, C> Pointi_ToFloat(csg::Point<int, C> const& p)
{
   return csg::ToFloat(p);
}

// This looks weird, right?  It's the Lua call to "convert this possibly doubleing point number
// to a grid address, which is an integer whole number".  To do this, we use ToClosestInt.  Then,
// to push it back into Lua, we have to convert it back into a double!  Crazy!!
//
template <int C>
csg::Point<double, C> Pointf_ToClosestInt(csg::Point<double, C> const& p)
{
   return csg::ToFloat(csg::ToClosestInt(p));
}

// Returns a hash of the INTEGER coordinates of the point
template <typename T>
int Point_Hash(T const& p)
{
   // The current implementation of Point<S, C>::Hash is poorly defined when S is floating point
   // and the values are large. We'll need to use a different hash function if we care about
   // hashing fractional coordinates into different buckets.
   auto cpp_hash = Point<int, T::Dimension>::Hash()(csg::ToInt(p));
   // Integer serialization to lua rolls over to negative numbers above int32 max
   int lua_hash = cpp_hash % std::numeric_limits<int32>::max();
   return lua_hash;
}

template <typename T>
static luabind::class_<T> RegisterCommon(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(constructor<>())
         .def(constructor<T const&>())
         .def(const_self + other<T const&>())
         .def(const_self - other<T const&>())
         .def(const_self == other<T const&>())
         .def(const_self < other<T const&>())
         .def(-const_self)
         .def("set",                &T::Set)
         .def("length",             &T::Length)
         .def("distance_squared",   &T::LengthSquared)
         .def("distance_to",        &T::DistanceTo)
         .def("is_adjacent_to",     &Point_IsAdjacentTo<T>)
         .def("scale",              static_cast<void (T::*)(double)>(&T::Scale))
         .def("normalize",          &T::Normalize)
         .def("translate",          &T::Translate)
         .def("dot",                &T::Dot)
         .def("scaled",             static_cast<T (T::*)(double) const>(&T::Scaled))
         .def("translated",         &T::Translated)
         .def("key_value",          &Point_KeyValue<T>)
         .def("hash",               &Point_Hash<T>)
         ;
}

template <typename T>
static luabind::class_<T> Register1(struct lua_State* L, const char* name)
{
   return RegisterCommon<T>(L, name)
         .def(constructor<T::ScalarType>())
         .def_readwrite("x", &T::x);
}

template <typename T>
static luabind::class_<T> Register2(struct lua_State* L, const char* name)
{
   return RegisterCommon<T>(L, name)
         .def(constructor<T::ScalarType, T::ScalarType>())
         .def_readwrite("x", &T::x)
         .def_readwrite("y", &T::y);
}

template <typename T>
static luabind::class_<T> Register3(struct lua_State* L, const char* name)
{
   return RegisterCommon<T>(L, name)
         .def(constructor<T::ScalarType, T::ScalarType, T::ScalarType>())
         .def_readwrite("x", &T::x)
         .def_readwrite("y", &T::y)
         .def_readwrite("z", &T::z);
}

scope LuaPoint::RegisterLuaTypes(lua_State* L)
{

   return
      Register1<Point1f>(L, "Point1")
         .def("to_int",             &Pointf_ToInt<1>)
         .def("to_closest_int",     &Pointf_ToClosestInt<1>),
      Register2<Point2f>(L, "Point2")
         .def("to_int",             &Pointf_ToInt<2>)
         .def("to_closest_int",     &Pointf_ToClosestInt<2>)
         .def(const_self * double())
         .def(const_self / double()),
      Register3<Point3f>(L, "Point3")
         .def("to_int",             &Pointf_ToInt<3>)
         .def("to_closest_int",     &Pointf_ToClosestInt<3>)
         .def("rotated",            (Point3f (*)(Point3f const&, int))&csg::Rotated)
         .def(const_self * double())
         .def(const_self / double())
         .def("lerp",   (Point3f (*)(Point3f const& a, Point3f const& b, double alpha))&csg::Interpolate),
      Register3<Point3>(L, "Point3i")
         .def("to_float",           &Pointi_ToFloat<3>),
      lua::RegisterType<Transform>("Transform")
         .def("lerp",   (Transform (*)(Transform const& a, Transform const& b, double alpha))&csg::Interpolate),
      lua::RegisterType<Color3>("Color3")
         .def(constructor<>())
         .def(constructor<int>())
         .def(constructor<const char*>())
         .def(constructor<int, int, int>())
         .def("to_integer",   &Color3::ToInteger)
         .def_readwrite("r", &Color3::r)
         .def_readwrite("g", &Color3::g)
         .def_readwrite("b", &Color3::b),
      lua::RegisterType<Color4>("Color4")
         .def(constructor<>())
         .def(constructor<int>())
         .def(constructor<const char*>())
         .def(constructor<int, int, int, int>())
         .def("to_integer",   &Color3::ToInteger)
         .def_readwrite("r", &Color4::r)
         .def_readwrite("g", &Color4::g)
         .def_readwrite("b", &Color4::b)
         .def_readwrite("a", &Color4::a)
      ;
}
