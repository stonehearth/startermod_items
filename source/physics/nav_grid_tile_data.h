#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_DATA_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_DATA_H

#include <bitset>
#include <unordered_map>
#include "namespace.h"
#include "csg/namespace.h"
#include "dm/dm.h"
#include "protocols/forward_defines.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

typedef std::unordered_multimap<dm::ObjectId, CollisionTrackerRef> TrackerMap;

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

   void FlushDirty(NavGrid& ng, TrackerMap& trackers, csg::Cube3 const& world_bounds);
   void UpdateBaseVectors(TrackerMap& trackers, csg::Cube3 const& world_bounds);
   void MarkDirty();
   bool IsMarked(TrackerType type, csg::Point3 const& offest);

private:
   bool IsMarked(TrackerType type, int bit_index);
   int Offset(csg::Point3 const& pt);
   void UpdateCollisionTracker(CollisionTracker const& tracker, csg::Cube3 const& world_bounds);
   void UpdateCanStand(NavGrid& ng, csg::Cube3 const& world_bounds);

private:
   enum DirtyBits {
      BASE_VECTORS =    (1 << 0),
      ALL_DIRTY_BITS =  (-1)
   };

private:
   typedef std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE> BitSet;
private:
   int      dirty_;
   BitSet   marked_[NUM_BIT_VECTOR_TRACKERS];
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_DATA_H
