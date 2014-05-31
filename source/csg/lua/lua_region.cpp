#include "pch.h"
#include "lib/lua/register.h"
#include "lib/lua/script_host.h"
#include "lua_region.h"
#include "csg/edge_tools.h"
#include "csg/region_tools.h"
#include "csg/region.h"
#include "csg/util.h"
#include "csg/rotate_shape.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

template <typename T>
void CopyRegion(T& region, T const& other)
{
   region = other;
}

template <typename T>
void LoadRegion(lua_State* L, T& region, object obj)
{
   // converts the lua object to a json object, then the json
   // object to a region.  Slow?  Yes, but effective in 1 line of code.
   region = json::Node(lua::ScriptHost::LuaToJson(L, obj)).as<T>();
}

template <typename T>
std::shared_ptr<T> Duplicate(T const& region)
{
   return std::make_shared<T>(region);
}

template <typename S, int C>
EdgeMap<S, C> RegionGetEdgeList(const Region<S, C>& region)
{
   return std::move(RegionTools<S, C>().GetEdgeMap(region));
}

template <typename T>
std::shared_ptr<T> RegionClip(const T& region, typename T::Cube const& cube)
{
   std::shared_ptr<T> result = std::make_shared<T>(region);
   (*result) &= cube;
   return result;
}

template <typename T>
T IntersectRegion(T const& lhs, T const& rhs)
{
   return lhs & rhs;
}

Region2 ProjectOntoXZPlane(Region3 const& region)
{
   Region2 r2;
   for (Cube3 cube : region) {
      Rect2 rect(Point2(cube.min.x, cube.min.z), Point2(cube.max.x, cube.max.z), cube.GetTag());
      if (rect.GetArea() > 0) {
         r2.Add(rect);
      }
   }
   return r2;
}

template <typename T>
static luabind::class_<T> Register(struct lua_State* L, const char* name)
{
   return
      lua::RegisterType<T>(name)
         .def(constructor<>())
         .def(constructor<typename T::Cube const&>())
         .def(const_self - other<T const&>())
         .def(const_self - other<T::Cube const&>())
         .def("load",               &LoadRegion<T>)
         .def("copy_region",        &CopyRegion<T>)
         .def("duplicate",          &Duplicate<T>)
         .def("empty",              &T::IsEmpty)
         .def("get_area",           &T::GetArea)
         .def("clear",              &T::Clear)
         .def("get_bounds",         &T::GetBounds)
         .def("optimize_by_oct_tree", &T::OptimizeByOctTree)
         .def("optimize_by_merge",  &T::OptimizeByMerge)
         .def("intersects",         &T::Intersects)
         .def("add_region",         (void (T::*)(T const&))&T::Add)
         .def("add_cube",           (void (T::*)(typename T::Cube const&))&T::Add)
         .def("add_point",          (void (T::*)(typename T::Point const&))&T::Add)
         .def("add_unique_cube",    (void (T::*)(typename T::Cube const&))&T::AddUnique)
         .def("add_unique_point",   (void (T::*)(typename T::Point const&))&T::AddUnique)
         .def("add_unique_region",  (void (T::*)(typename T const&))&T::AddUnique)
         .def("subtract_region",    (void (T::*)(T const&))&T::Subtract)
         .def("subtract_cube",      (void (T::*)(typename T::Cube const&))&T::Subtract)
         .def("subtract_point",     (void (T::*)(typename T::Point const&))&T::Subtract)
         .def("each_cube",          &T::GetContents, return_stl_iterator)
         .def("clipped",            &RegionClip<T>)
         .def("get_num_rects",      &T::GetRectCount)
         .def("get_rect",           &T::GetRect)
         .def("get_closest_point",  &T::GetClosestPoint)
         .def("translate",          &T::Translate)
         .def("translated",         &T::Translated)
         .def("intersected",        &IntersectRegion<T>)
         .def("inflated",           &T::Inflated)
         .def("contains",           &T::Contains)
      ;
}

scope LuaRegion::RegisterLuaTypes(lua_State* L)
{
   return
      def("intersect_region2", IntersectRegion<Region2>),
      def("intersect_region3", IntersectRegion<Region3>),
      Register<Region3>(L,  "Region3")
         .def("get_adjacent",             &GetAdjacent)
         .def("project_onto_xz_plane",    &ProjectOntoXZPlane)
         .def("get_edge_list",            &RegionGetEdgeList<int, 3>)
         .def("rotated",                  (Region3 (*)(Region3 const&, int))&csg::Rotated)
      ,
      Register<Region3f>(L, "Region3f")
         .def("get_edge_list",                &RegionGetEdgeList<float, 3>),
      Register<Region2>(L,  "Region2")
         .def("get_edge_list",                &RegionGetEdgeList<int, 2>),
      Register<Region1>(L,  "Region1");
}
