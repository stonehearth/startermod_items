#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_H

#include <bitset>
#include "namespace.h"
#include "csg/cube.h"
#include "protocols/forward_defines.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class NavGridTile {
public:
   enum {
      TILE_SIZE = 16
   };

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
   void ProcessCollisionTracker(TrackerType type, CollisionTracker const& tracker);
   void UpdateCanStand();

private:
   NavGrid&                                     ng_;
   bool                                         dirty_;
   csg::Cube3                                   bounds_;
   std::vector<CollisionTrackerRef>             trackers_[MAX_TRACKER_TYPES];
   std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE>   marked_[MAX_TRACKER_TYPES];
   std::bitset<TILE_SIZE*TILE_SIZE*TILE_SIZE>   can_stand_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
