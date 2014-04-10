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

      bool CanStandOn(om::EntityPtr entity, csg::Point3 const& pt);
      void RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& r);

      bool IsValidStandingRegion(csg::Region3 const& r);
      void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);
   
   private: // methods for internal helper classes
      friend CollisionTracker;
      friend TerrainTracker;
      friend TerrainTileTracker;
      friend RegionCollisionShapeTracker;
      friend VerticalPathingRegionTracker;
      int GetTraceCategory();
      void AddTerrainTileTracker(om::EntityRef entity, csg::Point3 const& offset, om::Region3BoxedPtr tile);
      void AddCollisionTracker(csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker);
      void MarkDirty(csg::Cube3 const& bounds);

   private: // methods exposed only to the OctTree
      friend OctTree;
      void TrackComponent(om::ComponentPtr component);

   private: // helper methods
      friend NavGridTileData;
      NavGridTile& GridTileResident(csg::Point3 const& pt);
      NavGridTile& GridTileNonResident(csg::Point3 const& pt);
      NavGridTile& GridTile(csg::Point3 const& pt, bool make_resident);
      bool CanStandOn(csg::Cube3 const& cube);

   private: // private types
      typedef std::unordered_map<csg::Point3, NavGridTile, csg::Point3::Hash> NavGridTileMap;
      typedef std::vector<std::pair<csg::Point3, bool>> ResidentTileList;
      typedef std::unordered_map<dm::ObjectId, CollisionTrackerPtr> CollisionTrackerMap;
      typedef std::unordered_map<dm::ObjectId, dm::TracePtr> CollisionTrackerDtorMap;
      typedef std::unordered_map<csg::Point3, CollisionTrackerPtr, csg::Point3::Hash> TerrainTileCollisionTrackerMap;

      void EvictNextUnvisitedTile(csg::Point3 const& pt);

   private: // instance variables
      int                              trace_category_;
      ResidentTileList                 resident_tiles_;
      int                              last_evicted_;
      NavGridTileMap                   tiles_;
      CollisionTrackerMap              collision_trackers_;
      CollisionTrackerDtorMap          collision_tracker_dtors_;
      TerrainTileCollisionTrackerMap   terrain_tile_collsion_trackers_;
      csg::Cube3                       bounds_;
      uint                             max_resident_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
