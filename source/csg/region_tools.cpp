#include "pch.h"
#include "color.h"
#include "region_tools.h"

using namespace ::radiant;
using namespace ::radiant::csg;

std::vector<PlaneInfo<int, 2>> RegionToolsTraitsBase<int, 2, RegionToolsTraits<int, 2>>::planes;
std::vector<PlaneInfo<int, 3>> RegionToolsTraitsBase<int, 3, RegionToolsTraits<int, 3>>::planes;
std::vector<PlaneInfo<double, 2>> RegionToolsTraitsBase<double, 2, RegionToolsTraits<double, 2>>::planes;
std::vector<PlaneInfo<double, 3>> RegionToolsTraitsBase<double, 3, RegionToolsTraits<double, 3>>::planes;

// works around the fact that visual studio does not yet support
// C++11 container type initializers
struct RegionToolsInitializer
{
   RegionToolsInitializer() {
      Initialize2();
      Initialize3();
   }

   void Initialize2() {
      // Do not reorder these!  See the definition of enum Planes in region_tools.h for
      // why.
      struct { int reduced_coord, x; } planes[] = {
         { 1, 0 },         // TOP_PLANE, BOTTOM_PLANE
         { 0, 1 }          // LEFT_PLANE, RIGHT_PLANE
      };
      for (const auto& item : planes) {
         RegionToolsTraits<int, 2>::PlaneInfo plane;
         plane.x = item.x;
         plane.reduced_coord = item.reduced_coord;
         RegionToolsTraits2::planes.push_back(plane);
         RegionToolsTraits2f::planes.push_back(ToFloat(plane));
      }
   }

   void Initialize3() {
      // Do not reorder these!  See the definition of enum Planes in region_tools.h for
      // why.
      struct { int reduced_coord, x, y; } planes[] = {
         { 1, 0, 2 },   // TOP_PLANE, BOTTOM_PLANE
         { 0, 2, 1 },   // LEFT_PLANE, RIGHT_PLANE
         { 2, 0, 1 },   // FRONT_PLANE, BACK_PLANE
      };
      for (const auto& item : planes) {
         RegionToolsTraits<int, 3>::PlaneInfo plane;
         plane.x = item.x;
         plane.y = item.y;
         plane.reduced_coord = item.reduced_coord;
         RegionToolsTraits3::planes.push_back(plane);
         RegionToolsTraits3f::planes.push_back(ToFloat(plane));
      }
   }
};

RegionToolsInitializer ri;

// Start of RegionTools implementation

template <typename S, int C>
typename RegionTools<S, C>::Plane RegionTools<S, C>::GetNeighbor(typename RegionTools<S, C>::Plane direction)
{
   static const typename RegionTools<S, C>::Plane neighbors[] = {
      TOP_PLANE,
      BOTTOM_PLANE,
      RIGHT_PLANE,
      LEFT_PLANE,
      BACK_PLANE,
      FRONT_PLANE
   };
   ASSERT(direction >= 0 && direction < NUM_PLANES);
   return neighbors[direction];
}

template <typename S, int C>
RegionTools<S, C>::RegionTools() :
   flags_(0),
   iter_planes_(-1)
{
}

template <typename S, int C>
void RegionTools<S, C>::IgnorePlane(int plane) {
   iter_planes_ &= ~(1 << plane);
}

template <typename S, int C>
void RegionTools<S, C>::IncludePlane(int plane) {
   iter_planes_ |= (1 << plane);
}

// Iterate through every "face" of the region.  The faces are the extenal edges
// of the region, reduced by 1 dimension.  For example, a cube has 6 faces,
// a square has 4 sides, and a line has 2 points.
template <typename S, int C>
void RegionTools<S, C>::ForEachPlane(Region<S, C> const& region, typename Traits::ForEachPlaneCb cb)
{
   int current_front_plane = BOTTOM_PLANE;
   int current_back_plane = TOP_PLANE;

   for (PlaneInfo plane : Traits::planes) {
      // process 2 sets of planes at a time.  If neither bit is set, we can skip
      // these entirely!
      if ((iter_planes_ & (1 << (current_front_plane | current_back_plane))) != 0) {
         Traits::PlaneMap front, back;

         for (Cube<S, C> const& cube : EachCube(region)) {
            Cube<S, C-1> rect = Traits::ReduceCube(cube, plane);

            front[cube.min[plane.reduced_coord]].AddUnique(rect);
            back[cube.max[plane.reduced_coord]].AddUnique(rect);
         }

         if ((iter_planes_ & (1 << current_front_plane)) != 0) {
            plane.which = current_front_plane;
            plane.normal_dir = -1;
            ForEachPlane(front, back, plane, cb);
         }

         if ((iter_planes_ & (1 << current_back_plane)) != 0) {
            plane.which = current_back_plane;
            plane.normal_dir =  1;
            ForEachPlane(back, front, plane, cb);
         }
      }
      current_front_plane += 2;
      current_back_plane += 2;
   }
}

// Return every edge of each plane in the region. Edges that are shared between mulitple planes
// will be returned multiple times with the normal of the plane the edge is part of.
template <typename S, int C>
void RegionTools<S, C>::ForEachEdge(Region<S, C> const& region, typename RegionToolsTraits<S, C>::ForEachEdgeCb cb)
{
   RegionTools<S, C> tools;
   tools.ForEachPlane(region, [&](Region<S, C-1> const& r1, PlaneInfo const& pi) {
      EdgeInfo<S, C> edge_info;
      edge_info.normal = Point<S, C>::zero;
      edge_info.normal[pi.reduced_coord] = pi.normal_dir;

      RegionTools<S, C-1> tools;
      tools.ForEachEdge(r1, [&](EdgeInfo<S, C-1> const& ei) {
         edge_info.min = RegionToolsTraits<S, C>::ExpandPoint(ei.min, pi);
         edge_info.max = RegionToolsTraits<S, C>::ExpandPoint(ei.max, pi);
         cb(edge_info);
      });
   });
}

