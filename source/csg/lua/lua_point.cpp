#include "pch.h"
#include "lib/lua/register.h"
#include "lua_point.h"
#include "csg/point.h"
#include "csg/color.h"
#include "csg/transform.h"
#include "csg/util.h" // xxx: should be in csg/csg.h

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
bool Point_IsAdjacentTo(T const& a, T const& b)
{
   typename T::Scalar sum = 0;
   for (int i = 0; i < T::Dimension; i++) {
      if (a[i] < b[i]) {
         sum += (b[i] - a[i]);
      } else {
         sum += (a[i] - b[i]);
      }
   }
   return csg::IsZero(static_cast<float>(sum - 1));
}

// The csg version of these functions are optimized for use in templated code.  They
// avoid copies for nop conversions by returning a const& to the input parameter.
// For lua's "to_int" and "to_float", we always want to return a copy to avoid confusion.

template <int C>
csg::Point<int, C> Point_ToInt(csg::Point<int, C> const& p)
{
   return p;
}

template <int C>
csg::Point<float, C> Pointf_ToFloat(csg::Point<float, C> const& p)
{
   return p;
}

template <int C>
csg::Point<float, C> Point_ToFloat(csg::Point<int, C> const& p)
{
   return csg::ToFloat(p);
}

template <int C>
csg::Point<int, C> Pointf_ToInt(csg::Point<float, C> const& p)
{
   return csg::ToInt(p);
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
         .def("length",             &T::Length)
         .def("distance_squared",   &T::LengthSquared)
         .def("distance_to",        &T::DistanceTo)
         .def("is_adjacent_to",     &Point_IsAdjacentTo<T>)
         .def("scale",              &T::Scale)
         .def("normalize",          &T::Normalize)
         .def("dot",                &T::Dot)
         .def("scaled",             &T::Scaled)
         ;

}

template <typename T>
static luabind::class_<T> Register1(struct lua_State* L, const char* name)
{
   return RegisterCommon<T>(L, name)
         .def(constructor<T::Scalar>())
         .def_readwrite("x", &T::x);
}

template <typename T>
static luabind::class_<T> Register2(struct lua_State* L, const char* name)
{
   return RegisterCommon<T>(L, name)
         .def(constructor<T::Scalar, T::Scalar>())
         .def_readwrite("x", &T::x)
         .def_readwrite("y", &T::y);
}

template <typename T>
static luabind::class_<T> Register3(struct lua_State* L, const char* name)
{
   return RegisterCommon<T>(L, name)
         .def(constructor<T::Scalar, T::Scalar, T::Scalar>())
         .def_readwrite("x", &T::x)
         .def_readwrite("y", &T::y)
         .def_readwrite("z", &T::z);
}

scope LuaPoint::RegisterLuaTypes(lua_State* L)
{

   return
      Register1<Point1 >(L, "Point1")
         .def("to_int",             &Point_ToInt<1>)
         .def("to_float",           &Point_ToFloat<1>),
      Register1<Point1f>(L, "Point1f")
         .def("to_int",             &Pointf_ToInt<1>)
         .def("to_float",           &Pointf_ToFloat<1>),
      Register2<Point2 >(L, "Point2")
         .def("to_int",             &Point_ToInt<2>)
         .def("to_float",           &Point_ToFloat<2>),
      Register2<Point2f>(L, "Point2f")
         .def("to_int",             &Pointf_ToInt<2>)
         .def("to_float",           &Pointf_ToFloat<2>),
      Register3<Point3 >(L, "Point3")
         .def("to_int",             &Point_ToInt<3>)
         .def("to_float",           &Point_ToFloat<3>),
      Register3<Point3f>(L, "Point3f")
         .def("to_int",             &Pointf_ToInt<3>)
         .def("to_float",           &Pointf_ToFloat<3>)
         .def("lerp",   (Point3f (*)(Point3f const& a, Point3f const& b, float alpha))&csg::Interpolate),
      lua::RegisterType<Transform>("Transform")
         .def("lerp",   (Transform (*)(Transform const& a, Transform const& b, float alpha))&csg::Interpolate),
      lua::RegisterType<Color3>("Color3")
         .def(constructor<int, int, int>())
         .def_readwrite("r", &Color3::r)
         .def_readwrite("g", &Color3::g)
         .def_readwrite("b", &Color3::b),
      lua::RegisterType<Color4>("Color4")
         .def(constructor<int, int, int, int>())
         .def_readwrite("r", &Color4::r)
         .def_readwrite("g", &Color4::g)
         .def_readwrite("b", &Color4::b)
         .def_readwrite("a", &Color4::a)
      ;
}
