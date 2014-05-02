#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_H

#include <memory>
#include <bitset>
#include "namespace.h"
#include "nav_grid_tile_data.h"
#include "protocols/forward_defines.h"
#include "csg/cube.h"
#include "core/guard.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/*
 * -- NavGridTile 
 *
 * Hold the metadata for the navigation grid.  This is predominately a list of
 * collision trackers which overlap this tile.  The actual navgrid information is
 * store in a NavGridTileData pointer referenced by the NavGridTile.  This
 * lets us keep most of the navgrid paged out, but rebuild it very quickly 
 * when it's required.  in a 1024x1024x128 world there may be up to 32768 
 * NavGridTile tiles in memory, but only a handful actually need to be resident.
 * Extra care is taken to ensure that this class is as small as possible (e.g.
 * the bounds and NavGrid reference as passed into methods which need that
 * information rather than stored in this class).
 */

class NavGridTile {
public:
   NavGridTile();

   typedef std::function<void(CollisionTrackerPtr)> ForEachTrackerCb;

   void RemoveCollisionTracker(CollisionTrackerPtr tracker);
   void AddCollisionTracker(CollisionTrackerPtr tracker);

   void OnTrackerChanged(CollisionTrackerPtr tracker);
   void OnTrackerRemoved(dm::ObjectId entityId);

   bool IsEmpty(csg::Cube3 const& bounds);
   bool CanStandOn(csg::Point3 const& pt);
   void FlushDirty(NavGrid& ng, csg::Point3 const& index);
   void MarkDirty();

   bool IsDataResident() const;
   void SetDataResident(bool value);

   void ForEachTracker(ForEachTrackerCb cb);

   enum ChangeNotifications {
      ENTITY_REMOVED = (1 << 0),
      ENTITY_ADDED = (1 << 1),
      ENTITY_MOVED = (1 << 2),
   };
   typedef std::function<void(int, dm::ObjectId, om::EntityPtr)> ChangeCb;
   
   core::Guard TraceEntityPositions(int flags, csg::Cube3 const& worldBounds, ChangeCb cb);

public:
   void UpdateBaseVectors(csg::Point3 const& index);
   void ShowDebugShapes(protocol::shapelist* msg, csg::Point3 const& index);

private:
   friend NavGridTileData;
   std::shared_ptr<NavGridTileData> GetTileData();

private:
   bool IsMarked(TrackerType type, csg::Point3 const& offest);
   bool IsMarked(TrackerType type, int bit_index);
   int Offset(csg::Point3 const& pt);
   void UpdateCollisionTracker(TrackerType type, CollisionTracker const& tracker);
   void UpdateDerivedVectors(csg::Cube3 const& world_bounds);
   void UpdateCanStand();
   csg::Cube3 GetWorldBounds(csg::Point3 const& index) const;

private:
   void OnTrackerAdded(CollisionTrackerPtr tracker);
   void PruneExpiredChangeTrackers();

   enum DirtyBits {
      BASE_VECTORS =    (1 << 0),
      DERIVED_VECTORS = (1 << 1),
      ALL_DIRTY_BITS =  (-1)
   };

   struct ChangeTracker {
      int         id;
      int         flags;
      csg::Cube3  bounds;
      ChangeCb    cb;
      bool        expired;

      ChangeTracker(int id, int f, csg::Cube3 const& b, ChangeCb c) :
         id(id),
         flags(f),
         bounds(b),
         cb(c),
         expired(false)
      {
      }
   };

private:
   TrackerMap                                trackers_;
   std::shared_ptr<NavGridTileData>          data_;
   int                                       next_change_tracker_id_;
   std::vector<ChangeTracker>                change_trackers_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
