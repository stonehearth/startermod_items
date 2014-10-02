#ifndef _RADIANT_CSG_REGION_TOOLS_H
#define _RADIANT_CSG_REGION_TOOLS_H

#include <vector>
#include "region.h"
#include "edge_tools.h"
#include "region_tools_traits.h"

BEGIN_RADIANT_CSG_NAMESPACE

template <typename S, int C> class RegionTools;

template <typename S>
class RegionTools<S, 1>
{
public:
   void ForEachEdge(Region<S, 1> const& region, typename RegionToolsTraits<S, 1>::ForEachEdgeCb cb) {
      for (Cube<S, 1> const &cube : region) {
         EdgeInfo<S, 1> edge_info;
         edge_info.normal = Point<S, 1>::zero;
         edge_info.min = cube.min;
         edge_info.max = cube.max;
         cb(edge_info);
      }
   }
};

template <typename S, int C>
class RegionTools
{
public:
   typedef RegionToolsTraits<S, C> Traits;
   typedef typename Traits::PlaneInfo PlaneInfo;

public:
   enum Flags {
      INCLUDE_HIDDEN_FACES    = (1 << 0)
   };

   // The ordering here is very, very specific.  The "front" plane must equal
   // the "back" plane << 1, and they must be ordered in dimensional order.  So
   // 1D objects only have top/bottom, 2D objects have top/bottom and left/right,
   // etc.  Failing to do so will break the logic which allows the client to
   // skip planes.
   enum Plane {
      BOTTOM_PLANE = 0,
      TOP_PLANE    = 1,
      LEFT_PLANE   = 2,
      RIGHT_PLANE  = 3,
      FRONT_PLANE  = 4,
      BACK_PLANE   = 5,
      NUM_PLANES   = 6,
   };

   static Plane GetNeighbor(Plane direction)
   {
      static const Plane neighbors[] = {
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

   RegionTools() :
      flags_(0),
      iter_planes_(-1) { }

   void IgnorePlane(int plane) {
      iter_planes_ &= ~(1 << plane);
   }

   void IncludePlane(int plane) {
      iter_planes_ |= (1 << plane);
   }

   // Iterate through every "face" of the region.  The faces are the extenal edges
   // of the region, reduced by 1 dimension.  For example, a cube has 6 faces,
   // a square has 4 sides, and a line has 2 points.

   void ForEachPlane(Region<S, C> const& region,
                     typename Traits::ForEachPlaneCb cb)
   {
      int current_front_plane = BOTTOM_PLANE;
      int current_back_plane = TOP_PLANE;

      for (PlaneInfo plane : Traits::planes) {
         // process 2 sets of planes at a time.  If neither bit is set, we can skip
         // these entirely!
         if ((iter_planes_ & (1 << (current_front_plane | current_back_plane))) != 0) {
            Traits::PlaneMap front, back;

            for (Cube<S, C> const& cube : region) {
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

   // Return every edge of the region.
   void ForEachEdge(Region<S, C> const& region, typename RegionToolsTraits<S, C>::ForEachEdgeCb cb)
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

   EdgeMap<S, C> GetEdgeMap(Region<S, C> const& region) {
      EdgeMap<S, C> edgemap;
      ForEachEdge(region, [&](EdgeInfo<S, C> const& info) {
         edgemap.AddEdge(info.min, info.max, info.normal);
      });
      edgemap.FixNormals();
      return std::move(edgemap);
   }

   Region<double, C> GetInnerBorder(Region<S, C> const& region, double d) {
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

private:
   void ForEachPlane(typename Traits::PlaneMap const& front, 
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

private:
   int         flags_;
   int         iter_planes_;
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_REGION_TOOLS_H
