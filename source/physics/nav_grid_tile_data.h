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
   float GetMaxMovementSpeedBonus();
   bool CanPassThrough(om::EntityPtr const& entity, csg::Point3 const& offset);

private:
   enum {
      NUM_BITSETS = 3, // COLLISION, TERRAIN, and LADDER...
   };
   typedef std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE> BitSet;

   template <TrackerType Type> bool IsMarked(int bit_index);
   template <TrackerType Type> __forceinline BitSet& GetBitVector();
   template <TrackerType Type> __forceinline void UpdateTileData();
   template <int TrackerMask, TrackerType BitVectorType> __forceinline void UpdateTileDataForTrackers();

   void UpdateMovementSpeedBonus();
   void UpdateMovementGuardMap();
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
   BitSet         marked_[NUM_BITSETS];
   float          _movementSpeedBonus[TILE_SIZE*TILE_SIZE*TILE_SIZE];
   float          _maxMovementSpeedBonus;
   BitSet         _movementGuardBits;
   std::vector<MovementGuardShapeTrackerRef> _movementGuardTrackers;
};


END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_DATA_H
