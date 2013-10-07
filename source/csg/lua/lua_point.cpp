#include "pch.h"
#include "lua/register.h"
#include "lua_point.h"
#include "csg/point.h"
#include "csg/color.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
std::string PointToJson(T const& pt, luabind::object state)
{
   char label = 'x';
   std::stringstream os;
   os << "{ ";
   for (int i = 0; i < T::Dimension; i++) {
      os << "\"" << static_cast<char>(label + i) << "\" :" << pt[i];
      if (i < (T::Dimension - 1)) {
         os << ", ";
      }
   }
   os << "}";
   return os.str();
}

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
   return csg::IsZero(sum - 1);
}

template <typename T>
static luabind::class_<T> RegisterCommon(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<T const&>())
         .def("__tojson",    &PointToJson<T>)
         .def(const_self + other<T const&>())
         .def(const_self - other<T const&>())
         .def(const_self == other<T const&>())
         .def(const_self < other<T const&>())
         .def("distance_squared",   &T::LengthSquared)
         .def("distance_to",        &T::DistanceTo)
         .def("is_adjacent_to",     &Point_IsAdjacentTo<T>)
         .def("scale",              &T::Scale)
         .def("normalize",          &T::Normalize)
         .def("dot",                &T::Dot);

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
      Register1<Point1 >(L, "Point1"),
      Register1<Point1f>(L, "Point1f"),
      Register2<Point2 >(L, "Point2"),
      Register2<Point2f>(L, "Point2f"),
      Register3<Point3 >(L, "Point3"),
      Register3<Point3f>(L, "Point3f"),
      lua::RegisterType<Color4>("Color4")
         .def(constructor<int, int, int, int>())
         .def_readwrite("r", &Color4::r)
         .def_readwrite("g", &Color4::g)
         .def_readwrite("b", &Color4::b)
         .def_readwrite("a", &Color4::a),
      def("lerp",             &csg::Interpolate);
}
