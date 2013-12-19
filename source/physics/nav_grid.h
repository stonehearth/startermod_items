#ifndef _RADIANT_PHYSICS_NAV_GRID_H
#define _RADIANT_PHYSICS_NAV_GRID_H

#include <unordered_map>
#include "namespace.h"
#include "om/om.h"
#include "om/region.h"
#include "dm/dm.h"
#include "csg/namespace.h"
#include "nav_grid_tile.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/*
 * -- NavGrid 
 *
 * The NavGrid provides an efficient implementation of functions to navigate the world.
 * It's implemented using a sparse-array of 3d-bit vectors (one for each query).  The
 * vectors themselves are managed by the NavGridTile class.  So each query is simply
 * a std::unordered_map() lookup to find the tile for a point and a very fast bit-vector
 * lookup on the proper table.
 *
 */

class NavGrid {
   public: // public methods
      NavGrid(int trace_category);

      bool CanStandOn(om::EntityPtr entity, csg::Point3 const& pt) const;
      void RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& r) const;

      bool IsValidStandingRegion(csg::Region3 const& r) const;
      void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);
   
   private: // methods for internal helper classes
      friend CollisionTracker;
      friend TerrainTracker;
      friend TerrainTileTracker;
      friend RegionCollisionShapeTracker;
      friend VerticalPathingRegionTracker;
      int GetTraceCategory() const;
      void AddTerrainTileTracker(om::EntityRef entity, csg::Point3 const& offset, om::Region3BoxedPtr tile);
      void AddCollisionTracker(NavGridTile::TrackerType type, csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker);

   private: // methods exposed only to the OctTree
      friend OctTree;
      void TrackComponent(std::shared_ptr<dm::Object> component);

   private: // helper methods
      NavGridTile& GridTile(csg::Point3 const& pt) const;
      bool CanStandOn(csg::Cube3 const& cube) const;

   private: // private types
      typedef std::unordered_map<csg::Point3, NavGridTile, csg::Point3::Hash> NavGridTileMap;
      typedef std::unordered_map<dm::ObjectId, CollisionTrackerPtr> CollisionTrackerMap;
      typedef std::unordered_map<csg::Point3, CollisionTrackerPtr, csg::Point3::Hash> TerrainTileCollisionTrackerMap;

   private: // instance variables
      int                              trace_category_;
      mutable NavGridTileMap           tiles_;
      CollisionTrackerMap              collision_trackers_;
      TerrainTileCollisionTrackerMap   terrain_tile_collsion_trackers_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
