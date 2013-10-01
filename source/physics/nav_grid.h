#ifndef _RADIANT_PHYSICS_NAV_GRID_H
#define _RADIANT_PHYSICS_NAV_GRID_H

#include <unordered_map>
#include "namespace.h"
#include "csg/region.h"
#include "om/om.h"
#include "dm/dm.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class NavGrid {
   public:
      NavGrid();

      void TrackComponent(dm::ObjectType type, std::shared_ptr<dm::Object> component);
      bool CanStand(csg::Point3 const& pt) const;
      bool IsEmpty(csg::Point3 const& pt) const;


   private:
      void AddVerticalPathingRegion(om::VerticalPathingRegionPtr vpr);
      void AddRegionCollisionShape(om::RegionCollisionShapePtr region);
      bool CanStandOn(csg::Point3 const& pt) const;
      bool Intersects(csg::Cube3 const& bounds, om::RegionCollisionShapePtr rgnCollsionShape) const;
      bool PointOnLadder(csg::Point3 const& pt) const;

   private:
      bool                    dirty_;
      mutable std::map<dm::ObjectId, om::TerrainRef> terrain_;
      mutable std::map<dm::ObjectId, om::VerticalPathingRegionRef> vprs_;
      mutable std::map<dm::ObjectId, om::RegionCollisionShapeRef>  regionCollisionShapes_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAV_GRID_H
