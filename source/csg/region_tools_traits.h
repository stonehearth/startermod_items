#ifndef _RADIANT_CSG_REGION_TOOLS_TRAITS_H
#define _RADIANT_CSG_REGION_TOOLS_TRAITS_H

BEGIN_RADIANT_CSG_NAMESPACE

// xxx: move to util
template <int C>
Point<float, C> ToFloat(Point<int, C> const& pt) {
   Point<float, C> result;
   for (int i = 0; i < C; i++) {
      result[i] = static_cast<float>(pt[i] + k_epsilon);
   }
   return result;
}

template <int C>
Point<float, C> ToFloat(Point<float, C> const& pt) {
   return pt;
}

template <typename S, int C> struct PlaneInfo;

template <typename S>
struct PlaneInfo<S, 2> {
   int    normal_dir;
   S      reduced_value;
   int    reduced_coord;
   int    x;
};

template <typename S>
struct PlaneInfo<S, 3>
 {
   int    normal_dir;
   S      reduced_value;
   int    reduced_coord;
   int    x;
   int    y;
};

template <typename S, int C> PlaneInfo<float, C> ToFloat(PlaneInfo<S, C> const& p);

template <typename S> PlaneInfo<float, 3> ToFloat(PlaneInfo<S, 3> const& p)
{
   PlaneInfo<float, 3> result;
   result.normal_dir = p.normal_dir;
   result.reduced_value = static_cast<float>(p.reduced_value);
   result.reduced_coord = p.reduced_coord;
   result.x = p.x;
   result.y = p.y;
   return result;
}

template <typename S, int C, typename Derived>
struct RegionToolsTraitsBase
{
   typedef PlaneInfo<S, C> PlaneInfo;
   static std::vector<typename PlaneInfo> planes;

   typedef std::unordered_map<S, Region<S, C-1>>   PlaneMap;
   typedef std::function<void(typename Region<S, C-1> const&, typename PlaneInfo const&)> ForEachPlaneCb;
   typedef std::function<void(typename EdgeInfo<S, C> const&)> ForEachEdgeCb;

   static Cube<S, C-1> ReduceCube(Cube<S, C> const& cube, PlaneInfo const& p) {
      return Cube<S, C-1>(Derived::ReducePoint(cube.min, p), Derived::ReducePoint(cube.max, p), cube.GetTag());
   }

   static Cube<S, C> ExpandCube(Cube<S, C-1> const& cube, PlaneInfo const& p) {
      return Cube<S, C>(Derived::ExpandPoint(cube.min, p), Derived::ExpandPoint(cube.max, p), cube.GetTag());
   }
};

template <typename S, int C> struct RegionToolsTraits;

template <typename S>
struct RegionToolsTraits<S, 3> : public RegionToolsTraitsBase<S, 3, RegionToolsTraits<S, 3>>
{
   static Point<S, 2> ReducePoint(Point<S, 3> const& pt, PlaneInfo const& p) {
      return Point<S, 2>(pt[p.x], pt[p.y]);
   }

   static Point<S, 3> ExpandPoint(Point<S, 2> const& pt, PlaneInfo const& p) {
      Point<S, 3> result;
      result[p.reduced_coord] = p.reduced_value;
      result[p.x] = pt[0];
      result[p.y] = pt[1];
      return result;
   }

   static void ForEachCorner(Cube<S, 3> const& cube, std::function<void(Point<S, 3> const&)> cb) {
      cb(Point<S, 3>(cube.min.x, cube.min.y, cube.min.z));
      cb(Point<S, 3>(cube.min.x, cube.max.y, cube.min.z));
      cb(Point<S, 3>(cube.min.x, cube.min.y, cube.max.z));
      cb(Point<S, 3>(cube.min.x, cube.max.y, cube.max.z));
      cb(Point<S, 3>(cube.max.x, cube.min.y, cube.min.z));
      cb(Point<S, 3>(cube.max.x, cube.max.y, cube.min.z));
      cb(Point<S, 3>(cube.max.x, cube.min.y, cube.max.z));
      cb(Point<S, 3>(cube.max.x, cube.max.y, cube.max.z));
   }
};

template <typename S>
struct RegionToolsTraits<S, 2> : public RegionToolsTraitsBase<S, 2, RegionToolsTraits<S, 2>>
{
   static Point<S, 1> ReducePoint(Point<S, 2> const& pt, PlaneInfo const& p) {
      return Point<S, 1>(pt[p.x]);
   }

   static Point<S, 2> ExpandPoint(Point<S, 1> const& pt, PlaneInfo const& p) {
      Point<S, 2> result;
      result[p.reduced_coord] = p.reduced_value;
      result[p.x] = pt[0];
      return result;
   }

   static void ForEachCorner(Cube<S, 2> const& cube, std::function<void(Point<S, 2> const&)> cb) {
      cb(cube.min);
      cb(Point<S, 2>(cube.max.x, cube.min.y));
      cb(cube.max);
      cb(Point<S, 2>(cube.min.x, cube.max.y));
   }
};

template <typename S>
struct RegionToolsTraits<S, 1> : public RegionToolsTraitsBase<S, 1, RegionToolsTraits<S, 1>>
{
   // If we ever actually reduce this far, we'll throw an implementation in here.
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_REGION_TOOLS_TRAITS_H
