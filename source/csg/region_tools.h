#ifndef _RADIANT_CSG_REGION_TOOLS_H
#define _RADIANT_CSG_REGION_TOOLS_H

#include <vector>
#include "region.h"
#include "edge_tools.h"
#include "region_tools_traits.h"

BEGIN_RADIANT_CSG_NAMESPACE

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

   static Plane GetNeighbor(Plane direction);

   RegionTools();

   void IgnorePlane(int plane);
   void IncludePlane(int plane);

   void ForEachPlane(Region<S, C> const& region, typename Traits::ForEachPlaneCb cb);
   void ForEachEdge(Region<S, C> const& region, typename RegionToolsTraits<S, C>::ForEachEdgeCb cb);
   void ForEachUniqueEdge(Region<S, C> const& region, typename RegionToolsTraits<S, C>::ForEachEdgeCb cb);

   EdgeMap<S, C> GetEdgeMap(Region<S, C> const& region);
   Region<double, C> GetInnerBorder(Region<S, C> const& region, double d);

private:
   void ForEachPlane(typename Traits::PlaneMap const& front, typename Traits::PlaneMap const& back,
                     typename PlaneInfo info, typename Traits::ForEachPlaneCb cb);

private:
   int         flags_;
   int         iter_planes_;
};

template <typename S>
class RegionTools<S, 1>
{
public:
   void ForEachEdge(Region<S, 1> const& region, typename RegionToolsTraits<S, 1>::ForEachEdgeCb cb);
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_REGION_TOOLS_H