// template aliases not supported until VS2013
// template <typename S, int C>
// using LineSegment = std::pair<Point<S, C>, Point<S, C>>;
#define LINE_SEGMENT std::pair<Point<S, C>, Point<S, C>>

template <typename S, int C>
class LineSegmentHash
{
public:
   size_t operator()(LINE_SEGMENT const& segment) const {
      // use boost::hash_combine if you want something better
      return 51 * Point<S, C>::Hash()(segment.first) + Point<S, C>::Hash()(segment.second);
   }
};

template <typename S, int C>
void RegionTools<S, C>::ForEachUniqueEdge(Region<S, C> const& region, typename RegionToolsTraits<S, C>::ForEachEdgeCb cb)
{
   std::unordered_map<LINE_SEGMENT, EdgeInfo<S, C>, LineSegmentHash<S, C>> edges;

   ForEachEdge(region, [&edges](EdgeInfo<S, C> const& edge_info) {
      LINE_SEGMENT edge(edge_info.min, edge_info.max);
      EdgeInfo<S, C> new_edge_info;
      auto iterator = edges.find(edge);

      if (iterator != edges.end()) {
         new_edge_info = iterator->second;
         // normal is the accumulated normal
         new_edge_info.normal += edge_info.normal;
      } else {
         new_edge_info = edge_info;
      }

      edges[edge] = new_edge_info;
   });

   for (auto const& entry : edges) {
      cb(entry.second);
   }
}

template <typename S, int C>
EdgeMap<S, C> RegionTools<S, C>::GetEdgeMap(Region<S, C> const& region) {
   EdgeMap<S, C> edgemap;
   ForEachEdge(region, [&](EdgeInfo<S, C> const& info) {
      edgemap.AddEdge(info.min, info.max, info.normal);
   });
   edgemap.FixNormals();
   return std::move(edgemap);
}

template <typename S, int C>
Region<double, C> RegionTools<S, C>::GetInnerBorder(Region<S, C> const& region, double d) {
   EdgeMap<S, C> edgemap;
   ForEachEdge(region, [&](EdgeInfo<S, C> const& info) {
      edgemap.AddEdge(info.min, info.max, info.normal);
   });
   edgemap.FixNormals();

   Region<double, C> result;
   for (Edge<S, C> const& edge : edgemap) {
      Point<double, C> min = ToFloat(edge.min->location);
      Point<double, C> max = ToFloat(edge.max->location);

      for (int i = 0; i < C; i++) {
         S n = edge.normal[i];

         // move in the opposite direction of the normal
         if (n < 0) {
            max[i] += static_cast<double>(-n) * d;
         } else if (n > 0) {
            min[i] += static_cast<double>(-n) * d;
         } else {
            // if the edges agree in the translation in the tangent if it makes
            // the resultant cube bigger.
            if (edge.min->accumulated_normals[i] > 0) {
               min[i] += static_cast<double>(-edge.min->accumulated_normals[i]) * d;
            }
            if (edge.max ->accumulated_normals[i] < 0) {
               max[i] += static_cast<double>(-edge.max->accumulated_normals[i]) * d;
            }
         }
      }
      result.Add(Cube<double, C>(min, max));
   }
   return result;
}

template <typename S, int C>
void RegionTools<S, C>::ForEachPlane(typename Traits::PlaneMap const& front, 
                  typename Traits::PlaneMap const& back,
                  typename PlaneInfo info,
                  typename Traits::ForEachPlaneCb cb)
{
   for (const auto& entry : front) {
      info.reduced_value = entry.first;
      csg::Region<S, C-1> const& region = entry.second;

      if ((flags_ & INCLUDE_HIDDEN_FACES) != 0) {
         cb(region, info);
      } else {
         auto i = back.find(info.reduced_value);
         if (i != back.end()) {
            cb(region - i->second, info);
         } else {
            cb(region, info);
         }
      }
   }   
}

template <typename S>
void RegionTools<S, 1>::ForEachEdge(Region<S, 1> const& region, typename RegionToolsTraits<S, 1>::ForEachEdgeCb cb) {
   for (Cube<S, 1> const &cube : EachCube(region)) {
      EdgeInfo<S, 1> edge_info;
      edge_info.normal = Point<S, 1>::zero;
      edge_info.min = cube.min;
      edge_info.max = cube.max;
      cb(edge_info);
   }
}

template <typename S, int C>
Region<S, C-1> RegionTools<S, C>::GetCrossSection(Region<S, C> region, int dimension, S value)
{
   Region<S, C-1> cross_section;
   int d = dimension;
   std::vector<int> surviving_dimensions; // faster if we make this a standard C array?

   for (int i = 0; i < C; i++) {
      if (i != d) {
         surviving_dimensions.push_back(i);
      }
   }

   for (Cube<S, C> const& cube : EachCube(region)) {
      if (IsBetween(cube.min[d], value, cube.max[d])) {
         Point<S, C-1> min, max;
         for (int i = 0; i < C-1; i++) {
            min[i] = cube.min[surviving_dimensions[i]];
            max[i] = cube.max[surviving_dimensions[i]];
         }

         Cube<S, C-1> projected_cube(min, max);
         cross_section.AddUnique(projected_cube);
      }
   }

   return cross_section;
}

// Create RegionTools implementations with these parameters
template RegionTools<int, 2>;
template RegionTools<int, 3>;
template RegionTools<double, 2>;
template RegionTools<double, 3>;
