#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_DATA_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_DATA_H

#include <bitset>
#include <unordered_map>
#include "namespace.h"
#include "csg/namespace.h"
#include "dm/dm.h"
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
   NavGridTileData(NavGridTile& ngt);
   ~NavGridTileData();

   template <TrackerType Type> bool IsMarked(csg::Point3 const& offest);
   void MarkDirty(TrackerType t);
   float GetMovementSpeedBonus(csg::Point3 const& offset);

private:
   typedef std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE> BitSet;

   template <TrackerType Type> bool IsMarked(int bit_index);
   template <TrackerType Type> BitSet& GetBitVector();
   template <TrackerType Type> void UpdateTileData();
   template <int TrackerMask, TrackerType BitVectorType> void UpdateTileDataForTrackers();

   void UpdateMovementSpeedBonus();
   inline int Offset(int x, int y, int z);
   void UpdateCollisionTracker(CollisionTracker const& tracker, csg::Cube3 const& world_bounds);
   void UpdateCanStand(NavGrid& ng, csg::Cube3 const& world_bounds);

private:
   enum DirtyBits {
      BASE_VECTORS =    (1 << 0),
      ALL_DIRTY_BITS =  (-1)
   };

private:
   NavGridTile&   _ngt;
   int            dirty_;
   BitSet         marked_[NUM_BIT_VECTOR_TRACKERS];
   float          _movementSpeedBonus[TILE_SIZE*TILE_SIZE*TILE_SIZE];
};


END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_DATA_H
