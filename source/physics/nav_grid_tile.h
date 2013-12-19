#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_H

#include <bitset>
#include "namespace.h"
#include "csg/cube.h"
#include "protocols/forward_defines.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/*
 * -- NavGridTile 
 *
 * Maintains the navigation bitmap for a region of the world (see TILE_SIZE).
 * This is a lazy data-structure.  Most of the heavy lifting only happens on
 * the first query since the last time it was modified.
 */

class NavGridTile {
public:
   enum {
      TILE_SIZE = 16
   };

   /*
    * We keep one 16x16x16 array of bits for every TrackerType, plus a number
    * of derived arrays (e.g. can_stand_).
    * 
    * COLLISION - bits that are on represent voxels which overlap some entities
    *             collision shape.  Mobile entities cannot overlap these voxels.
    *
    * LADDER - represents bits which overlap some entities VerticalPathingRegion
    *          region.  These bits are both traversible (i.e. you can walk through
    *          them) and can support an entity (i.e. you can stand on them).
    *          I guess technically that makes them more like a chute, but \/\/
    *          Ladder bits override collision bits!
    */
   enum TrackerType {
      COLLISION = 0,
      LADDER = 1,
      MAX_TRACKER_TYPES,
   };

   NavGridTile(NavGrid &ng, csg::Point3 const& index);
   void RemoveCollisionTracker(TrackerType type, CollisionTrackerPtr tracker);
   void AddCollisionTracker(TrackerType type, CollisionTrackerPtr tracker);
   bool IsEmpty(csg::Cube3 const& bounds);
   bool CanStandOn(csg::Point3 const& pt);

public:
   void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);

private:
   void FlushDirty();
   bool IsMarked(TrackerType type, csg::Point3 const& offest);
   bool IsMarked(TrackerType type, int bit_index);
   int Offset(csg::Point3 const& pt);
   void UpdateCollisionTracker(TrackerType type, CollisionTracker const& tracker);
   void UpdateBaseVectors();
   void UpdateDerivedVectors();
   void UpdateCanStand();

private:
   enum DirtyBits {
      BASE_VECTORS =    (1 << 0),
      DERIVED_VECTORS = (1 << 1)
   };

private:
   NavGrid&                                     ng_;
   int                                          dirty_;
   csg::Point3                                  index_;
   csg::Cube3                                   bounds_;
   std::vector<CollisionTrackerRef>             trackers_[MAX_TRACKER_TYPES];
   std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE>   marked_[MAX_TRACKER_TYPES];
   std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE>   can_stand_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
