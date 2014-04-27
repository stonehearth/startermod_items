#ifndef _RADIANT_PHYSICS_NAV_GRID_TILE_H
#define _RADIANT_PHYSICS_NAV_GRID_TILE_H

#include <memory>
#include <bitset>
#include "namespace.h"
#include "nav_grid_tile_data.h"
#include "protocols/forward_defines.h"

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

   bool IsEmpty(csg::Cube3 const& bounds);
   bool CanStandOn(csg::Point3 const& pt);
   void FlushDirty(NavGrid& ng, csg::Point3 const& index);
   void MarkDirty();

   bool IsDataResident() const;
   void SetDataResident(bool value);

   void ForEachTracker(ForEachTrackerCb cb);

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
   enum DirtyBits {
      BASE_VECTORS =    (1 << 0),
      DERIVED_VECTORS = (1 << 1),
      ALL_DIRTY_BITS =  (-1)
   };

private:
   bool                                visited_;
   TrackerMap                          trackers_;
   std::shared_ptr<NavGridTileData>    data_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
