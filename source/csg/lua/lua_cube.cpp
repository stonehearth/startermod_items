#include "pch.h"
#include "lib/lua/register.h"
#include "lua_cube.h"
#include "csg/cube.h"
#include "lib/json/namespace.h"
#include "csg/util.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T> 
class PointIterator
{
public:
   PointIterator(T const& cube) :
      cube_(cube),
      i_(cube_.begin())
   {
   }

   static int NextIteration(lua_State *L)
   {
      PointIterator* iter = object_cast<PointIterator*>(object(from_stack(L, -2)));
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
   NO_COPY_CONSTRUCTOR(PointIterator)

private:
   const T& cube_;
   typename T::PointIterator i_;
};

template <typename T>
static void EachPoint(lua_State *L, T const& cube)
{
   lua_pushcfunction(L, PointIterator<T>::NextIteration);  // f
   object(L, new PointIterator<T>(cube)).push(L);          // s
   object(L, 1).push(L);                                   // var (ignored)
}

Cube3f ToCube3f(const Cube3& r) {
   return ToFloat(r);
}


IMPLEMENT_TRIVIAL_TOSTRING(PointIterator<Cube3>);
DEFINE_INVALID_JSON_CONVERSION(PointIterator<Cube3>);

template <typename T>
T IntersectCube(T const& lhs, T const& rhs)
{
   return lhs & rhs;
}

Rect2 ProjectOntoXZPlane(Cube3 const& cube)
{
   Rect2 projection(
      Point2(cube.min.x, cube.min.z),
      Point2(cube.max.x, cube.max.z),
      cube.GetTag()
   );
   return projection;
}

template <typename T>
static luabind::class_<T> Register(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const T&>())
         .def(constructor<const typename T::Point&, const typename T::Point&>())
         .def(constructor<const typename T::Point&, const typename T::Point&, int>())
         .property("min",     &T::GetMin, &T::SetMin)
         .property("max",     &T::GetMax, &T::SetMax)
         .property("tag",     &T::GetTag, &T::SetTag)
         .def("get_area",     &T::GetArea)
         .def("contains",     &T::Contains)
         .def("width",        &T::GetWidth) 
         .def("height",       &T::GetHeight)
         .def("translated",   &T::Translated)
         .def("scaled",       &T::Scaled)
      ;
}

template <typename T>
static luabind::scope RegisterWithIterator(struct lua_State* L, const char* name)
{
   typedef PointIterator<T> Iterator;
   lua::RegisterTypePtr<Iterator>()
}

scope LuaCube::RegisterLuaTypes(lua_State* L)
{
   return
      def("construct_cube3", &Cube3::Construct),
      def("intersect_cube3", IntersectCube<Cube3>),
      def("intersect_cube2", IntersectCube<Rect2>),
      Register<Cube3>(L,  "Cube3")
         .def("to_float",     &ToCube3f)
         .def("each_point",   &EachPoint<Cube3>)
         .def("project_onto_xz_plane", &ProjectOntoXZPlane),
      Register<Cube3f>(L, "Cube3f"),
      Register<Rect2>(L,  "Rect2"),
      Register<Line1>(L,  "Line1"),
      lua::RegisterType<PointIterator<Cube3>>()
   ;
}
