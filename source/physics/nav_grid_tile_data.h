#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_DATA_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_DATA_H

#include <bitset>
#include "namespace.h"
#include "csg/namespace.h"
#include "protocols/forward_defines.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/*
 * -- NavGridTileData
 *
 * Maintains the navigation bitmap for a region of the world (see TILE_SIZE).
 * This is a lazy data-structure.  Most of the heavy lifting only happens on
 * the first query since the last time it was modified.
 */

class NavGridTileData {
public:
   NavGridTileData();
   ~NavGridTileData();

   void FlushDirty(NavGrid& ng, std::vector<CollisionTrackerRef>& trackers, csg::Cube3 const& world_bounds);
   void UpdateBaseVectors(std::vector<CollisionTrackerRef>& trackers, csg::Cube3 const& world_bounds);
   bool IsEmpty(csg::Cube3 const& bounds);
   bool CanStandOn(csg::Point3 const& pt);
   void MarkDirty();

public:
   void ShowDebugShapes(protocol::shapelist* msg, csg::Cube3 const& world_bounds);

private:
   bool IsMarked(TrackerType type, csg::Point3 const& offest);
   bool IsMarked(TrackerType type, int bit_index);
   int Offset(csg::Point3 const& pt);
   void UpdateCollisionTracker(CollisionTracker const& tracker, csg::Cube3 const& world_bounds);
   void UpdateDerivedVectors(NavGrid& ng, csg::Cube3 const& world_bounds);
   void UpdateCanStand(NavGrid& ng, csg::Cube3 const& world_bounds);

private:
   enum DirtyBits {
      BASE_VECTORS =    (1 << 0),
      DERIVED_VECTORS = (1 << 1),
      ALL_DIRTY_BITS =  (-1)
   };

private:
   typedef std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE> BitSet;
private:
   int      dirty_;
   BitSet   marked_[MAX_TRACKER_TYPES];
   BitSet   can_stand_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_DATA_H
