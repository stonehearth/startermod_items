#include "pch.h"
#include "lua/register.h"
#include "lua_region.h"
#include "csg/region.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
static std::shared_ptr<T> CopyRegion(std::shared_ptr<T> other)
{
   return std::make_shared<T>(*other);
}

template <typename T>
std::shared_ptr<T> RegionClip(const T& region, typename T::Cube const& cube)
{
   std::shared_ptr<T> result = std::make_shared<T>(region);
   (*result) &= cube;
   return result;
}

template <typename T>
static void AddCube(T& region, typename T::Cube const& cube)
{
   region.Add(cube);
}


template <typename T>
static void AddPoint(T& region, typename T::Point const& point)
{
   region.Add(point);
}

template <typename T>
static void RemoveCube(T& region, typename T::Cube const& cube)
{
   region -= cube;
}


template <typename T>
static void RemovePoint(T& region, typename T::Point const& point)
{
   region -= T::Cube(point);
}

template <typename T>
static void AddUniqueCube(T& region, typename T::Cube const& cube)
{
   region.AddUnique(cube);
}

template <typename T>
static luabind::scope Register(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<typename T::Cube const&>())
         .def("copy",               &CopyRegion<T>)
         .def("empty",              &T::IsEmpty)
         .def("get_area",           &T::GetArea)
         .def("clear",              &T::Clear)
         .def("get_bounds",         &T::GetBounds)
         .def("optimize",           &T::Optimize)
         .def("intersects",         &T::Intersects)
         .def("add_cube",           &AddCube<T>)
         .def("add_point",          &AddPoint<T>)
         .def("add_unique",         &AddUniqueCube<T>)
         .def("remove_cube",        &RemoveCube<T>)
         .def("remove_point",       &RemovePoint<T>)
         .def("contents",           &T::GetContents, return_stl_iterator)
         .def("clip",               &RegionClip<T>)
         .def("get_num_rects",      &T::GetRectCount)
         .def("get_rect",           &T::GetRect)
         .def("get_closest_point",  &T::GetClosestPoint)
         .def("translate",          &T::Translate)
         .def("contains",           &T::Contains)
      ;
}

scope LuaRegion::RegisterLuaTypes(lua_State* L)
{
   return
      Register<Region3>(L,  "Region3"),
      Register<Region3f>(L, "Region3f"),
      Register<Region2>(L,  "Region2"),
      Register<Region1>(L,  "Region1");
}
