#include "pch.h"
#include "lua/register.h"
#include "lua_region.h"
#include "csg/region.h"
#include "csg/util.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
void CopyRegion(T& region, T const& other)
{
   region = other;
}

template <typename T>
std::shared_ptr<T> Duplicate(T const& region)
{
   return std::make_shared<T>(region);
}

template <typename T>
std::shared_ptr<T> RegionClip(const T& region, typename T::Cube const& cube)
{
   std::shared_ptr<T> result = std::make_shared<T>(region);
   (*result) &= cube;
   return result;
}

template <typename T>
T Region_Intersection(T const& lhs, T const& rhs)
{
   return lhs & rhs;
}

template <typename T>
static luabind::class_<T> Register(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<typename T::Cube const&>())
         .def(const_self - other<T const&>())
         .def(const_self - other<T::Cube const&>())
         .def("copy_region",        &CopyRegion<T>)
         .def("duplicate",          &Duplicate<T>)
         .def("empty",              &T::IsEmpty)
         .def("get_area",           &T::GetArea)
         .def("clear",              &T::Clear)
         .def("get_bounds",         &T::GetBounds)
         .def("optimize",           &T::Optimize)
         .def("intersects",         &T::Intersects)
         .def("add_region",         (void (T::*)(T const&))&T::Add)
         .def("add_cube",           (void (T::*)(typename T::Cube const&))&T::Add)
         .def("add_point",          (void (T::*)(typename T::Point const&))&T::Add)
         .def("add_unique_cube",    (void (T::*)(typename T::Cube const&))&T::AddUnique)
         .def("add_unique_region",  (void (T::*)(typename T const&))&T::AddUnique)
         .def("subtract_region",    (void (T::*)(T const&))&T::Subtract)
         .def("subtract_cube",      (void (T::*)(typename T::Cube const&))&T::Subtract)
         .def("subtract_point",     (void (T::*)(typename T::Point const&))&T::Subtract)
         .def("intersect_region",   (void (T::*)(T const&))&T::Subtract)
         .def("contents",           &T::GetContents, return_stl_iterator)
         .def("clip",               &RegionClip<T>)
         .def("get_num_rects",      &T::GetRectCount)
         .def("get_rect",           &T::GetRect)
         .def("get_closest_point",  &T::GetClosestPoint)
         .def("translate",          &T::Translate)
         .def("translated",         &T::Translated)
         .def("contains",           &T::Contains)
      ;
}

scope LuaRegion::RegisterLuaTypes(lua_State* L)
{
   return
      def("region3_intersection", Region_Intersection<Region3>),
      Register<Region3>(L,  "Region3")
         .def("get_adjacent",       &GetAdjacent),
      Register<Region3f>(L, "Region3f"),
      Register<Region2>(L,  "Region2"),
      Register<Region1>(L,  "Region1");
}
