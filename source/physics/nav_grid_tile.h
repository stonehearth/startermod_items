#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_H

#include <memory>
#include <bitset>
#include "namespace.h"
#include "nav_grid_tile_data.h"
#include "protocols/forward_defines.h"
#include "csg/cube.h"
#include "core/slot.h"

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

class NavGridTile;
DECLARE_SHARED_POINTER_TYPES(NavGridTile)

class NavGridTile : public std::enable_shared_from_this<NavGridTile> {
public:
   NavGridTile(NavGrid& ng, csg::Point3 const& index);
   NavGridTile(NavGridTile &&other);

   typedef std::function<bool(CollisionTrackerPtr)> ForEachTrackerCb;

   void RemoveCollisionTracker(CollisionTrackerPtr tracker);
   void AddCollisionTracker(CollisionTrackerPtr tracker);

   void OnTrackerRemoved(dm::ObjectId entityId, TrackerType t);

   bool IsBlocked(csg::Point3 const& pt);
   bool IsBlocked(csg::Cube3 const& bounds);

   bool IsSupport(csg::Point3 const& pt);
   bool IsSupport(csg::Cube3 const& bounds);
   bool IsSupport(csg::Region3 const& region);
   void FlushDirty(NavGrid& ng, csg::Point3 const& index);

   bool IsDataResident() const;
   void SetDataResident(bool value);

   bool ForEachTracker(ForEachTrackerCb cb);
   bool ForEachTrackerForEntity(dm::ObjectId entityId, ForEachTrackerCb cb);

   enum ChangeNotifications {
      ENTITY_REMOVED = (1 << 0),
      ENTITY_ADDED   = (1 << 1),
      ENTITY_MOVED   = (1 << 2),
   };
   struct ChangeNotification {
      dm::ObjectId         entityId;
      ChangeNotifications  reason;
      CollisionTrackerPtr  tracker;
   
      ChangeNotification(ChangeNotifications r, dm::ObjectId i, CollisionTrackerPtr t) :
         entityId(i),
         reason(r),
         tracker(t) { }
   };

   typedef std::function<void(ChangeNotification const&)> ChangeCb;

   core::Guard RegisterChangeCb(ChangeCb cb);

public:
   void ShowDebugShapes(protocol::shapelist* msg, csg::Point3 const& index);

private:
   NO_COPY_CONSTRUCTOR(NavGridTile);
   friend NavGridTileData;
   std::shared_ptr<NavGridTileData> GetTileData();

private:
   void MarkDirty(TrackerType type);
   bool IsMarked(TrackerType type, csg::Point3 const& offest);
   bool IsMarked(TrackerType type, int bit_index);
   int Offset(csg::Point3 const& pt);
   void UpdateCollisionTracker(TrackerType type, CollisionTracker const& tracker);
   csg::Cube3 GetWorldBounds(csg::Point3 const& index) const;

private:
   void OnTrackerAdded(CollisionTrackerPtr tracker);
   void PruneExpiredChangeTrackers();
   bool ForEachTrackerInRange(TrackerMap::const_iterator begin, TrackerMap::const_iterator end, ForEachTrackerCb cb);

   enum DirtyBits {
      BASE_VECTORS =    (1 << 0),
      DERIVED_VECTORS = (1 << 1),
      ALL_DIRTY_BITS =  (-1)
   };

private:
   NavGrid&                                  _ng;
   TrackerMap                                trackers_;
   csg::Point3                               _index;
   std::shared_ptr<NavGridTileData>          data_;
   core::Slot<ChangeNotification>            changed_slot_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
