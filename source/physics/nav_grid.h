#ifndef _RADIANT_PHYSICS_NAV_GRID_H
#define _RADIANT_PHYSICS_NAV_GRID_H

#include <unordered_map>
#include "radiant_macros.h"
#include "namespace.h"
#include "csg/region.h"
#include "om/om.h"
#include "om/region.h"
#include "dm/dm.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE


class NavGrid {
   public:
      NavGrid(int trace_category);

      void TrackComponent(dm::ObjectType type, std::shared_ptr<dm::Object> component);
      bool CanStand(csg::Point3 const& pt) const;
      bool IsEmpty(csg::Point3 const& pt) const;
      bool IsEmpty(csg::Cube3 const& pt) const;
      void ClipRegion(csg::Region3& r) const;

      bool CanStandOn(csg::Cube3 const& cube) const;
      bool IsValidStandingRegion(csg::Region3 const& r) const;
      TerrainChangeCbId AddCollisionRegionChangeCb(csg::Region3 const* r, TerrainChangeCb cb);
      void RemoveCollisionRegionChangeCb(TerrainChangeCbId id);

   private:
      void AddVerticalPathingRegion(om::VerticalPathingRegionPtr vpr);
      void AddRegionCollisionShape(om::RegionCollisionShapePtr region);
      bool CanStandOn(csg::Point3 const& pt) const;
      bool Intersects(csg::Cube3 const& bounds, om::RegionCollisionShapePtr rgnCollsionShape) const;
      bool PointOnLadder(csg::Point3 const& pt) const;
      void FireRegionChangeNotifications(csg::Region3 const& r);

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
      TerrainChangeCbId                               next_region_change_cb_id_;
      std::map<TerrainChangeCbId, TerrainChangeEntry> region_change_cb_map_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
