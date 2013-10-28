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
      INCLUDE_HIDDEN_FACES    = (1 << 1)
   };

   RegionTools() :
      flags_(0) { }

   // Iterate through every "face" of the region.  The faces are the extenal edges
   // of the region, reduced by 1 dimension.  For example, a cube has 6 faces,
   // a square has 4 sides, and a line has 2 points.

   void ForEachPlane(Region<S, C> const& region,
                     typename Traits::ForEachPlaneCb cb)
   {
      for (PlaneInfo plane : Traits::planes) {
         Traits::PlaneMap front, back;

         for (Cube<S, C> const& cube : region) {
            Cube<S, C-1> rect = Traits::ReduceCube(cube, plane);

            front[cube.min[plane.reduced_coord]].AddUnique(rect);
            back[cube.max[plane.reduced_coord]].AddUnique(rect);
         }
         plane.normal_dir = -1; ForEachPlane(front, back, plane, cb);
         plane.normal_dir =  1; ForEachPlane(back, front, plane, cb);
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
         for (Cube<S, C-1> const& cube : r1) {
            RegionTools<S, C-1> tools;
            tools.ForEachEdge(r1, [&](EdgeInfo<S, C-1> const& ei) {
               // xxx: needs to be some "reduce" function in edge_info, right?
               edge_info.min = RegionToolsTraits<S, C>::ExpandPoint(ei.min, pi);
               edge_info.max = RegionToolsTraits<S, C>::ExpandPoint(ei.max, pi);
               cb(edge_info);
            });
         }
      });
   }

   // Return a region that is shrunk by the specified amount
   Region<float, C> Inset(Region<S, C> const& region, float d) {
      EdgeMap<S, C> edgemap;
      ForEachEdge(region, [&](EdgeInfo<S, C> const& info) {
         edgemap.AddEdge(info.min, info.max, info.normal);
      });

      // Iterate though every rect in the region and move the points over
      // by the accumulated normals
      Region<float, C> result;
      for (Cube<S, C> const& cube : region) {
         Cube<float, C> next;
         bool first = true;
         RegionToolsTraits<S, C>::ForEachCorner(cube, [&](Point<S, C> pt) {
            Point<float, C> corner = ToFloat(pt);

            EdgePoint<S, C>* edge_pt;
            if (edgemap.FindEdgePoint(pt, &edge_pt)) {
               corner -= ToFloat(edge_pt->accumulated_normals) * d;  
            }

            if (first) {
               next = Cube<float, C>(corner, corner, cube.GetTag());
               first = false;
            } else {
               next.Grow(corner);
            }  
         });
         result.AddUnique(next);
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
};

typedef RegionTools<int, 2> RegionTools2;
typedef RegionTools<int, 3> RegionTools3;

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_REGION_TOOLS_H
