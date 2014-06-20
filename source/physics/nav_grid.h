#ifndef _RADIANT_PHYSICS_NAV_GRID_H
#define _RADIANT_PHYSICS_NAV_GRID_H

#include <unordered_map>
#include <boost/container/flat_map.hpp>
#include "namespace.h"
#include "om/om.h"
#include "om/region.h"
#include "dm/dm.h"
#include "dm/trace_categories.h"
#include "csg/namespace.h"
#include "nav_grid_tile.h"
#include "derived_region_tracker.h"
#include "om/components/region_collision_shape.ridl.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

// trackers used by the NavGrid
typedef DerivedRegionTracker<om::RegionCollisionShape, TrackerType::COLLISION> RegionCollisionShapeTracker;
typedef DerivedRegionTracker<om::RegionCollisionShape, TrackerType::NON_COLLISION> RegionNonCollisionShapeTracker;
typedef DerivedRegionTracker<om::Destination, TrackerType::DESTINATION> DestinationRegionTracker;
typedef DerivedRegionTracker<om::VerticalPathingRegion, TrackerType::LADDER> VerticalPathingRegionTracker;

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
      NavGrid(dm::TraceCategories trace_category);

      typedef std::function<bool(om::EntityPtr)> ForEachEntityCb;

      // Blocked Queries.  Blocked means there is some opaque voxel making that location
      // unreachable (e.g. inside the ground, or a wall, etc.)
      bool IsBlocked(om::EntityPtr entity, csg::Point3 const& pt);
      bool IsBlocked(csg::Point3 const& worldPoint);
      bool IsBlocked(csg::Region3 const& worldRegion);

      // Support Queries.  Supported means all the blocks directly under the bottom
      // row of the region are capable of supporting weight.
      bool IsSupport(csg::Point3 const& worldPoint);
      bool IsSupport(csg::Region3 const& worldRegion);

      // Standable Queries.  Standable means the region below the bottom of the location
      // is Support and the entire region is not Blocked
      bool IsStandable(csg::Point3 const& worldPoint);
      bool IsStandable(csg::Region3 const& worldRegion);
      bool IsStandable(om::EntityPtr entity, csg::Point3 const& pt);
      csg::Point3 GetStandablePoint(om::EntityPtr entity, csg::Point3 const& pt);

      // Queries.  
      bool IsEntityInCube(om::EntityPtr entity, csg::Cube3 const& worldBounds);
      bool ForEachEntityAtIndex(csg::Point3 const& index, ForEachEntityCb cb);
      bool ForEachEntityInBounds(csg::Cube3 const& worldBounds, ForEachEntityCb cb);
      bool ForEachEntityInRegion(csg::Region3 const& worldRegion, ForEachEntityCb cb);

      // Misc
      void RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& r);
      void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);

   private: // methods for internal helper classes
      friend CollisionTracker;
      friend RegionTracker;
      friend TerrainTracker;
      friend TerrainTileTracker;
      friend RegionCollisionShapeTracker;
      friend RegionNonCollisionShapeTracker;
      friend VerticalPathingRegionTracker;
      friend DestinationRegionTracker;
      friend MobTracker;
      friend SensorTracker;
      friend SensorTileTracker;

      typedef std::function<bool(CollisionTrackerPtr)> ForEachTrackerCb;
      typedef std::function<bool(csg::Point3 const& index, NavGridTile&)> ForEachTileCb;
      typedef std::function<bool(csg::Point3 const& index)> ForEachPointCb;

      dm::TraceCategories GetTraceCategory();
      void AddTerrainTileTracker(om::EntityRef entity, csg::Point3 const& offset, om::Region3BoxedPtr tile);
      void OnTrackerBoundsChanged(csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker);
      void OnTrackerDestroyed(csg::Cube3 const& bounds, dm::ObjectId entityId);
      bool ForEachTileInBounds(csg::Cube3 const& bounds, ForEachTileCb);
      bool ForEachTileInRegion(csg::Region3 const& region, ForEachTileCb cb);
      bool ForEachTrackerInRegion(csg::Region3 const& worldRegion, ForEachTrackerCb cb);
      bool ForEachTrackerForEntity(dm::ObjectId entityId, ForEachTrackerCb cb);
      bool ForEachPointInEntityRegion(dm::ObjectId entityId, csg::Point3 const& offset, ForEachPointCb cb);
      csg::Region3 GetEntityCollisionShape(dm::ObjectId entityId);

   private: // methods exposed only to the OctTree
      friend OctTree;
      void TrackComponent(om::ComponentPtr component);

   private: // helper methods
      friend NavGridTileData;
      NavGridTile& GridTileResident(csg::Point3 const& pt);
      NavGridTile& GridTileNonResident(csg::Point3 const& pt);
      NavGridTile& GridTile(csg::Point3 const& pt, bool make_resident);

   private: // private types
      typedef std::unordered_map<csg::Point3, NavGridTile, csg::Point3::Hash> NavGridTileMap;
      typedef std::vector<std::pair<csg::Point3, bool>> ResidentTileList;
      typedef boost::container::flat_map<dm::ObjectId, CollisionTrackerPtr> CollisionTrackerFlatMap;
      typedef std::unordered_map<dm::ObjectId, CollisionTrackerFlatMap> CollisionTrackerMap;
      typedef std::unordered_map<dm::ObjectId, dm::TracePtr> CollisionTrackerDtorMap;
      typedef std::unordered_map<dm::ObjectId, dm::TracePtr> CollisonTypeTraceMap;
      typedef std::unordered_map<csg::Point3, CollisionTrackerPtr, csg::Point3::Hash> TerrainTileCollisionTrackerMap;

      void AddComponentTracker(CollisionTrackerPtr tracker, om::ComponentPtr component);
      void RemoveComponentTracker(dm::ObjectId entityId, dm::ObjectId componentId);
      CollisionTrackerPtr CreateRegionCollisonShapeTracker(std::shared_ptr<om::RegionCollisionShape> regionCollisionShapePtr);
      void CreateCollisionTypeTrace(std::shared_ptr<om::RegionCollisionShape> regionCollisionShapePtr);
      void OnCollisionTypeChanged(std::weak_ptr<om::RegionCollisionShape> regionCollisionShapeRef);
      void EvictNextUnvisitedTile(csg::Point3 const& pt);

   private: // instance variables
      dm::TraceCategories              trace_category_;
      ResidentTileList                 resident_tiles_;
      int                              last_evicted_;
      NavGridTileMap                   tiles_;
      CollisionTrackerMap              collision_trackers_;
      CollisionTrackerDtorMap          collision_tracker_dtors_;
      CollisonTypeTraceMap             collision_type_traces_;
      TerrainTileCollisionTrackerMap   terrain_tile_collision_trackers_;
      csg::Cube3                       bounds_;
      uint                             max_resident_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
