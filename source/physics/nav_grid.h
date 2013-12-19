#ifndef _RADIANT_PHYSICS_NAV_GRID_H
#define _RADIANT_PHYSICS_NAV_GRID_H

#include <unordered_map>
#include "radiant_macros.h"
#include "namespace.h"
#include "csg/region.h"
#include "om/om.h"
#include "om/region.h"
#include "dm/dm.h"
#include "nav_grid_tile.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE


class NavGrid {
   public:
      NavGrid(int trace_category);

      void TrackComponent(std::shared_ptr<dm::Object> component);
      bool CanStandOn(om::EntityPtr entity, csg::Point3 const& pt) const;
      void RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& r) const;

      bool CanStandOn(csg::Cube3 const& cube) const;
      bool IsValidStandingRegion(csg::Region3 const& r) const;
      int GetTraceCategory() const;

   public:
      void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);

   private:
      friend CollisionTracker;
      friend TerrainTracker;
      friend TerrainTileTracker;
      friend RegionCollisionShapeTracker;
      friend VerticalPathingRegionTracker;
      void AddTerrainTileTracker(om::EntityRef entity, csg::Point3 const& offset, om::Region3BoxedPtr tile);
      void AddCollisionTracker(NavGridTile::TrackerType type, csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker);

   private:
      bool IsEmpty(csg::Cube3 const& pt) const;
      NavGridTile& GridTile(csg::Point3 const& pt) const;

   private:
      struct TerrainCollisionObject {
         std::weak_ptr<om::Terrain> obj;
         om::DeepRegionGuardPtr     trace;
         TerrainCollisionObject() { }
         TerrainCollisionObject(std::shared_ptr<om::Terrain> o) : obj(o) { }
      private:
         NO_COPY_CONSTRUCTOR(TerrainCollisionObject)
      };

      struct VerticalPathingRegionCollisionObject {
         std::weak_ptr<om::VerticalPathingRegion> obj;
         core::Guard                                guard;
         VerticalPathingRegionCollisionObject() { }
         VerticalPathingRegionCollisionObject(std::shared_ptr<om::VerticalPathingRegion> o) : obj(o) { }
      private:
         NO_COPY_CONSTRUCTOR(VerticalPathingRegionCollisionObject)
      };

      struct RegionCollisionShapeObject {
         std::weak_ptr<om::RegionCollisionShape> obj;
         om::DeepRegionGuardPtr                  trace;
         RegionCollisionShapeObject() { }
         RegionCollisionShapeObject(std::shared_ptr<om::RegionCollisionShape> o) : obj(o) { }
      private:
         NO_COPY_CONSTRUCTOR(RegionCollisionShapeObject)
      };

      mutable std::unordered_map<dm::ObjectId, std::shared_ptr<TerrainCollisionObject>> terrain_;
      mutable std::unordered_map<dm::ObjectId, std::shared_ptr<VerticalPathingRegionCollisionObject>> vprs_;
      mutable std::unordered_map<dm::ObjectId, std::shared_ptr<RegionCollisionShapeObject>>  regionCollisionShapes_;

     struct TerrainChangeEntry {
         TerrainChangeEntry(csg::Region3 const* r, TerrainChangeCb c) : region(r), cb(c) { }
         csg::Region3 const*  region;
         TerrainChangeCb      cb;
      };

      int                                             trace_category_;
      mutable std::unordered_map<csg::Point3, NavGridTile, csg::Point3::Hash>    nav_grid_;
      std::unordered_map<dm::ObjectId, CollisionTrackerPtr> object_collsion_trackers_;
      std::unordered_map<csg::Point3, CollisionTrackerPtr, csg::Point3::Hash> terrain_tile_collsion_trackers_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
